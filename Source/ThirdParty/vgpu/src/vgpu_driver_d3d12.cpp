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
#include <d3d12.h>
#include "vgpu_d3d_common.h"

typedef struct d3d12_descriptor_heap {
    ID3D12DescriptorHeap* handle;
    D3D12_CPU_DESCRIPTOR_HANDLE CPUStart;
    D3D12_GPU_DESCRIPTOR_HANDLE GPUStart;
    uint32_t size;
    uint32_t capacity;
    uint32_t descriptor_size;
} d3d12_descriptor_heap;

typedef struct d3d12_gpu_frame {
    ID3D12CommandAllocator* allocator;
    d3d12_descriptor_heap GPUHeap;
} d3d12_gpu_frame;

typedef struct D3D12BackendContext {
    uint32_t syncInterval;
    uint32_t presentFlags;
    IDXGISwapChain3* swapchain;
    uint32_t backbufferIndex;

    /* Frame data. */
    uint64_t frameIndex;
    uint64_t frameCount;
    ID3D12GraphicsCommandList* commandList;
    ID3D12GraphicsCommandList4* commandList4;
    ID3D12Fence* fence;
    HANDLE fenceEvent;
    d3d12_gpu_frame frames[3];
} D3D12BackendContext;

struct D3D12Texture {
    ID3D12Resource* handle;
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
#endif


    D3D_FEATURE_LEVEL min_feature_level;
    uint32_t max_inflight_frames;
    uint32_t backbuffer_count;
    DWORD factoryFlags;
    IDXGIFactory4* factory;
    uint32_t factory_caps;

    ID3D12Device* d3dDevice;
    D3D_FEATURE_LEVEL d3dFeatureLevel;

    /* Queues */
    ID3D12CommandQueue* direct_command_queue;
    ID3D12CommandQueue* compute_command_queue;

    /* Descriptor heaps */
    d3d12_descriptor_heap RTVHeap;
    d3d12_descriptor_heap DSVHeap;
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

/* Device functions */
static bool d3d12_createFactory(bool debug)
{
    SAFE_RELEASE(d3d12.factory);

    HRESULT hr = S_OK;

    if (debug)
    {
        ID3D12Debug* d3d12Debug;
        if (SUCCEEDED(vgpuD3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&d3d12Debug)))
        {
            d3d12Debug->EnableDebugLayer();

            ID3D12Debug1* d3d12Debug1;
            if (SUCCEEDED(d3d12Debug->QueryInterface(__uuidof(ID3D12Debug1), (void**)&d3d12Debug1)))
            {
                d3d12Debug1->SetEnableGPUBasedValidation(TRUE);
                //d3d12Debug1->SetEnableSynchronizedCommandQueueValidation(TRUE);
                d3d12Debug1->Release();
            }

            d3d12Debug->Release();
        }
        else
        {
            OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
        }
    }

    d3d12.factoryFlags = 0;

#if defined(_DEBUG)
    if (debug)
    {
        IDXGIInfoQueue* dxgiInfoQueue;
        if (SUCCEEDED(vgpuDXGIGetDebugInterface1(0, __uuidof(IDXGIInfoQueue), (void**)&dxgiInfoQueue)))
        {
            d3d12.factoryFlags = DXGI_CREATE_FACTORY_DEBUG;

            VHR(dxgiInfoQueue->SetBreakOnSeverity(D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE));
            VHR(dxgiInfoQueue->SetBreakOnSeverity(D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE));
            VHR(dxgiInfoQueue->SetBreakOnSeverity(D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, FALSE));

            DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
            {
                80 // IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides.
            };

            DXGI_INFO_QUEUE_FILTER filter;
            memset(&filter, 0, sizeof(filter));
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            dxgiInfoQueue->AddStorageFilterEntries(D3D_DXGI_DEBUG_DXGI, &filter);
            dxgiInfoQueue->Release();
        }
    }

#endif

    hr = vgpuCreateDXGIFactory2(d3d12.factoryFlags, __uuidof(IDXGIFactory4), (void**)&d3d12.factory);
    if (FAILED(hr)) {
        return false;
    }

    d3d12.factory_caps = DXGIFACTORY_CAPS_FLIP_PRESENT;

    // Check tearing support.
    {
        BOOL allowTearing = FALSE;
        IDXGIFactory5* factory5;
        HRESULT hr = d3d12.factory->QueryInterface(__uuidof(IDXGIFactory5), (void**)&factory5);
        if (SUCCEEDED(hr))
        {
            hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
            factory5->Release();
        }

        if (FAILED(hr) || !allowTearing)
        {
#ifdef _DEBUG
            OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
        }
        else
        {
            d3d12.factory_caps |= DXGIFACTORY_CAPS_TEARING;
        }
    }

    return true;
}

static IDXGIAdapter1* d3d12_getAdapter(IDXGIFactory4* factory, D3D_FEATURE_LEVEL min_feature_level, bool low_power)
{
    /* Detect adapter now. */
    IDXGIAdapter1* adapter = NULL;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
    IDXGIFactory6* factory6;
    HRESULT hr = factory->QueryInterface(__uuidof(IDXGIFactory6), (void**)&factory6);
    if (SUCCEEDED(hr))
    {
        // By default prefer high performance
        DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
        if (low_power) {
            gpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
        }

        for (uint32_t i = 0;
            DXGI_ERROR_NOT_FOUND != factory6->EnumAdapterByGpuPreference(i, gpuPreference, IID_PPV_ARGS(&adapter)); i++)
        {
            DXGI_ADAPTER_DESC1 desc;
            VHR(adapter->GetDesc1(&desc));

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                adapter->Release();
                continue;
            }

            if (SUCCEEDED(vgpuD3D12CreateDevice(adapter, min_feature_level, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }

        factory6->Release();
    }
#endif

    if (!adapter)
    {
        for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(i, &adapter); i++)
        {
            DXGI_ADAPTER_DESC1 desc;
            VHR(adapter->GetDesc1(&desc));

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                adapter->Release();

                continue;
            }

            if (SUCCEEDED(vgpuD3D12CreateDevice(adapter, min_feature_level, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }
#if !defined(NDEBUG)
    if (!adapter)
    {
        // Try WARP12 instead
        if (FAILED(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter))))
        {
            //throw std::exception("WARP12 not available. Enable the 'Graphics Tools' optional feature");
        }

        OutputDebugStringA("Direct3D Adapter - WARP12\n");
    }
#endif

    return adapter;
}

static void d3d12_CreateDescriptorHeap(d3d12_descriptor_heap* descriptor_heap, uint32_t capacity, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shader_visible) {

    descriptor_heap->size = 0;
    descriptor_heap->capacity = capacity;
    descriptor_heap->descriptor_size = d3d12.d3dDevice->GetDescriptorHandleIncrementSize(type);

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = type;
    heapDesc.NumDescriptors = capacity;
    heapDesc.Flags = shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.NodeMask = 0x0;

    VHR(d3d12.d3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptor_heap->handle)));

    descriptor_heap->CPUStart = descriptor_heap->handle->GetCPUDescriptorHandleForHeapStart();
    if (shader_visible) {
        descriptor_heap->GPUStart = descriptor_heap->handle->GetGPUDescriptorHandleForHeapStart();
    }
    else {
        descriptor_heap->GPUStart.ptr = 0;
    }
}

static d3d12_descriptor_heap* d3d12_GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, bool shader_visible)
{
    if (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
    {
        return &d3d12.RTVHeap;
    }
    else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
    {
        return &d3d12.DSVHeap;
    }
    else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    {
        //if (shader_visible) {
        //    return &renderer->frames[renderer->frame_index].GPUHeap;
        //}

        /* TODO: Should we support non shader visible CPU heap? */
        //return &renderer->CPUHeap;
    }

    VGPU_UNREACHABLE();
    return NULL;
}

static D3D12_CPU_DESCRIPTOR_HANDLE d3d12_AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count)
{
    d3d12_descriptor_heap* heap = d3d12_GetDescriptorHeap(type, false);
    VGPU_ASSERT((heap->size + count) < heap->capacity);

    D3D12_CPU_DESCRIPTOR_HANDLE handle;
    handle.ptr = heap->CPUStart.ptr + (size_t)heap->size * heap->descriptor_size;
    heap->size += count;

    return handle;
}


static bool d3d12_init(const VGPUDeviceDescription* desc)
{
    d3d12.min_feature_level = D3D_FEATURE_LEVEL_11_0;
    d3d12.max_inflight_frames = 2u;
    d3d12.backbuffer_count = 2u;

    if (!d3d12_createFactory(desc->debug)) {
        return false;
    }

    IDXGIAdapter1* adapter = d3d12_getAdapter(d3d12.factory, d3d12.min_feature_level, false);

    /* Create d3d12 device */
    {
        // Create the DX12 API device object.
        VHR(vgpuD3D12CreateDevice(adapter, d3d12.min_feature_level, IID_PPV_ARGS(&d3d12.d3dDevice)));

#ifndef NDEBUG
        ID3D12InfoQueue* d3dInfoQueue;
        if (SUCCEEDED(d3d12.d3dDevice->QueryInterface(IID_PPV_ARGS(&d3dInfoQueue))))
        {
#ifdef _DEBUG
            d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif

            D3D12_MESSAGE_ID hide[] =
            {
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

        HRESULT hr = d3d12.d3dDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
        if (SUCCEEDED(hr))
        {
            d3d12.d3dFeatureLevel = featLevels.MaxSupportedFeatureLevel;
        }
        else
        {
            d3d12.d3dFeatureLevel = d3d12.min_feature_level;
        }
    }

    adapter->Release();

    /* Create command queue's */
    const D3D12_COMMAND_QUEUE_DESC directQueueDesc = {
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
        D3D12_COMMAND_QUEUE_FLAG_NONE,
        0x0
    };

    const D3D12_COMMAND_QUEUE_DESC computeQueueDesc = {
        D3D12_COMMAND_LIST_TYPE_COMPUTE,
        D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
        D3D12_COMMAND_QUEUE_FLAG_NONE,
        0x0
    };

    VHR(d3d12.d3dDevice->CreateCommandQueue(&directQueueDesc, IID_PPV_ARGS(&d3d12.direct_command_queue)));
    VHR(d3d12.d3dDevice->CreateCommandQueue(&computeQueueDesc, IID_PPV_ARGS(&d3d12.compute_command_queue)));
    VHR(d3d12.direct_command_queue->SetName(L"Direct Command Queue"));
    VHR(d3d12.compute_command_queue->SetName(L"Compute Command Queue"));

    /* Create descriptor heaps */
    {
        d3d12_CreateDescriptorHeap(&d3d12.RTVHeap, 1024, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false);
        d3d12_CreateDescriptorHeap(&d3d12.DSVHeap, 256, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false);
    }

    return true;
}

static void d3d12_shutdown(void)
{
    /* Destroy command queues */
    SAFE_RELEASE(d3d12.compute_command_queue);
    SAFE_RELEASE(d3d12.direct_command_queue);

    /* Destroy descriptor heaps */
    SAFE_RELEASE(d3d12.RTVHeap.handle);
    SAFE_RELEASE(d3d12.DSVHeap.handle);

    ULONG refCount = d3d12.d3dDevice->Release();
#if !defined(NDEBUG)
    if (refCount > 0)
    {
        //gpuLog(GPULogLevel_Error, "Direct3D12: There are %d unreleased references left on the device", ref_count);

        ID3D12DebugDevice* d3dDebugDevice = nullptr;
        if (SUCCEEDED(d3d12.d3dDevice->QueryInterface(IID_PPV_ARGS(&d3dDebugDevice))))
        {
            d3dDebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_IGNORE_INTERNAL);
            d3dDebugDevice->Release();
        }
    }
#else
    (void)refCount; // avoid warning
#endif

    SAFE_RELEASE(d3d12.factory);

#ifdef _DEBUG
    IDXGIDebug1* dxgiDebug;
    if (SUCCEEDED(vgpuDXGIGetDebugInterface1(0, __uuidof(IDXGIDebug1), (void**)&dxgiDebug)))
    {
        dxgiDebug->ReportLiveObjects(D3D_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        dxgiDebug->Release();
    }
#endif

    memset(&d3d12, 0, sizeof(d3d12));
}

static bool d3d12_beginFrame(void)
{
    return true;
}

static void d3d12_endFrame(void)
{
}

/* Texture */
VGPUTexture d3d12_allocTexture(void) {
    return { VGPU_INVALID_ID };
};

static bool d3d12_initTexture(VGPUTexture handle, const VGPUTextureDescription* desc)
{
    return true;
}

static void d3d12_destroyTexture(VGPUTexture handle)
{
}

/* Driver functions */
static bool d3d12_isSupported(void) {
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

    HRESULT hr = vgpuD3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);

    if (FAILED(hr))
    {
        return false;
    }

    d3d12.available = true;
    return true;
}

static VGPUGraphicsContext* d3d12_createContext(void) {
    static VGPUGraphicsContext graphicsContext = { nullptr };
    ASSIGN_DRIVER(d3d12);
    return &graphicsContext;
}

vgpu_driver d3d12_driver = {
    VGPUBackendType_D3D12,
    d3d12_isSupported,
    d3d12_createContext
};

#endif /* defined(VGPU_DRIVER_VULKAN) */
