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

#if defined(VGPU_DRIVER_D3D12)

#include "vgpu_d3d_common.h"

// Use the C++ standard templated min / max
#define NOMINMAX

// DirectX apps don't need GDI
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP

// Include <mcx.h> if you need this
#define NOMCX

// Include <winsvc.h> if you need this
#define NOSERVICE

// WinHelp is deprecated
#define NOHELP

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <d3d12.h>
#include "D3D12MemAlloc.h"

// To use graphics and CPU markup events with the latest version of PIX, change this to include <pix3.h>
// then add the NuGet package WinPixEventRuntime to the project.
#include <pix.h>

#ifdef _DEBUG
#include <dxgidebug.h>

static const GUID vgpu_DXGI_DEBUG_ALL = { 0xe48ae283, 0xda80, 0x490b, {0x87, 0xe6, 0x43, 0xe9, 0xa9, 0xcf, 0xda, 0x8} };
static const GUID vgpu_DXGI_DEBUG_DXGI = { 0x25cddaa4, 0xb1c6, 0x47e1, {0xac, 0x3e, 0x98, 0x87, 0x5b, 0x5a, 0x2e, 0x2a} };
#endif

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY2)(UINT flags, REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_GET_DXGI_DEBUG_INTERFACE1)(UINT flags, REFIID _riid, void** _debug);
#endif

#include <queue>

struct ResourceRelease
{
    uint64_t frameID;
    IUnknown* resource;
};

/* Global data */
static struct {
    bool available_initialized;
    bool available;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    struct
    {
        HMODULE instance;
        PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2;
        PFN_GET_DXGI_DEBUG_INTERFACE1 DXGIGetDebugInterface1;
    } dxgi;

    HMODULE instance;
    PFN_D3D12_CREATE_DEVICE D3D12CreateDevice;
    PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface;
    PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignature;
    PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER D3D12CreateRootSignatureDeserializer;
    PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE D3D12SerializeVersionedRootSignature;
    PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER D3D12CreateVersionedRootSignatureDeserializer;
#endif

    UINT factory_flags;
    IDXGIFactory4* factory;
    bool tearing_support;

    D3D_FEATURE_LEVEL min_feature_level = D3D_FEATURE_LEVEL_11_0;
    ID3D12Device* device;
    D3D12MA::Allocator* allocator;

    ID3D12CommandQueue* direct_command_queue;
    ID3D12CommandQueue* compute_command_queue;

    IDXGISwapChain3* swapchain;
    uint32_t backbuffer_index;

    /* Frame data */
    uint64_t render_latency;
    ID3D12Fence* frame_fence;
    HANDLE frame_fence_event;
    uint64_t frame_number;
    uint64_t frame_index;


    ID3D12CommandAllocator* commandAllocators[3u];
    ID3D12GraphicsCommandList* command_list;

    std::queue<ResourceRelease> deferred_releases;
} d3d12;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#   define _vgpu_CreateDXGIFactory2 d3d12.dxgi.CreateDXGIFactory2
#   define _vgpu_D3D12GetDebugInterface d3d12.D3D12GetDebugInterface
#   define _vgpu_D3D12CreateDevice d3d12.D3D12CreateDevice
#else
#   define _vgpu_CreateDXGIFactory2 CreateDXGIFactory2
#   define _vgpu_D3D12GetDebugInterface D3D12GetDebugInterface
#   define _vgpu_D3D12CreateDevice D3D12CreateDevice
#endif

/* Renderer */
static void d3d12_release_resource(IUnknown* resource)
{
    if (resource == nullptr) {
        return;
    }

    d3d12.deferred_releases.push({ d3d12.frame_number, resource });
}

static void d3d12_execute_deferred_releases() {
    uint64_t gpu_value = d3d12.frame_fence->GetCompletedValue();
    while (d3d12.deferred_releases.size() && d3d12.deferred_releases.front().frameID <= gpu_value)
    {
        d3d12.deferred_releases.front().resource->Release();
        d3d12.deferred_releases.pop();
    }
}

static IDXGIAdapter1* d3d12_get_adapter(vgpu_device_preference device_preference) {
    /* Detect adapter now. */
    IDXGIAdapter1* adapter = nullptr;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
    if (device_preference != VGPU_DEVICE_PREFERENCE_DONT_CARE) {
        IDXGIFactory6* dxgiFactory6;
        HRESULT hr = d3d12.factory->QueryInterface(IID_PPV_ARGS(&dxgiFactory6));
        if (SUCCEEDED(hr))
        {
            // By default prefer high performance
            DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
            if (device_preference == VGPU_DEVICE_PREFERENCE_LOW_POWER) {
                gpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
            }

            for (uint32_t i = 0;
                DXGI_ERROR_NOT_FOUND != dxgiFactory6->EnumAdapterByGpuPreference(i, gpuPreference, IID_PPV_ARGS(&adapter)); i++)
            {
                DXGI_ADAPTER_DESC1 desc;
                VHR(adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    adapter->Release();
                    continue;
                }

                // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
                if (SUCCEEDED(_vgpu_D3D12CreateDevice(adapter, d3d12.min_feature_level, _uuidof(ID3D12Device), nullptr)))
                {
                    break;
                }
            }

            dxgiFactory6->Release();
        }
    }
#endif

    if (!adapter)
    {
        for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != d3d12.factory->EnumAdapters1(i, &adapter); i++)
        {
            DXGI_ADAPTER_DESC1 desc;
            VHR(adapter->GetDesc1(&desc));

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                adapter->Release();

                continue;
            }

            // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
            if (SUCCEEDED(_vgpu_D3D12CreateDevice(adapter, d3d12.min_feature_level, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }


#if !defined(NDEBUG)
    if (!adapter)
    {
        // Try WARP12 instead
        if (FAILED(d3d12.factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter))))
        {
            //LOG_ERROR("WARP12 not available. Enable the 'Graphics Tools' optional feature");
        }

        OutputDebugStringA("Direct3D Adapter - WARP12\n");
    }
#endif

    return adapter;
    }

static bool d3d12_init(const vgpu_config* config) {
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    //
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    if (config->debug)
    {
        ID3D12Debug* debugController;
        if (SUCCEEDED(_vgpu_D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            /*ID3D12Debug1* d3d12Debug1 = nullptr;
            if (SUCCEEDED(debugController->QueryInterface(&d3d12Debug1)))
            {
                d3d12Debug1->SetEnableGPUBasedValidation(true);
                //d3d12Debug1->SetEnableSynchronizedCommandQueueValidation(TRUE);
                d3d12Debug1->Release();
            }*/

            debugController->Release();
        }
        else
        {
            OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
        }

        d3d12.factory_flags = 0;

#if defined(_DEBUG)
        IDXGIInfoQueue* dxgiInfoQueue;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (d3d12.dxgi.DXGIGetDebugInterface1 && SUCCEEDED(d3d12.dxgi.DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
#else
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
#endif
        {
            d3d12.factory_flags = DXGI_CREATE_FACTORY_DEBUG;
            //dxgiInfoQueue->SetMuteDebugOutput(vgpu_DXGI_DEBUG_ALL, FALSE);
            dxgiInfoQueue->SetBreakOnSeverity(vgpu_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
            dxgiInfoQueue->SetBreakOnSeverity(vgpu_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);

            DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
            {
                80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
            };
            DXGI_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            dxgiInfoQueue->AddStorageFilterEntries(vgpu_DXGI_DEBUG_DXGI, &filter);
            dxgiInfoQueue->Release();
    }
#endif
}

    VHR(_vgpu_CreateDXGIFactory2(d3d12.factory_flags, IID_PPV_ARGS(&d3d12.factory)));

    // Determines whether tearing support is available for fullscreen borderless windows.
    {
        d3d12.tearing_support = true;
        BOOL allowTearing = FALSE;

        IDXGIFactory5* dxgiFactory5 = nullptr;
        HRESULT hr = d3d12.factory->QueryInterface(&dxgiFactory5);
        if (SUCCEEDED(hr))
        {
            hr = dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
        }

        if (FAILED(hr) || !allowTearing)
        {
            d3d12.tearing_support = false;
#ifdef _DEBUG
            OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
        }

        SAFE_RELEASE(dxgiFactory5);
    }

    IDXGIAdapter1* dxgi_adapter = d3d12_get_adapter(config->device_preference);

    // Create the DX12 API device object.
    VHR(_vgpu_D3D12CreateDevice(dxgi_adapter, d3d12.min_feature_level, IID_PPV_ARGS(&d3d12.device)));
    d3d12.device->SetName(L"vgpu device");

    // Configure debug device (if active).
    if (config->debug)
    {
        ID3D12InfoQueue* d3dInfoQueue;
        if (SUCCEEDED(d3d12.device->QueryInterface(&d3dInfoQueue)))
        {
#ifdef _DEBUG
            d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif
            D3D12_MESSAGE_ID hide[] =
            {
                D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
                D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
                D3D12_MESSAGE_ID_COPY_DESCRIPTORS_INVALID_RANGES,
                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE
            };
            D3D12_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            d3dInfoQueue->AddStorageFilterEntries(&filter);
            d3dInfoQueue->Release();
        }
    }


    // Create memory allocator
    {
        D3D12MA::ALLOCATOR_DESC desc = {};
        desc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
        desc.pDevice = d3d12.device;
        desc.pAdapter = dxgi_adapter;

        VHR(D3D12MA::CreateAllocator(&desc, &d3d12.allocator));
        switch (d3d12.allocator->GetD3D12Options().ResourceHeapTier)
        {
        case D3D12_RESOURCE_HEAP_TIER_1:
            //LOG_DEBUG("ResourceHeapTier = D3D12_RESOURCE_HEAP_TIER_1");
            break;
        case D3D12_RESOURCE_HEAP_TIER_2:
            //LOG_DEBUG("ResourceHeapTier = D3D12_RESOURCE_HEAP_TIER_2");
            break;
        default:
            break;
        }
    }

    // Create command queue's
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.NodeMask = 0;

        VHR(d3d12.device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&d3d12.direct_command_queue)));
        d3d12.direct_command_queue->SetName(L"Direct Command Queue");
    }

    if (config->swapchain.native_handle) {
        uint32_t factory_caps = DXGIFACTORY_CAPS_FLIP_PRESENT;
        if (d3d12.tearing_support) {
            factory_caps |= DXGIFACTORY_CAPS_TEARING;
        }

        IDXGISwapChain* tempSwapChain = vgpu_d3d_create_swapchain(
            d3d12.factory,
            factory_caps,
            d3d12.direct_command_queue,
            config->swapchain.native_handle,
            config->swapchain.width, config->swapchain.height,
            config->swapchain.color_format,
            2u,
            config->swapchain.is_fullscreen
        );
        VHR(tempSwapChain->QueryInterface(IID_PPV_ARGS(&d3d12.swapchain)));
        SAFE_RELEASE(tempSwapChain);

        d3d12.backbuffer_index = d3d12.swapchain->GetCurrentBackBufferIndex();
    }

    SAFE_RELEASE(dxgi_adapter);

    // Create a fence for tracking GPU execution progress.
    {
        d3d12.render_latency = 2u;
        d3d12.frame_index = 0;
        d3d12.frame_number = 0;
        VHR(d3d12.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d3d12.frame_fence)));
        d3d12.frame_fence->SetName(L"Frame Fence");

        d3d12.frame_fence_event = CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
        if (!d3d12.frame_fence_event)
        {
            //LOG_ERROR("Direct3D12: CreateEventEx failed.");
        }
    }

    // Create command allocators and command list
    {
        for (uint64_t i = 0; i < d3d12.render_latency; ++i)
        {
            VHR(d3d12.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&d3d12.commandAllocators[i])));
        }

        VHR(d3d12.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, d3d12.commandAllocators[0], nullptr, IID_PPV_ARGS(&d3d12.command_list)));
        VHR(d3d12.command_list->Close());
        d3d12.command_list->SetName(L"Frame Command List");
    }

    return true;
}

static void d3d12_shutdown(void) {
    SAFE_RELEASE(d3d12.direct_command_queue);
    CloseHandle(d3d12.frame_fence_event);
    SAFE_RELEASE(d3d12.frame_fence);
    /*for (uint32_t i = 0; i < backBufferCount; i++)
    {
        SAFE_RELEASE(swapChainRenderTargets[i]);
    }*/

    SAFE_RELEASE(d3d12.swapchain);

    for (uint64_t i = 0; i < d3d12.render_latency; ++i)
    {
        SAFE_RELEASE(d3d12.commandAllocators[i]);
    }
    SAFE_RELEASE(d3d12.command_list);

    // Allocator
    D3D12MA::Stats stats;
    d3d12.allocator->CalculateStats(&stats);

    if (stats.Total.UsedBytes > 0) {
        //LOG_ERROR("Total device memory leaked: %llu bytes.", stats.Total.UsedBytes);
    }

    SAFE_RELEASE(d3d12.allocator);

    ULONG refCount = d3d12.device->Release();
#if !defined(NDEBUG)
    if (refCount > 0)
    {
        //LOG_DEBUG("Direct3D12: There are %d unreleased references left on the device", refCount);

        ID3D12DebugDevice* debugDevice;
        if (SUCCEEDED(d3d12.device->QueryInterface(&debugDevice)))
        {
            debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_SUMMARY | D3D12_RLDO_IGNORE_INTERNAL);
            debugDevice->Release();
        }
    }
#else
    (void)refCount; // avoid warning
#endif

    // Release factory at last.
    SAFE_RELEASE(d3d12.factory);

#ifdef _DEBUG
    {
        IDXGIDebug1* dxgiDebug;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (d3d12.dxgi.DXGIGetDebugInterface1 && SUCCEEDED(d3d12.dxgi.DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
#else
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
#endif
        {
            dxgiDebug->ReportLiveObjects(vgpu_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            dxgiDebug->Release();
    }
}
#endif

    memset(&d3d12, 0, sizeof(d3d12));
}

static void d3d12_begin_frame(void) {
    // Prepare the command buffers to be used for the current frame.
    VHR(d3d12.commandAllocators[d3d12.frame_index]->Reset());
    VHR(d3d12.command_list->Reset(d3d12.commandAllocators[d3d12.frame_index], nullptr));
    //d3d12.command_list->SetDescriptorHeaps(1, &d3d12->CbvSrvUavGpuHeaps[d3d12.frame_index].Heap);
}

static void d3d12_end_frame(void) {
    VHR(d3d12.command_list->Close());

    // TODO: Finish upload
    //EndFrame_Upload();

    ID3D12CommandList* commandLists[] = { d3d12.command_list };
    d3d12.direct_command_queue->ExecuteCommandLists(_countof(commandLists), commandLists);

    // Present the frame.
    if (d3d12.swapchain) {
        const uint32_t sync_interval = 1u;
        VHR(d3d12.swapchain->Present(sync_interval, sync_interval == 0 ? DXGI_PRESENT_ALLOW_TEARING : 0));
        d3d12.backbuffer_index = d3d12.swapchain->GetCurrentBackBufferIndex();
    }

    // Signal the fence with the current frame number, so that we can check back on it
    VHR(d3d12.direct_command_queue->Signal(d3d12.frame_fence, ++d3d12.frame_number));

    // Wait for the GPU to catch up before we stomp an executing command buffer
    uint64_t GPUFrameCount = d3d12.frame_fence->GetCompletedValue();

    if ((d3d12.frame_number - GPUFrameCount) >= d3d12.render_latency)
    {
        VHR(d3d12.frame_fence->SetEventOnCompletion(GPUFrameCount + 1, d3d12.frame_fence_event));
        WaitForSingleObject(d3d12.frame_fence_event, INFINITE);
    }

    d3d12_execute_deferred_releases();
    d3d12.frame_index = d3d12.frame_number % d3d12.render_latency;
}

/* Driver */
static bool d3d12_is_supported(void) {
    if (d3d12.available_initialized) {
        return d3d12.available;
    }

    d3d12.available_initialized = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
    d3d12.dxgi.instance = LoadLibraryA("dxgi.dll");
    if (!d3d12.dxgi.instance) {
        return false;
    }

    d3d12.dxgi.CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(d3d12.dxgi.instance, "CreateDXGIFactory2");
    if (!d3d12.dxgi.CreateDXGIFactory2)
    {
        return false;
    }
    d3d12.dxgi.DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(d3d12.dxgi.instance, "DXGIGetDebugInterface1");

    d3d12.instance = LoadLibraryA("d3d12.dll");
    if (!d3d12.instance) {
        return false;
    }

    d3d12.D3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(d3d12.instance, "D3D12CreateDevice");
    d3d12.D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(d3d12.instance, "D3D12GetDebugInterface");
    if (d3d12.D3D12CreateDevice == nullptr) {
        return false;
    }

    d3d12.D3D12SerializeRootSignature = (PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)GetProcAddress(d3d12.instance, "D3D12SerializeRootSignature");
    d3d12.D3D12CreateRootSignatureDeserializer = (PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(d3d12.instance, "D3D12CreateRootSignatureDeserializer");
    d3d12.D3D12SerializeVersionedRootSignature = (PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE)GetProcAddress(d3d12.instance, "D3D12SerializeVersionedRootSignature");
    d3d12.D3D12CreateVersionedRootSignatureDeserializer = (PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(d3d12.instance, "D3D12CreateVersionedRootSignatureDeserializer");
#endif

    HRESULT hr = _vgpu_D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);

    if (FAILED(hr))
    {
        return false;
    }

    d3d12.available = true;
    return true;
};

static vgpu_renderer* d3d12_init_renderer(void) {
    static vgpu_renderer renderer = { nullptr };
    renderer.init = d3d12_init;
    renderer.shutdown = d3d12_shutdown;
    renderer.begin_frame = d3d12_begin_frame;
    renderer.end_frame = d3d12_end_frame;
    return &renderer;
}

vgpu_driver d3d12_driver = {
    VGPU_BACKEND_TYPE_D3D12,
    d3d12_is_supported,
    d3d12_init_renderer
};

#endif /* defined(VGPU_DRIVER_D3D12) */
