//
// Copyright (c) 2020 Amer Koleci and contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#if defined(GPU_D3D12_BACKEND)
#include "gpu_backend.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif 

#if defined(_WIN32)
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <d3d12.h>

#if defined(NTDDI_WIN10_RS2)
#include <dxgi1_6.h>
#else
#include <dxgi1_5.h>
#endif

#ifdef USE_PIX
#include "pix3.h"
#endif

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#include <D3D12MemAlloc.h>

#define gpu_ASSERT(expr) { if (!(expr)) __debugbreak(); }
#define VHR(hr) if (FAILED(hr)) { gpu_ASSERT(0); }
#define SAFE_RELEASE(obj) if ((obj)) { (obj)->Release(); (obj) = nullptr; }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY2)(UINT flags, REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_GET_DXGI_DEBUG_INTERFACE1)(UINT flags, REFIID _riid, void** _debug);
#endif

namespace gpu
{
    struct SwapchainD3D12 {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
        HWND window;
#else
        IUnknown* window;
#endif

        IDXGISwapChain3* handle;
    };

    struct TextureD3D12 {
        D3D12MA::Allocation* allocation = nullptr;
        ID3D12Resource* handle = nullptr;
    };

    static const uint32_t kD3D12FrameCount = 3;
    static struct {
        bool                    available_initialized = false;
        bool                    available = false;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
        HMODULE                 dxgi_handle = nullptr;
        HMODULE                 d3d12_handle = nullptr;
#endif

        UINT factory_flags = 0;
        IDXGIFactory4* dxgi_factory = nullptr;
        bool shutting_down = false;

        /// Minimum requested feature level.
        ID3D12Device* device = nullptr;

        /// Memor allocator;
        D3D12MA::Allocator* memory_allocator = nullptr;

        /// Command queue's
        ID3D12CommandQueue* graphics_queue = nullptr;
        ID3D12CommandQueue* compute_queue = nullptr;
        ID3D12CommandQueue* copy_queue = nullptr;

        /// Maximum supported feature leavel.
        D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;

        /// Root signature version
        D3D_ROOT_SIGNATURE_VERSION root_signature_version = D3D_ROOT_SIGNATURE_VERSION_1_1;

        /* Pools */
        SwapchainD3D12      swapchains[64] = {};
        Pool<TextureD3D12, GPU_MAX_TEXTURES>           textures;
        //Pool<FramebufferD3D12, GPU_MAX_FRAMEBUFFERS>   framebuffers;
        //Pool<BufferD3D12, GPU_MAX_BUFFERS>             buffers;
        //Pool<ShaderD3D12, GPU_MAX_SHADERS>             shaders;
        //Pool<PipelineD3D12, GPU_MAX_PIPELINES>         pipelines;
        //Pool<SamplerD3D12, GPU_MAX_SAMPLERS>           samplers;

    } g_d3d12;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    // DXGI functions
    PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2 = nullptr;
    PFN_GET_DXGI_DEBUG_INTERFACE1 DXGIGetDebugInterface1 = nullptr;
    // D3D12 functions.
    PFN_D3D12_CREATE_DEVICE D3D12CreateDevice = nullptr;
    PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface = nullptr;
    PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignature = nullptr;
    PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER D3D12CreateRootSignatureDeserializer = nullptr;
    PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE D3D12SerializeVersionedRootSignature = nullptr;
    PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER D3D12CreateVersionedRootSignatureDeserializer = nullptr;
#endif

    class D3D12Device final : public Device
    {
    public:
        D3D12Device() = default;
        ~D3D12Device();

        bool init(const gpu_config* config) override;
        void shutdown() override;
        void init_swap_chain(SwapchainD3D12* swapchain, const gpu_swapchain_desc* desc);
        void destroy_swapchain(SwapchainD3D12* swapchain);
    };

    /* Conversion functions */

    static DXGI_FORMAT _gpu_d3d_swapchain_pixel_format(agpu_pixel_format format) {
        switch (format)
        {
        case AGPU_PIXEL_FORMAT_UNDEFINED:
        case AGPU_PIXEL_FORMAT_BGRA8_UNORM:
        case AGPU_PIXEL_FORMAT_BGRA8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8A8_UNORM;

        case AGPU_PIXEL_FORMAT_RGBA8_UNORM:
        case AGPU_PIXEL_FORMAT_RGBA8_UNORM_SRGB:
            return DXGI_FORMAT_R8G8B8A8_UNORM;

        case AGPU_PIXEL_FORMAT_RGBA16_FLOAT:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;

        case AGPU_PIXEL_FORMAT_RGB10A2_UNORM:
            return DXGI_FORMAT_R10G10B10A2_UNORM;

        default:
            //vgpuLogErrorFormat("PixelFormat (%u) is not supported for creating swapchain buffer", (uint32_t)format);
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    /* Implementation */
    D3D12Device::~D3D12Device()
    {

    }

    bool D3D12Device::init(const gpu_config* config)
    {
#if defined(_DEBUG)
        // Enable the debug layer (requires the Graphics Tools "optional feature").
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        if (config->validation)
        {
            ID3D12Debug* d3d12debug;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&d3d12debug))))
            {
                d3d12debug->EnableDebugLayer();

                ID3D12Debug1* d3d12debug1;
                if (SUCCEEDED(d3d12debug->QueryInterface(IID_PPV_ARGS(&d3d12debug1))))
                {
                    d3d12debug1->SetEnableGPUBasedValidation(FALSE);
                    /*if (d3d12.gpuValidation) {
                        // Enable these if you want full validation (will slow down rendering a lot).
                        d3d12debug1->SetEnableGPUBasedValidation(true);
                        //d3d12debug1->SetEnableSynchronizedCommandQueueValidation(true);
                    }
                    else {
                        d3d12debug1->SetEnableGPUBasedValidation(false);
                    }*/
                }

                SAFE_RELEASE(d3d12debug1);
                SAFE_RELEASE(d3d12debug);
            }
            else
            {
                OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
            }

            IDXGIInfoQueue* dxgiInfoQueue;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
            {
                // Enable additional debug layers.
                g_d3d12.factory_flags |= DXGI_CREATE_FACTORY_DEBUG;

                // Declare debug guids to avoid linking with "dxguid.lib"
                static const GUID g_DXGI_DEBUG_ALL = { 0xe48ae283, 0xda80, 0x490b, {0x87, 0xe6, 0x43, 0xe9, 0xa9, 0xcf, 0xda, 0x8} };
                static const GUID g_DXGI_DEBUG_DXGI = { 0x25cddaa4, 0xb1c6, 0x47e1, {0xac, 0x3e, 0x98, 0x87, 0x5b, 0x5a, 0x2e, 0x2a} };

                dxgiInfoQueue->SetBreakOnSeverity(g_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
                dxgiInfoQueue->SetBreakOnSeverity(g_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

                DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
                {
                    80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
                };
                DXGI_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                dxgiInfoQueue->AddStorageFilterEntries(g_DXGI_DEBUG_DXGI, &filter);
                SAFE_RELEASE(dxgiInfoQueue);
            }
        }
#endif
        if (FAILED(CreateDXGIFactory2(g_d3d12.factory_flags, IID_PPV_ARGS(&g_d3d12.dxgi_factory))))
        {
            //gpu_error("Direct3D12: Failed to create DXGI factory");
            return false;
        }

        /* Search for best GPU adapter. */
        static constexpr D3D_FEATURE_LEVEL min_feature_level = D3D_FEATURE_LEVEL_11_0;

        IDXGIAdapter1* dxgi_adapter = nullptr;
#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        IDXGIFactory6* dxgi_factory6;
        if (SUCCEEDED(g_d3d12.dxgi_factory->QueryInterface(&dxgi_factory6)))
        {
            IDXGIAdapter1* adapter;
            for (UINT adapterIndex = 0;
                SUCCEEDED(dxgi_factory6->EnumAdapterByGpuPreference(
                    adapterIndex,
                    DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                    IID_PPV_ARGS(&adapter)));
                adapterIndex++)
            {
                DXGI_ADAPTER_DESC1 desc = {};
                adapter->GetDesc1(&desc);

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    adapter->Release();
                    // Don't select the Basic Render Driver adapter.
                    continue;
                }

                // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
                if (SUCCEEDED(D3D12CreateDevice(adapter, min_feature_level, _uuidof(ID3D12Device), nullptr)))
                {
                    dxgi_adapter = adapter;
                    break;
                }

                adapter->Release();
            }
            dxgi_factory6->Release();
        }
#endif

        if (dxgi_adapter == nullptr)
        {
            IDXGIAdapter1* adapter;
            UINT i = 0;
            while (g_d3d12.dxgi_factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND)
            {
                i++;
                DXGI_ADAPTER_DESC1 desc;
                adapter->GetDesc1(&desc);

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    adapter->Release();
                    // Don't select the Basic Render Driver adapter.
                    continue;
                }

                // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
                if (SUCCEEDED(D3D12CreateDevice(adapter, min_feature_level, _uuidof(ID3D12Device), nullptr)))
                {
                    dxgi_adapter = adapter;
                    break;
                }
            }
        }

#if !defined(NDEBUG)
        if (!dxgi_adapter)
        {
            // Try WARP12 instead
            if (FAILED(g_d3d12.dxgi_factory->EnumWarpAdapter(IID_PPV_ARGS(&dxgi_adapter))))
            {
                //vgpuLogError("WARP12 not available. Enable the 'Graphics Tools' optional feature");
            }

            OutputDebugStringA("Direct3D Adapter - WARP12\n");
        }
#endif

        if (!dxgi_adapter) {
            return false;
        }

        assert(g_d3d12.dxgi_factory->IsCurrent());

        if (FAILED(D3D12CreateDevice(dxgi_adapter, min_feature_level, IID_PPV_ARGS(&g_d3d12.device))))
        {
            //vgpuLogError("Direct3D12: Failed to create device");
            return false;
        }

#if defined(_DEBUG)
        if (config->validation)
        {
            // Setup break on error + corruption.
            ID3D12InfoQueue* d3dInfoQueue;
            if (SUCCEEDED(g_d3d12.device->QueryInterface(IID_PPV_ARGS(&d3dInfoQueue))))
            {
#ifdef _DEBUG
                d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
                d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
#endif

                D3D12_MESSAGE_ID hide[] =
                {
                    D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
                    D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
                    //D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE,
                    D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                    D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
                    //D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE
                };

                D3D12_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                d3dInfoQueue->AddStorageFilterEntries(&filter);
                d3dInfoQueue->Release();
            }
        }
#endif

        //InitCapsAndLimits();

        // Create memory allocator
        {
            D3D12MA::ALLOCATOR_DESC desc = {};
            desc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
            desc.pDevice = g_d3d12.device;
            desc.pAdapter = dxgi_adapter;

            /*D3D12MA::ALLOCATION_CALLBACKS allocationCallbacks = {};
            if (ENABLE_CPU_ALLOCATION_CALLBACKS)
            {
                allocationCallbacks.pAllocate = &CustomAllocate;
                allocationCallbacks.pFree = &CustomFree;
                desc.pAllocationCallbacks = &allocationCallbacks;
            }*/

            VHR(D3D12MA::CreateAllocator(&desc, &g_d3d12.memory_allocator));

            switch (g_d3d12.memory_allocator->GetD3D12Options().ResourceHeapTier)
            {
            case D3D12_RESOURCE_HEAP_TIER_1:
                //vgpuLog(VGPU_LOG_TYPE_INFO, "ResourceHeapTier = D3D12_RESOURCE_HEAP_TIER_1");
                break;
            case D3D12_RESOURCE_HEAP_TIER_2:
                //vgpuLog(VGPU_LOG_TYPE_INFO, "ResourceHeapTier = D3D12_RESOURCE_HEAP_TIER_2");
                break;
            default:
                assert(0);
            }
        }

        SAFE_RELEASE(dxgi_adapter);

        // Create command queue's
        {
            /* Direct */
            D3D12_COMMAND_QUEUE_DESC queueDesc = {};
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
            queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            VHR(g_d3d12.device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_d3d12.graphics_queue)));
            g_d3d12.graphics_queue->SetName(L"Graphics Command Queue");

            // Compute 
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
            VHR(g_d3d12.device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_d3d12.compute_queue)));
            g_d3d12.compute_queue->SetName(L"Compute Command Queue");

            // Copy 
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
            VHR(g_d3d12.device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_d3d12.copy_queue)));
            g_d3d12.copy_queue->SetName(L"Copy Command Queue");
        }

        /* Create main swap chain*/
        if (config->swapchain) {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            g_d3d12.swapchains[0].window = (HWND)config->swapchain->native_handle;
#else
            g_d3d12.swapchains[0].window = (IUnknown*)config->swapchain->native_handle;
            IUnknown* window;
#endif
            init_swap_chain(&g_d3d12.swapchains[0], config->swapchain);
        }

        /* Init pools first. */
        g_d3d12.textures.init();
        //g_d3d12.framebuffers.init();
        //g_d3d12.buffers.init();
        //g_d3d12.shaders.init();
        //g_d3d12.pipelines.init();
        //g_d3d12.samplers.init();

        return true;
    }

    void D3D12Device::shutdown()
    {
        if (g_d3d12.device == nullptr)
        {
            return;
        }

        //wait_idle();
        g_d3d12.shutting_down = true;

        /* Destroy swap chains.*/
        for (SwapchainD3D12& swapchain : g_d3d12.swapchains) {
            if (!swapchain.handle) continue;

            destroy_swapchain(&swapchain);
        }

        // Queue's
        {
            SAFE_RELEASE(g_d3d12.copy_queue);
            SAFE_RELEASE(g_d3d12.compute_queue);
            SAFE_RELEASE(g_d3d12.graphics_queue);
        }

        // Allocator
        D3D12MA::Stats stats;
        g_d3d12.memory_allocator->CalculateStats(&stats);

        if (stats.Total.UsedBytes > 0) {
            //vgpuLogErrorFormat("Total device memory leaked: %lu bytes.", stats.Total.UsedBytes);
        }

        SAFE_RELEASE(g_d3d12.memory_allocator);

        ULONG refCount = g_d3d12.device->Release();
#if !defined(NDEBUG)
        if (refCount > 0)
        {
            //DebugString("Direct3D12: There are %d unreleased references left on the device", refCount);

            ID3D12DebugDevice* debugDevice = nullptr;
            if (SUCCEEDED(g_d3d12.device->QueryInterface(&debugDevice)))
            {
                debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
                debugDevice->Release();
            }
        }
#else
        (void)refCount; // avoid warning
#endif

        SAFE_RELEASE(g_d3d12.dxgi_factory);
    }

    void D3D12Device::init_swap_chain(SwapchainD3D12* swapchain, const gpu_swapchain_desc* desc)
    {
        uint32_t width = desc->width;
        uint32_t height = desc->height;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
        const HWND window = reinterpret_cast<HWND>(desc->native_handle);

        if (!IsWindow(window))
        {
            //vgpuLogError("Direct3D12: Invalid Win32 window handle");
            return;
        }

        if (width == 0 || height == 0)
        {
            RECT rect;
            if (GetClientRect(window, &rect))
            {
                width = rect.right - rect.left;
                height = rect.bottom - rect.top;
            }
        }
#else
        IUnknown* window = reinterpret_cast<IUnknown*>(desc->native_handle);
#endif

        const DXGI_FORMAT back_buffer_dxgi_format = _gpu_d3d_swapchain_pixel_format(desc->color_format);

        DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
        swap_chain_desc.Width = width;
        swap_chain_desc.Height = height;
        swap_chain_desc.Format = back_buffer_dxgi_format;
        swap_chain_desc.Stereo = FALSE;
        swap_chain_desc.SampleDesc.Count = 1;
        swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
        swap_chain_desc.BufferCount = kD3D12FrameCount;

#if defined(_XBOX_ONE)
        swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
#else
        swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
#endif

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
        swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
#else
        swap_chain_desc.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
#endif
        swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        IDXGISwapChain1* tempSwapChain = nullptr;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC swap_chain_fullscreen_desc = {};
        memset(&swap_chain_fullscreen_desc, 0, sizeof(swap_chain_fullscreen_desc));
        swap_chain_fullscreen_desc.Windowed = TRUE;

        // Create a SwapChain from a Win32 window.
        VHR(g_d3d12.dxgi_factory->CreateSwapChainForHwnd(
            g_d3d12.graphics_queue,
            swapchain->window,
            &swap_chain_desc,
            &swap_chain_fullscreen_desc,
            nullptr,
            &tempSwapChain
        ));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        VHR(g_d3d12.dxgi_factory->MakeWindowAssociation(swapchain->window, DXGI_MWA_NO_ALT_ENTER));
#else
        // Create a swap chain for the window.
        VHR(g_d3d12.dxgi_factory->CreateSwapChainForCoreWindow(
            g_d3d12.graphics_queue,
            swapchain->window,
            &swapChainDesc,
            nullptr,
            &tempSwapChain
        ));

        VHR(tempSwapChain->SetRotation(DXGI_MODE_ROTATION_IDENTITY));
#endif

        VHR(tempSwapChain->QueryInterface(IID_PPV_ARGS(&swapchain->handle)));
        SAFE_RELEASE(tempSwapChain);
    }

    void D3D12Device::destroy_swapchain(SwapchainD3D12* swapchain)
    {
        /*if (swapChain->depthStencilTexture.isValid()) {
            vgpu_destroy_texture(swapChain->depthStencilTexture);
        }

        for (uint32_t i = 0; i < kD3D12FrameCount; ++i) {
            vgpu_destroy_texture(swapChain->backBufferTextures[i]);
            vgpu_destroy_framebuffer(swapChain->framebuffers[i]);
        }*/

        SAFE_RELEASE(swapchain->handle);
    }

    bool d3d12_supported()
    {
        if (g_d3d12.available_initialized) {
            return g_d3d12.available;
        }

        g_d3d12.available_initialized = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
        g_d3d12.dxgi_handle = LoadLibraryW(L"dxgi.dll");
        if (g_d3d12.dxgi_handle == nullptr) {
            return false;
        }

        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(g_d3d12.dxgi_handle, "CreateDXGIFactory2");
        if (CreateDXGIFactory2 == nullptr) {
            return false;
        }

        DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(g_d3d12.dxgi_handle, "DXGIGetDebugInterface1");

        g_d3d12.d3d12_handle = LoadLibraryW(L"d3d12.dll");
        if (g_d3d12.d3d12_handle == nullptr) {
            return false;
        }

        D3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(g_d3d12.d3d12_handle, "D3D12CreateDevice");
        if (D3D12CreateDevice == nullptr) {
            return false;
        }

        D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(g_d3d12.d3d12_handle, "D3D12GetDebugInterface");
        D3D12SerializeRootSignature = (PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)GetProcAddress(g_d3d12.d3d12_handle, "D3D12SerializeRootSignature");
        D3D12CreateRootSignatureDeserializer = (PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(g_d3d12.d3d12_handle, "D3D12CreateRootSignatureDeserializer");
        D3D12SerializeVersionedRootSignature = (PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE)GetProcAddress(g_d3d12.d3d12_handle, "D3D12SerializeVersionedRootSignature");
        D3D12CreateVersionedRootSignatureDeserializer = (PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(g_d3d12.d3d12_handle, "D3D12CreateVersionedRootSignatureDeserializer");
#endif

        g_d3d12.available = true;
        return g_d3d12.available;
    }

    Device* create_d3d12_backend()
    {
        if (!d3d12_supported())
        {
            //vgpuLogError("Direct3D12 is not supported");
            return false;
        }

        return new D3D12Device();
    }
}

#undef VHR
#undef SAFE_RELEASE
#endif 
