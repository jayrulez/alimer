//
// Copyright (c) 2019-2020 Amer Koleci.
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
#define CINTERFACE
#define COBJMACROS
#include <d3d12.h>
#include "vgpu_d3d_common.h"

/* D3D12 guids */
static const IID D3D12_IID_ID3D12Device = { 0x189819f1,0x1db6,0x4b57, {0xbe,0x54,0x18,0x21,0x33,0x9b,0x85,0xf7 } };
static const IID D3D12_IID_ID3D12Fence = { 0x0a753dcf, 0xc4d8, 0x4b91, {0xad, 0xf6, 0xbe, 0x5a, 0x60, 0xd9, 0x5a, 0x76 } };
static const IID D3D12_IID_ID3D12CommandQueue = { 0x0ec870a6, 0x5d7e, 0x4c22, {0x8c, 0xfc, 0x5b, 0xaa, 0xe0, 0x76, 0x16, 0xed} };
static const IID D3D12_IID_ID3D12CommandAllocator = { 0x6102dee4, 0xaf59, 0x4b09, {0xb9, 0x99, 0xb4, 0x4d, 0x73, 0xf0, 0x9b, 0x24} };
static const IID D3D12_IID_ID3D12GraphicsCommandList = { 0x5b160d0f, 0xac1b, 0x4185, {0x8b, 0xa8, 0xb3, 0xae, 0x42, 0xa5, 0xa4, 0x55} };
static const IID D3D12_IID_ID3D12GraphicsCommandList4 = { 0x8754318e, 0xd3a9, 0x4541, {0x98, 0xcf, 0x64, 0x5b, 0x50, 0xdc, 0x48, 0x74} };

#if !defined( D3D12_IGNORE_SDK_LAYERS )
static const IID D3D12_IID_ID3D12Debug = { 0x344488b7, 0x6846, 0x474b, {0xb9, 0x89, 0xf0, 0x27, 0x44, 0x82, 0x45, 0xe0} };
static const IID D3D12_IID_ID3D12Debug1 = { 0xaffaa4ca, 0x63fe, 0x4d8e, {0xb8, 0xad, 0x15, 0x90, 0x00, 0xaf, 0x43, 0x04} };
static const IID D3D12_IID_ID3D12DebugDevice = { 0x3febd6dd,0x4973,0x4787, {0x81,0x94,0xe4,0x5f,0x9e,0x28,0x92,0x3e } };
static const IID D3D12_IID_ID3D12InfoQueue = { 0x0742a90b, 0xc387, 0x483f, {0xb9, 0x46, 0x30, 0xa7, 0xe4, 0xe6, 0x14, 0x58 } };
#endif 

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
#endif
} d3d12;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#   define vgpuCreateDXGIFactory2 d3d12.dxgi.CreateDXGIFactory2
#   define vgpuDXGIGetDebugInterface1 d3d12.dxgi.DXGIGetDebugInterface1
#   define vgpuD3D12CreateDevice d3d12.D3D12CreateDevice
#   define vgpuD3D12GetDebugInterface d3d12.D3D12GetDebugInterface
#else
#   define vgpuCreateDXGIFactory2 CreateDXGIFactory2
#   define vgpuDXGIGetDebugInterface1 DXGIGetDebugInterface1
#   define vgpuD3D12CreateDevice D3D12CreateDevice
#   define vgpuD3D12GetDebugInterface D3D12GetDebugInterface
#endif

#ifdef _DEBUG
static const IID D3D11_IID_ID3D11Debug = { 0x79cf2233, 0x7536, 0x4948, {0x9d, 0x36, 0x1e, 0x46, 0x92, 0xdc, 0x57, 0x60} };
static const IID D3D11_IID_ID3D11InfoQueue = { 0x6543dbb6, 0x1b48, 0x42f5, {0xab,0x82,0xe9,0x7e,0xc7,0x43,0x26,0xf6} };
#endif

typedef struct d3d12_gpu_frame {
    ID3D12CommandAllocator* allocator;
} d3d12_gpu_frame;

typedef struct d3d12_renderer {
    D3D_FEATURE_LEVEL min_feature_level;
    uint32_t max_inflight_frames;
    DWORD factoryFlags;
    IDXGIFactory4* factory;
    uint32_t factory_caps;

    ID3D12Device* device;
    D3D_FEATURE_LEVEL feature_level;

    /* Queues */
    ID3D12CommandQueue* direct_command_queue;
    ID3D12CommandQueue* compute_command_queue;

    /* Frame data. */
    uint64_t frame_index;
    uint64_t frame_count;

    ID3D12GraphicsCommandList* command_list;
    ID3D12GraphicsCommandList4* command_list4;
    ID3D12Fence* frame_fence;
    HANDLE frame_fence_event;
    d3d12_gpu_frame frames[3];

    uint32_t sync_interval;
    uint32_t present_flags;
    IDXGISwapChain3* swapchain;
    uint32_t backbuffer_index;
} d3d12_renderer;

/* Device functions */
static bool d3d12_create_factory(d3d12_renderer* renderer, bool debug)
{
    if (renderer->factory) {
        IDXGIFactory4_Release(renderer->factory);
        renderer->factory = NULL;
    }

    HRESULT hr = S_OK;

    if (debug)
    {
        ID3D12Debug* d3d12Debug;
        if (SUCCEEDED(vgpuD3D12GetDebugInterface(&D3D12_IID_ID3D12Debug, (void**)&d3d12Debug)))
        {
            ID3D12Debug_EnableDebugLayer(d3d12Debug);

            ID3D12Debug1* d3d12Debug1;
            if (SUCCEEDED(ID3D12Debug_QueryInterface(d3d12Debug, &D3D12_IID_ID3D12Debug1, (void**)&d3d12Debug1)))
            {
                ID3D12Debug1_SetEnableGPUBasedValidation(d3d12Debug1, TRUE);
                //ID3D12Debug1_SetEnableSynchronizedCommandQueueValidation(d3d12Debug1, TRUE);
                ID3D12Debug1_Release(d3d12Debug1);
            }

            ID3D12Debug_Release(d3d12Debug);
        }
        else
        {
            OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
        }
    }

    renderer->factoryFlags = 0;

#if defined(_DEBUG)
    if (debug)
    {
        IDXGIInfoQueue* dxgiInfoQueue;
        if (SUCCEEDED(vgpuDXGIGetDebugInterface1(0, &D3D_IID_IDXGIInfoQueue, (void**)&dxgiInfoQueue)))
        {
            renderer->factoryFlags = DXGI_CREATE_FACTORY_DEBUG;

            VHR(IDXGIInfoQueue_SetBreakOnSeverity(dxgiInfoQueue, D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE));
            VHR(IDXGIInfoQueue_SetBreakOnSeverity(dxgiInfoQueue, D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE));
            VHR(IDXGIInfoQueue_SetBreakOnSeverity(dxgiInfoQueue, D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, FALSE));

            DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
            {
                80 // IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides.
            };

            DXGI_INFO_QUEUE_FILTER filter;
            memset(&filter, 0, sizeof(filter));
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            IDXGIInfoQueue_AddStorageFilterEntries(dxgiInfoQueue, D3D_DXGI_DEBUG_DXGI, &filter);
            IDXGIInfoQueue_Release(dxgiInfoQueue);
        }
    }

#endif

    hr = vgpuCreateDXGIFactory2(renderer->factoryFlags, &D3D_IID_IDXGIFactory4, (void**)&renderer->factory);
    if (FAILED(hr)) {
        return false;
    }

    renderer->factory_caps = DXGIFACTORY_CAPS_FLIP_PRESENT;

    // Check tearing support.
    {
        BOOL allowTearing = FALSE;
        IDXGIFactory5* factory5;
        HRESULT hr = IDXGIFactory4_QueryInterface(renderer->factory, &D3D_IID_IDXGIFactory5, (void**)&factory5);
        if (SUCCEEDED(hr))
        {
            hr = IDXGIFactory5_CheckFeatureSupport(factory5, DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
            IDXGIFactory5_Release(factory5);
        }

        if (FAILED(hr) || !allowTearing)
        {
#ifdef _DEBUG
            OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
        }
        else
        {
            renderer->factory_caps |= DXGIFACTORY_CAPS_TEARING;
        }
    }

    return true;
}

static void d3d12_waitForGPU(vgpu_renderer driver_data) {
    d3d12_renderer* renderer = (d3d12_renderer*)driver_data;
    VHR(ID3D12CommandQueue_Signal(renderer->direct_command_queue, renderer->frame_fence, ++renderer->frame_count));
    VHR(ID3D12Fence_SetEventOnCompletion(renderer->frame_fence, renderer->frame_count, renderer->frame_fence_event));
    WaitForSingleObject(renderer->frame_fence_event, INFINITE);

    //GPUDescriptorHeaps[renderer->frame_index].size = 0;
    //GPUUploadMemoryHeaps[renderer->frame_index].size = 0;
}

static void d3d12_destroy(vgpu_device device)
{
    d3d12_waitForGPU(device->renderer);
    d3d12_renderer* renderer = (d3d12_renderer*)device->renderer;

    /* Destroy frame data */
    renderer->frame_index = 0;
    renderer->frame_count = 0;
    for (uint32_t i = 0; i < renderer->max_inflight_frames; ++i)
    {
        ID3D12CommandAllocator_Release(renderer->frames[i].allocator);
    }

    if (renderer->command_list4) {
        ID3D12GraphicsCommandList4_Release(renderer->command_list4);
    }
    ID3D12GraphicsCommandList_Release(renderer->command_list);
    CloseHandle(renderer->frame_fence_event);
    ID3D12Fence_Release(renderer->frame_fence);

    /* Destroy command queues */
    ID3D12CommandQueue_Release(renderer->compute_command_queue);
    ID3D12CommandQueue_Release(renderer->direct_command_queue);

    if (renderer->swapchain) {
        IDXGISwapChain1_Release(renderer->swapchain);
    }

    ULONG ref_count = ID3D12Device_Release(renderer->device);
#if !defined(NDEBUG)
    if (ref_count > 0)
    {
        //gpuLog(GPULogLevel_Error, "Direct3D12: There are %d unreleased references left on the device", ref_count);

        ID3D12DebugDevice* d3d_debug = NULL;
        if (SUCCEEDED(ID3D12Device_QueryInterface(renderer->device, &D3D12_IID_ID3D12DebugDevice, (void**)&d3d_debug)))
        {
            ID3D12DebugDevice_ReportLiveDeviceObjects(d3d_debug, D3D12_RLDO_DETAIL);
            ID3D12DebugDevice_Release(d3d_debug);
        }
    }
#else
    (void)ref_count; // avoid warning
#endif

    VGPU_FREE(renderer);
    VGPU_FREE(device);
}

static void d3d12_init_command_list(d3d12_renderer* renderer) {
    VHR(ID3D12CommandAllocator_Reset(renderer->frames[renderer->frame_index].allocator));
    VHR(ID3D12GraphicsCommandList_Reset(renderer->command_list, renderer->frames[renderer->frame_index].allocator, NULL));
    //ID3D12GraphicsCommandList_SetDescriptorHeaps(renderer->command_list, 1, &Gfx->GPUDescriptorHeaps[Gfx->FrameIndex].Heap);
}

static void d3d12_begin_frame(vgpu_renderer driver_data) {
}

static void d3d12_present_frame(vgpu_renderer driver_data) {
    d3d12_renderer* renderer = (d3d12_renderer*)driver_data;

    VHR(ID3D12GraphicsCommandList_Close(renderer->command_list));

    /* TODO: Handle upload + compute logic here */
    //TODO_Upload();

    ID3D12CommandList* command_lists[] = { (ID3D12CommandList*)renderer->command_list };
    ID3D12CommandQueue_ExecuteCommandLists(renderer->direct_command_queue, _countof(command_lists), command_lists);

    HRESULT hr = S_OK;
    if (renderer->swapchain) {
        hr = IDXGISwapChain3_Present(renderer->swapchain, renderer->sync_interval, renderer->present_flags);
        renderer->backbuffer_index = IDXGISwapChain3_GetCurrentBackBufferIndex(renderer->swapchain);
    }

    // If the device was reset we must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        /* TODO: Handle device lost. */
        return;
    }

    VHR(ID3D12CommandQueue_Signal(renderer->direct_command_queue, renderer->frame_fence, ++renderer->frame_count));

    uint64_t GPUFrameCount = ID3D12Fence_GetCompletedValue(renderer->frame_fence);
    if ((renderer->frame_count - GPUFrameCount) >= renderer->max_inflight_frames)
    {
        VHR(ID3D12Fence_SetEventOnCompletion(renderer->frame_fence, GPUFrameCount + 1, renderer->frame_fence_event));
        WaitForSingleObject(renderer->frame_fence_event, INFINITE);
    }

    renderer->frame_index = renderer->frame_count % renderer->max_inflight_frames;

    // Prepare the command buffers to be used for the next frame
    d3d12_init_command_list(renderer);
}

/* Driver functions */
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
    if (!d3d12.D3D12CreateDevice) {
        return false;
    }
    d3d12.D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(d3d12.instance, "D3D12GetDebugInterface");
#endif

    HRESULT hr = vgpuD3D12CreateDevice(
        NULL,
        D3D_FEATURE_LEVEL_11_0,
        &D3D12_IID_ID3D12Device,
        NULL
    );

    if (FAILED(hr))
    {
        return false;
    }

    d3d12.available = true;
    return true;
}

static IDXGIAdapter1* d3d12_get_adapter(IDXGIFactory4* factory, D3D_FEATURE_LEVEL min_feature_level, bool low_power)
{
    /* Detect adapter now. */
    IDXGIAdapter1* adapter = NULL;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
    IDXGIFactory6* factory6;
    HRESULT hr = IDXGIFactory4_QueryInterface(factory, &D3D_IID_IDXGIFactory6, (void**)&factory6);
    if (SUCCEEDED(hr))
    {
        // By default prefer high performance
        DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
        if (low_power) {
            gpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
        }

        for (uint32_t i = 0;
            DXGI_ERROR_NOT_FOUND != IDXGIFactory6_EnumAdapterByGpuPreference(factory6, i, gpuPreference, &D3D_IID_IDXGIAdapter1, (void**)&adapter); i++)
        {
            DXGI_ADAPTER_DESC1 desc;
            VHR(IDXGIAdapter1_GetDesc1(adapter, &desc));

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                IDXGIAdapter1_Release(adapter);
                continue;
            }

            if (SUCCEEDED(vgpuD3D12CreateDevice((IUnknown*)adapter, min_feature_level, &D3D12_IID_ID3D12Device, NULL)))
            {
                break;
            }
        }

        IDXGIFactory6_Release(factory6);
    }
#endif

    if (!adapter)
    {
        for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != IDXGIFactory4_EnumAdapters1(factory, i, &adapter); i++)
        {
            DXGI_ADAPTER_DESC1 desc;
            VHR(IDXGIAdapter1_GetDesc1(adapter, &desc));

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                IDXGIAdapter1_Release(adapter);

                continue;
            }

            if (SUCCEEDED(vgpuD3D12CreateDevice((IUnknown*)adapter, min_feature_level, &D3D12_IID_ID3D12Device, NULL)))
            {
                break;
            }
        }
    }
#if !defined(NDEBUG)
    if (!adapter)
    {
        // Try WARP12 instead
        if (FAILED(IDXGIFactory4_EnumWarpAdapter(factory, &D3D_IID_IDXGIAdapter1, (void**)&adapter)))
        {
            //throw std::exception("WARP12 not available. Enable the 'Graphics Tools' optional feature");
        }

        OutputDebugStringA("Direct3D Adapter - WARP12\n");
    }
#endif

    return adapter;
}

static vgpu_device d3d12_create_device(const vgpu_device_desc* desc) {
    vgpu_device_t* result;
    d3d12_renderer* renderer;

    /* Allocate and zero out the renderer */
    renderer = VGPU_ALLOC(d3d12_renderer);
    memset(renderer, 0, sizeof(d3d12_renderer));

    renderer->min_feature_level = D3D_FEATURE_LEVEL_11_0;
    renderer->max_inflight_frames = 2u;

    if (!d3d12_create_factory(renderer, desc->debug)) {
        return NULL;
    }

    IDXGIAdapter1* adapter = d3d12_get_adapter(renderer->factory, renderer->min_feature_level, false);

    /* Create d3d12 device */
    {
        // Create the DX12 API device object.
        VHR(vgpuD3D12CreateDevice(
            (IUnknown*)adapter,
            renderer->min_feature_level,
            &D3D12_IID_ID3D12Device,
            (void**)&renderer->device
        ));

#ifndef NDEBUG
        ID3D12InfoQueue* d3d_info_queue;
        if (SUCCEEDED(ID3D12Device_QueryInterface(renderer->device, &D3D12_IID_ID3D12InfoQueue, (void**)&d3d_info_queue)))
        {
#ifdef _DEBUG
            ID3D12InfoQueue_SetBreakOnSeverity(d3d_info_queue, D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            ID3D12InfoQueue_SetBreakOnSeverity(d3d_info_queue, D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif

            D3D12_MESSAGE_ID hide[] =
            {
                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE
            };

            D3D12_INFO_QUEUE_FILTER filter;
            memset(&filter, 0, sizeof(D3D12_INFO_QUEUE_FILTER));
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            ID3D12InfoQueue_AddStorageFilterEntries(d3d_info_queue, &filter);
            ID3D12InfoQueue_Release(d3d_info_queue);
        }
#endif

        // Determine maximum supported feature level for this device
        static const D3D_FEATURE_LEVEL s_featureLevels[] =
        {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
        };

        D3D12_FEATURE_DATA_FEATURE_LEVELS featLevels =
        {
            _countof(s_featureLevels), s_featureLevels, D3D_FEATURE_LEVEL_11_0
        };

        HRESULT hr = ID3D12Device_CheckFeatureSupport(renderer->device, D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
        if (SUCCEEDED(hr))
        {
            renderer->feature_level = featLevels.MaxSupportedFeatureLevel;
        }
        else
        {
            renderer->feature_level = renderer->min_feature_level;
        }
    }

    IDXGIAdapter1_Release(adapter);

    /* Create command queue's */
    const D3D12_COMMAND_QUEUE_DESC directQueueDesc = {
        .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
        .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
        .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
        .NodeMask = 0x0
    };

    const D3D12_COMMAND_QUEUE_DESC computeQueueDesc = {
        .Type = D3D12_COMMAND_LIST_TYPE_COMPUTE,
        .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
        .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
        .NodeMask = 0x0
    };

    VHR(ID3D12Device_CreateCommandQueue(renderer->device, &directQueueDesc, &D3D12_IID_ID3D12CommandQueue, (void**)&renderer->direct_command_queue));
    VHR(ID3D12Device_CreateCommandQueue(renderer->device, &computeQueueDesc, &D3D12_IID_ID3D12CommandQueue, (void**)&renderer->compute_command_queue));
    VHR(ID3D12CommandQueue_SetName(renderer->direct_command_queue, L"Direct Command Queue"));
    VHR(ID3D12CommandQueue_SetName(renderer->compute_command_queue, L"Compute  Command Queue"));

    /* Create frame data. */
    renderer->frame_index = 0;
    renderer->frame_count = 0;
    for (uint32_t i = 0; i < renderer->max_inflight_frames; ++i)
    {
        VHR(ID3D12Device_CreateCommandAllocator(
            renderer->device,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            &D3D12_IID_ID3D12CommandAllocator,
            (void**)&renderer->frames[i].allocator
        ));
    }

    VHR(ID3D12Device_CreateCommandList(
        renderer->device,
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        renderer->frames[0].allocator,
        NULL,
        &D3D12_IID_ID3D12GraphicsCommandList,
        (void**)&renderer->command_list)
    );
    VHR(ID3D12GraphicsCommandList_Close(renderer->command_list));

    VHR(ID3D12Device_CreateFence(renderer->device, 0, D3D12_FENCE_FLAG_NONE, &D3D12_IID_ID3D12Fence, (void**)&renderer->frame_fence));
    renderer->frame_fence_event = CreateEventEx(NULL, NULL, 0, EVENT_ALL_ACCESS);

    /* Create swap chain if required. */
    if (desc->swapchain.window_handle) {
        renderer->sync_interval = vgpu_d3d_get_sync_interval(desc->swapchain.present_interval);
        renderer->present_flags = 0;

        if (desc->swapchain.present_interval == VGPU_PRESENT_INTERVAL_IMMEDIATE
            && renderer->factory_caps & DXGIFACTORY_CAPS_TEARING) {
            renderer->present_flags |= DXGI_PRESENT_ALLOW_TEARING;
        }

        IDXGISwapChain1* temp_swap_chain = vgpu_d3d_create_swapchain(
            (IDXGIFactory2*)renderer->factory,
            (IUnknown*)renderer->direct_command_queue,
            renderer->factory_caps,
            desc->swapchain.window_handle,
            desc->swapchain.width, desc->swapchain.height,
            renderer->max_inflight_frames /* 3 for triple buffering */
        );

        VHR(IDXGISwapChain1_QueryInterface(temp_swap_chain, &D3D_IID_IDXGISwapChain3, (void**)&renderer->swapchain));
        IDXGISwapChain1_Release(temp_swap_chain);

        renderer->backbuffer_index = IDXGISwapChain3_GetCurrentBackBufferIndex(renderer->swapchain);
    }

    d3d12_init_command_list(renderer);

    /* Create and return the gpu_device */
    result = VGPU_ALLOC(vgpu_device_t);
    result->renderer = (vgpu_renderer)renderer;
    ASSIGN_DRIVER(d3d12);
    return result;
}

vgpu_driver d3d12_driver = {
    VGPU_BACKEND_TYPE_D3D12,
    d3d12_is_supported,
    d3d12_create_device
};

#endif /* defined(VGPU_DRIVER_VULKAN) */