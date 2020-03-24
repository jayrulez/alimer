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
#include "agpu_internal.h"

#ifndef NOMINMAX
#   define NOMINMAX
#endif 

#if defined(_WIN32)
#   define NODRAWTEXT
#   define NOGDI
#   define NOBITMAP
#   define NOMCX
#   define NOSERVICE
#   define NOHELP
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#endif

#include <d3d12.h>

#if defined(NTDDI_WIN10_RS2)
#   include <dxgi1_6.h>
#else
#   include <dxgi1_5.h>
#endif

#ifdef USE_PIX
#   include "pix3.h"
#endif

#ifdef _DEBUG
#   include <dxgidebug.h>
#endif

#include <D3D12MemAlloc.h>
#include <stdio.h>

#define gpu_ASSERT(expr) { if (!(expr)) __debugbreak(); }
#define VHR(hr) if (FAILED(hr)) { gpu_ASSERT(0); }
#define SAFE_RELEASE(obj) if ((obj)) { (obj)->Release(); (obj) = nullptr; }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY2)(UINT flags, REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_GET_DXGI_DEBUG_INTERFACE1)(UINT flags, REFIID _riid, void** _debug);
#endif

typedef struct {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
    HWND window;
#else
    IUnknown* window;
#endif

    IDXGISwapChain3* handle;
} d3d12_swapchain;

typedef struct {
    ID3D12Fence*    handle;
    HANDLE          eventHandle;
    uint64_t        cpu_value;
} d3d12_fence;

struct TextureD3D12 {
    D3D12MA::Allocation* allocation = nullptr;
    ID3D12Resource* handle = nullptr;
};

typedef struct {
    uint32_t index;
    ID3D12CommandAllocator* command_allocator;
} d3d12_gpu_frame;

static struct {
    bool                    available_initialized = false;
    bool                    available = false;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
    HMODULE                 dxgi_handle = nullptr;
    HMODULE                 d3d12_handle = nullptr;

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

    /* Search for best GPU adapter. */
    D3D_FEATURE_LEVEL min_feature_level = D3D_FEATURE_LEVEL_11_0;

    agpu_config config;
    bool headless;
    bool validation;
    uint32_t max_inflight_frames;

    UINT factory_flags = 0;
    IDXGIFactory4* dxgi_factory = nullptr;
    bool shutting_down = false;

    ID3D12Device* device = nullptr;
    bool is_lost = false;

    /// Memor allocator;
    D3D12MA::Allocator* memory_allocator = nullptr;

    /// Command queue's
    ID3D12CommandQueue* graphics_queue;
    ID3D12CommandQueue* compute_queue;
    ID3D12CommandQueue* copy_queue;

    /// Frame fence.
    d3d12_fence frame_fence;

    d3d12_gpu_frame  frames[4];
    d3d12_gpu_frame* frame;

    /// Maximum supported feature leavel.
    D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;

    /// Root signature version
    D3D_ROOT_SIGNATURE_VERSION root_signature_version = D3D_ROOT_SIGNATURE_VERSION_1_1;

    /* Pools */
    d3d12_swapchain      swapchains[64] = {};
    Pool<TextureD3D12, AGPU_MAX_TEXTURES>           textures;
    //Pool<FramebufferD3D12, GPU_MAX_FRAMEBUFFERS>   framebuffers;
    //Pool<BufferD3D12, GPU_MAX_BUFFERS>             buffers;
    //Pool<ShaderD3D12, GPU_MAX_SHADERS>             shaders;
    //Pool<PipelineD3D12, GPU_MAX_PIPELINES>         pipelines;
    //Pool<SamplerD3D12, GPU_MAX_SAMPLERS>           samplers;

} d3d12;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
#   define agpuCreateDXGIFactory2 d3d12.CreateDXGIFactory2
#   define agpuDXGIGetDebugInterface1 d3d12.DXGIGetDebugInterface1

#   define agpuD3D12CreateDevice d3d12.D3D12CreateDevice
#   define agpuD3D12GetDebugInterface d3d12.D3D12GetDebugInterface
#   define agpuD3D12SerializeRootSignature d3d12.D3D12SerializeRootSignature
#   define agpuD3D12CreateRootSignatureDeserializer d3d12.D3D12CreateRootSignatureDeserializer
#   define agpuD3D12SerializeVersionedRootSignature d3d12.D3D12SerializeVersionedRootSignature
#   define agpuD3D12CreateVersionedRootSignatureDeserializer d3d12.D3D12CreateVersionedRootSignatureDeserializer
#else
#   define agpuCreateDXGIFactory2 CreateDXGIFactory2
#   define agpuDXGIGetDebugInterface1 DXGIGetDebugInterface1

#   define agpuD3D12CreateDevice D3D12CreateDevice
#   define agpuD3D12GetDebugInterface D3D12GetDebugInterface
#   define agpuD3D12SerializeRootSignature D3D12SerializeRootSignature
#   define agpuD3D12CreateRootSignatureDeserializer D3D12CreateRootSignatureDeserializer
#   define agpuD3D12SerializeVersionedRootSignature D3D12SerializeVersionedRootSignature
#   define agpuD3D12CreateVersionedRootSignatureDeserializer D3D12CreateVersionedRootSignatureDeserializer
#endif

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

static D3D12_COMMAND_LIST_TYPE get_d3d12_command_list_type(agpu_queue_type type) {
    static const D3D12_COMMAND_LIST_TYPE queue_types[AGPU_QUEUE_TYPE_COUNT] = {
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        D3D12_COMMAND_LIST_TYPE_COMPUTE,
        D3D12_COMMAND_LIST_TYPE_COPY
    };
    return queue_types[type];
}

static agpu_backend d3d12_get_backend(void) {
    return AGPU_BACKEND_DIRECT3D12;
}

static void d3d12_backend_shutdown(void);
static void d3d12_backend_wait_idle(void);

/* Fence */
static void d3d12_init_fence(d3d12_fence* fence);
static void d3d12_destroy_fence(d3d12_fence* fence);
static uint64_t d3d12_fence_signal(d3d12_fence* fence, ID3D12CommandQueue* queue);
static void d3d12_fence_sync_cpu(d3d12_fence* fence, uint64_t fenceValue);

/* Swapchain */
static void d3d12_init_swap_chain(d3d12_swapchain* swapchain, const agpu_swapchain_desc* desc);
static void d3d12_destroy_swapchain(d3d12_swapchain* swapchain);

static bool d3d12_backend_initialize(const agpu_config* config) {
    // Copy settings
    memcpy(&d3d12.config, config, sizeof(*config));
    if (d3d12.config.flags & AGPU_CONFIG_FLAGS_HEADLESS) {
        d3d12.headless = true;
    }

    if (config->flags & AGPU_CONFIG_FLAGS_VALIDATION | AGPU_CONFIG_FLAGS_GPU_BASED_VALIDATION) {
        d3d12.validation = true;
    }

    d3d12.max_inflight_frames = _gpu_min(_gpu_def(config->max_inflight_frames, 3), 3);

    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    if (d3d12.validation)
    {
        ID3D12Debug* d3d12debug;
        if (SUCCEEDED(agpuD3D12GetDebugInterface(IID_PPV_ARGS(&d3d12debug))))
        {
            d3d12debug->EnableDebugLayer();

            ID3D12Debug1* d3d12debug1;
            if (SUCCEEDED(d3d12debug->QueryInterface(IID_PPV_ARGS(&d3d12debug1))))
            {
                if (config->flags & AGPU_CONFIG_FLAGS_GPU_BASED_VALIDATION)
                {
                    // Enable these if you want full validation (will slow down rendering a lot).
                    d3d12debug1->SetEnableGPUBasedValidation(TRUE);
                    //d3d12debug1->SetEnableSynchronizedCommandQueueValidation(TRUE);
                }
                else {
                    d3d12debug1->SetEnableGPUBasedValidation(FALSE);
                }
                d3d12debug1->Release();
            }

            d3d12debug->Release();
        }
        else
        {
            OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
        }

        IDXGIInfoQueue* dxgiInfoQueue;
        if (SUCCEEDED(agpuDXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
        {
            // Enable additional debug layers.
            d3d12.factory_flags |= DXGI_CREATE_FACTORY_DEBUG;

            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

            DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
            {
                80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
            };
            DXGI_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
            dxgiInfoQueue->Release();
        }
    }

    if (FAILED(agpuCreateDXGIFactory2(d3d12.factory_flags, IID_PPV_ARGS(&d3d12.dxgi_factory))))
    {
        //gpu_error("Direct3D12: Failed to create DXGI factory");
        return false;
    }

    IDXGIAdapter1* dxgi_adapter = nullptr;
#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
    IDXGIFactory6* dxgi_factory6;
    if (SUCCEEDED(d3d12.dxgi_factory->QueryInterface(&dxgi_factory6)))
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
            if (SUCCEEDED(agpuD3D12CreateDevice(adapter, d3d12.min_feature_level, _uuidof(ID3D12Device), nullptr)))
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
        while (d3d12.dxgi_factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND)
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
            if (SUCCEEDED(agpuD3D12CreateDevice(adapter, d3d12.min_feature_level, _uuidof(ID3D12Device), nullptr)))
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
        if (FAILED(d3d12.dxgi_factory->EnumWarpAdapter(IID_PPV_ARGS(&dxgi_adapter))))
        {
            //vgpuLogError("WARP12 not available. Enable the 'Graphics Tools' optional feature");
        }

        OutputDebugStringA("Direct3D Adapter - WARP12\n");
    }
#endif

    if (!dxgi_adapter) {
        return false;
    }

    assert(d3d12.dxgi_factory->IsCurrent());

    if (FAILED(agpuD3D12CreateDevice(dxgi_adapter, d3d12.min_feature_level, IID_PPV_ARGS(&d3d12.device))))
    {
        //vgpuLogError("Direct3D12: Failed to create device");
        return false;
    }

#if defined(_DEBUG)
    if (d3d12.validation)
    {
        // Setup break on error + corruption.
        ID3D12InfoQueue* d3dInfoQueue;
        if (SUCCEEDED(d3d12.device->QueryInterface(IID_PPV_ARGS(&d3dInfoQueue))))
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
        desc.pDevice = d3d12.device;
        desc.pAdapter = dxgi_adapter;

        VHR(D3D12MA::CreateAllocator(&desc, &d3d12.memory_allocator));

        switch (d3d12.memory_allocator->GetD3D12Options().ResourceHeapTier)
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

    // Create command queues.
    {
        // Graphics
        D3D12_COMMAND_QUEUE_DESC queue_desc = {};
        queue_desc.Type = get_d3d12_command_list_type(AGPU_QUEUE_TYPE_GRAPHICS);
        queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queue_desc.NodeMask = 0;
        VHR(d3d12.device->CreateCommandQueue(&queue_desc, __uuidof(ID3D12CommandQueue), (void**)&d3d12.graphics_queue));
        d3d12.graphics_queue->SetName(L"Graphics Command Queue");

        // Compute
        queue_desc.Type = get_d3d12_command_list_type(AGPU_QUEUE_TYPE_COMPUTE);
        VHR(d3d12.device->CreateCommandQueue(&queue_desc, __uuidof(ID3D12CommandQueue), (void**)&d3d12.compute_queue));
        d3d12.compute_queue->SetName(L"Compute Command Queue");

        // Copy
        queue_desc.Type = get_d3d12_command_list_type(AGPU_QUEUE_TYPE_COPY);
        VHR(d3d12.device->CreateCommandQueue(&queue_desc, __uuidof(ID3D12CommandQueue), (void**)&d3d12.copy_queue));
        d3d12.copy_queue->SetName(L"Copy Command Queue");
    }

    // Create frame fence and frame data.
    {
        d3d12_init_fence(&d3d12.frame_fence);
        d3d12.frame = &d3d12.frames[0];

        for (uint32_t i = 0; i < d3d12.max_inflight_frames; i++) {
            d3d12.frames[i].index = i;

            VHR(d3d12.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&d3d12.frames[i].command_allocator)));
        }
    }

    /* Create main swap chain*/
    if (config->swapchain) {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        d3d12.swapchains[0].window = (HWND)config->swapchain->native_handle;
#else
        g_d3d12.swapchains[0].window = (IUnknown*)config->swapchain->native_handle;
        IUnknown* window;
#endif
        d3d12_init_swap_chain(&d3d12.swapchains[0], config->swapchain);
    }

    /* Init pools first. */
    d3d12.textures.init();
    //g_d3d12.framebuffers.init();
    //g_d3d12.buffers.init();
    //g_d3d12.shaders.init();
    //g_d3d12.pipelines.init();
    //g_d3d12.samplers.init();

    return true;
}

static void d3d12_backend_shutdown(void) {
    if (d3d12.device == nullptr)
        return;

    d3d12_backend_wait_idle();

    /* Destroy swap chains.*/
    for (d3d12_swapchain& swapchain : d3d12.swapchains) {
        if (!swapchain.handle) continue;

        d3d12_destroy_swapchain(&swapchain);
    }

    // Destroy frame data.
    {
        for (uint32_t i = 0; i < d3d12.max_inflight_frames; i++) {
            d3d12_gpu_frame* frame = &d3d12.frames[i];
            SAFE_RELEASE(frame->command_allocator);
        }

        d3d12_destroy_fence(&d3d12.frame_fence);
    }

    // Queue's
    {
        SAFE_RELEASE(d3d12.copy_queue);
        SAFE_RELEASE(d3d12.compute_queue);
        SAFE_RELEASE(d3d12.graphics_queue);
    }

    // Allocator
    D3D12MA::Stats stats;
    d3d12.memory_allocator->CalculateStats(&stats);

    if (stats.Total.UsedBytes > 0) {
        //vgpuLogErrorFormat("Total device memory leaked: %lu bytes.", stats.Total.UsedBytes);
    }

    SAFE_RELEASE(d3d12.memory_allocator);

#if !defined(NDEBUG)
    ULONG refCount = d3d12.device->Release();
    if (refCount > 0)
    {
        //DebugString("Direct3D12: There are %d unreleased references left on the device", refCount);

        ID3D12DebugDevice* debugDevice = nullptr;
        if (SUCCEEDED(d3d12.device->QueryInterface(&debugDevice)))
        {
            debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
            debugDevice->Release();
        }
    }
#else
    d3d12.device->Release();
#endif

    d3d12.dxgi_factory->Release();

#ifdef _DEBUG
    {
        IDXGIDebug1* dxgiDebug;
        if (SUCCEEDED(agpuDXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
        {
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            dxgiDebug->Release();
        }
    }
#endif
}

static void d3d12_backend_wait_idle(void) {
    d3d12_fence_signal(&d3d12.frame_fence, d3d12.graphics_queue);
    d3d12_fence_sync_cpu(&d3d12.frame_fence, 0);
}

static void d3d12_backend_begin_frame(void) {
    //d3d12_frame* frame = &d3d12.frames[vk_state.frame];
}

static void d3d12_backend_end_frame(void) {
    if (d3d12.is_lost)
        return;

    HRESULT hr = S_OK;
    for (d3d12_swapchain& swapchain : d3d12.swapchains) {
        if (!swapchain.handle) continue;

        UINT sync_interval = 1; // d3d12.vsync ? 1 : 0;
        UINT present_flags = 0; // (!d3d12.vsync) ? DXGI_PRESENT_ALLOW_TEARING : 0;

        hr = swapchain.handle->Present(sync_interval, present_flags);

        if (hr == DXGI_ERROR_DEVICE_REMOVED
            || hr == DXGI_ERROR_DEVICE_HUNG
            || hr == DXGI_ERROR_DEVICE_RESET
            || hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR
            || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
        {

#ifdef _DEBUG
            char buff[64] = {};
            sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n", (hr == DXGI_ERROR_DEVICE_REMOVED) ? d3d12.device->GetDeviceRemovedReason() : hr);
            OutputDebugStringA(buff);
#endif
            d3d12.is_lost = true;
            //HandleDeviceLost();
            return;
        }
    }

    d3d12_fence_signal(&d3d12.frame_fence, d3d12.graphics_queue);
    if (d3d12.frame_fence.cpu_value >= d3d12.max_inflight_frames) {
        d3d12_fence_sync_cpu(&d3d12.frame_fence, d3d12.frame_fence.cpu_value - d3d12.max_inflight_frames);
    }

    // Advance to next frame.
    d3d12.frame = &d3d12.frames[(d3d12.frame->index + 1) % d3d12.max_inflight_frames];
}

static void d3d12_init_fence(d3d12_fence* fence) {
    fence->cpu_value = 0;
    VHR(d3d12.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence->handle)));
    fence->eventHandle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
    if (fence->eventHandle == nullptr) {
        //ALIMER_LOGERROR("Failed to create an event object");
    }

    fence->cpu_value++;
}

static void d3d12_destroy_fence(d3d12_fence* fence) {
    CloseHandle(fence->eventHandle);
    fence->handle->Release();
}

static uint64_t d3d12_fence_signal(d3d12_fence* fence, ID3D12CommandQueue* queue) {
    assert(queue);
    VHR(queue->Signal(fence->handle, fence->cpu_value));
    fence->cpu_value++;
    return fence->cpu_value - 1;
}

static void d3d12_fence_sync_cpu(d3d12_fence* fence, uint64_t fenceValue) {
    uint64_t sync_val = fenceValue == 0 ? (fence->cpu_value - 1) : fenceValue;
    uint64_t gpu_value = fence->handle->GetCompletedValue();
    if (gpu_value < sync_val)
    {
        VHR(fence->handle->SetEventOnCompletion(sync_val, fence->eventHandle));
        WaitForSingleObject(fence->eventHandle, INFINITE);
    }
}

/* Swapchain */
static void d3d12_init_swap_chain(d3d12_swapchain* swapchain, const agpu_swapchain_desc* desc) {
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
    if (swapchain->handle != nullptr)
    {
        VHR(swapchain->handle->GetDesc1(&swap_chain_desc));

        HRESULT hr = swapchain->handle->ResizeBuffers(
            swap_chain_desc.BufferCount,
            width, height,
            swap_chain_desc.Format,
            swap_chain_desc.Flags);

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
#ifdef _DEBUG
            char buff[64] = {};
            sprintf_s(buff, "Device Lost on ResizeBuffers: Reason code 0x%08X\n", (hr == DXGI_ERROR_DEVICE_REMOVED) ? d3d12.device->GetDeviceRemovedReason() : hr);
            OutputDebugStringA(buff);
#endif
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            //device->HandleDeviceLost();

            // Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method
            // and correctly set up the new device.
            return;
        }
        else
        {
            VHR(hr);
        }
    }
    else
    {
        swap_chain_desc = {};
        swap_chain_desc.Width = width;
        swap_chain_desc.Height = height;
        swap_chain_desc.Format = back_buffer_dxgi_format;
        swap_chain_desc.Stereo = FALSE;
        swap_chain_desc.SampleDesc.Count = 1;
        swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
        swap_chain_desc.BufferCount = d3d12.max_inflight_frames;

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
        VHR(d3d12.dxgi_factory->CreateSwapChainForHwnd(
            d3d12.graphics_queue,
            swapchain->window,
            &swap_chain_desc,
            &swap_chain_fullscreen_desc,
            nullptr,
            &tempSwapChain
            ));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        VHR(d3d12.dxgi_factory->MakeWindowAssociation(swapchain->window, DXGI_MWA_NO_ALT_ENTER));
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
}

void d3d12_destroy_swapchain(d3d12_swapchain* swapchain) {
    if (swapchain->handle == nullptr) 
        return;

    /*if (swapChain->depthStencilTexture.isValid()) {
        vgpu_destroy_texture(swapChain->depthStencilTexture);
    }

    for (uint32_t i = 0; i < kD3D12FrameCount; ++i) {
        vgpu_destroy_texture(swapChain->backBufferTextures[i]);
        vgpu_destroy_framebuffer(swapChain->framebuffers[i]);
    }*/

    swapchain->handle->Release();
    swapchain->handle = nullptr;
}

bool agpu_d3d12_supported(void) {

    if (d3d12.available_initialized) {
        return d3d12.available;
    }

    d3d12.available_initialized = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
    d3d12.dxgi_handle = LoadLibraryW(L"dxgi.dll");
    if (d3d12.dxgi_handle == nullptr) {
        return false;
    }

    d3d12.CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(d3d12.dxgi_handle, "CreateDXGIFactory2");
    if (d3d12.CreateDXGIFactory2 == nullptr) {
        return false;
    }

    d3d12.DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(d3d12.dxgi_handle, "DXGIGetDebugInterface1");

    d3d12.d3d12_handle = LoadLibraryW(L"d3d12.dll");
    if (d3d12.d3d12_handle == nullptr) {
        return false;
    }

    d3d12.D3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(d3d12.d3d12_handle, "D3D12CreateDevice");
    if (d3d12.D3D12CreateDevice == nullptr) {
        return false;
    }

    d3d12.D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(d3d12.d3d12_handle, "D3D12GetDebugInterface");
    d3d12.D3D12SerializeRootSignature = (PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)GetProcAddress(d3d12.d3d12_handle, "D3D12SerializeRootSignature");
    d3d12.D3D12CreateRootSignatureDeserializer = (PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(d3d12.d3d12_handle, "D3D12CreateRootSignatureDeserializer");
    d3d12.D3D12SerializeVersionedRootSignature = (PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE)GetProcAddress(d3d12.d3d12_handle, "D3D12SerializeVersionedRootSignature");
    d3d12.D3D12CreateVersionedRootSignatureDeserializer = (PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(d3d12.d3d12_handle, "D3D12CreateVersionedRootSignatureDeserializer");
#endif

    // Create temp factory and try to create harware d3d12 device.
    IDXGIFactory4* temp_dxgi_factory;
    HRESULT hr = agpuCreateDXGIFactory2(0, __uuidof(IDXGIFactory4), (void**)&temp_dxgi_factory);
    if (FAILED(hr))
    {
        return false;
    }
    SAFE_RELEASE(temp_dxgi_factory);

    if (SUCCEEDED(agpuD3D12CreateDevice(nullptr, d3d12.min_feature_level, _uuidof(ID3D12Device), nullptr)))
    {
        d3d12.available = true;
    }

    return d3d12.available;
}

agpu_renderer* agpu_create_d3d12_backend(void) {
    static agpu_renderer renderer = { nullptr };
    renderer.get_backend = d3d12_get_backend;
    renderer.initialize = d3d12_backend_initialize;
    renderer.shutdown = d3d12_backend_shutdown;
    renderer.wait_idle = d3d12_backend_wait_idle;
    renderer.begin_frame = d3d12_backend_begin_frame;
    renderer.end_frame = d3d12_backend_end_frame;
    return &renderer;
}

#undef VHR
#undef SAFE_RELEASE

#else

bool agpu_d3d12_supported(void) {
    return false;
}

agpu_renderer* agpu_create_d3d12_backend(void) {
    return nullptr;
}

#endif /* defined(GPU_D3D12_BACKEND) */
