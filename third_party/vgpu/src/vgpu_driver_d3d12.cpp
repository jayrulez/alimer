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
#include "vgpu_d3d_common.h"

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

struct d3d12_persistent_descriptor
{
    D3D12_CPU_DESCRIPTOR_HANDLE handles[VGPU_NUM_INFLIGHT_FRAMES];
    uint32_t index;
};

struct d3d12_descriptor_heap
{
    uint32_t persistent_allocated;
    uint32_t num_persistent;
    uint32_t heap_index;
    uint32_t num_heaps;
    uint32_t descriptor_size;
    uint32_t total_descriptors;

    ID3D12DescriptorHeap* heaps[VGPU_NUM_INFLIGHT_FRAMES];
    std::vector<uint32_t> dead_list;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_start[VGPU_NUM_INFLIGHT_FRAMES];
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_start[VGPU_NUM_INFLIGHT_FRAMES];
    SRWLOCK lock;
};

struct d3d12_texture {
    vgpu_texture_info info;
    D3D12MA::Allocation* allocation;
    ID3D12Resource* handle;
    D3D12_RESOURCE_STATES state; 
    union {
        D3D12_CPU_DESCRIPTOR_HANDLE rtv;
        D3D12_CPU_DESCRIPTOR_HANDLE dsv;
    };
};

struct d3d12_framebuffer {
    uint32_t color_rtvs_count;
    D3D12_CPU_DESCRIPTOR_HANDLE color_rtvs[VGPU_MAX_COLOR_ATTACHMENTS];
    D3D12_CPU_DESCRIPTOR_HANDLE* dsv;
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
    D3D_FEATURE_LEVEL feature_level;
    D3D_ROOT_SIGNATURE_VERSION root_signature_version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    bool render_pass_support;

    ID3D12CommandQueue* direct_command_queue;
    ID3D12CommandQueue* compute_command_queue;

    d3d12_descriptor_heap rtv_heap;
    d3d12_descriptor_heap dsv_heap;

    uint32_t num_backbuffers;
    IDXGISwapChain3* swapchain;
    vgpu_texture backbuffer_textures[3u];
    vgpu_texture depth_stencil_texture;
    uint32_t backbuffer_index;

    /* Frame data */
    uint64_t render_latency;
    ID3D12Fence* frame_fence;
    HANDLE frame_fence_event;
    uint64_t frame_number;
    uint64_t frame_index;

    ID3D12CommandAllocator* command_allocators[3u];
    ID3D12GraphicsCommandList* command_list;
    ID3D12GraphicsCommandList4* command_list4;

    bool shutting_down;
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

    if (d3d12.shutting_down || d3d12.device == nullptr) {
        resource->Release();
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

static d3d12_descriptor_heap d3d12_create_descriptor_heap(uint32_t num_persistent, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shader_visible)
{
    VGPU_ASSERT(num_persistent > 0);

    if (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV || type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
        shader_visible = false;

    d3d12_descriptor_heap descriptor_heap = {};
    descriptor_heap.persistent_allocated = 0;
    descriptor_heap.num_persistent = num_persistent;
    descriptor_heap.heap_index = 0;
    descriptor_heap.num_heaps = shader_visible ? VGPU_NUM_INFLIGHT_FRAMES : 1;
    descriptor_heap.total_descriptors = num_persistent;

    descriptor_heap.dead_list.resize(num_persistent);
    for (uint32_t i = 0; i < num_persistent; ++i)
    {
        descriptor_heap.dead_list[i] = uint32_t(i);
    }

    descriptor_heap.descriptor_size = d3d12.device->GetDescriptorHandleIncrementSize(type);
    descriptor_heap.lock = SRWLOCK_INIT;

    D3D12_DESCRIPTOR_HEAP_DESC d3d12_desc = {};
    d3d12_desc.Type = type;
    d3d12_desc.NumDescriptors = descriptor_heap.total_descriptors;
    d3d12_desc.Flags = shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    d3d12_desc.NodeMask = 0;
    for (uint32_t i = 0; i < descriptor_heap.num_heaps; ++i)
    {
        VHR(d3d12.device->CreateDescriptorHeap(&d3d12_desc, IID_PPV_ARGS(&descriptor_heap.heaps[i])));
        descriptor_heap.cpu_start[i] = descriptor_heap.heaps[i]->GetCPUDescriptorHandleForHeapStart();

        if (shader_visible)
        {
            descriptor_heap.gpu_start[i] = descriptor_heap.heaps[i]->GetGPUDescriptorHandleForHeapStart();
        }
    }

    descriptor_heap.num_persistent = num_persistent;

    return descriptor_heap;
}

static void d3d12_destroy_descriptor_heap(d3d12_descriptor_heap* heap) {
    VGPU_ASSERT(heap != nullptr);
    VGPU_ASSERT(heap->persistent_allocated == 0);
    for (uint32_t i = 0; i < heap->num_heaps; ++i)
    {
        SAFE_RELEASE(heap->heaps[i]);
    }
}

static d3d12_persistent_descriptor d3d12_allocate_persistent(d3d12_descriptor_heap* heap) {
    VGPU_ASSERT(heap != nullptr && heap->heaps[0] != nullptr);

    AcquireSRWLockExclusive(&heap->lock);

    VGPU_ASSERT(heap->persistent_allocated < heap->num_persistent);
    uint32_t index = heap->dead_list[heap->persistent_allocated];
    ++heap->persistent_allocated;

    ReleaseSRWLockExclusive(&heap->lock);

    d3d12_persistent_descriptor alloc;
    alloc.index = index;
    for (uint32_t i = 0; i < heap->num_heaps; ++i)
    {
        alloc.handles[i] = heap->cpu_start[i];
        alloc.handles[i].ptr += index * heap->descriptor_size;
    }

    return alloc;
}

static uint32_t d3d12_index_from_handle(d3d12_descriptor_heap* heap, D3D12_CPU_DESCRIPTOR_HANDLE handle) {
    VGPU_ASSERT(heap->heaps[0] != nullptr);
    VGPU_ASSERT(handle.ptr >= heap->cpu_start[heap->heap_index].ptr);
    VGPU_ASSERT(handle.ptr < heap->cpu_start[heap->heap_index].ptr + heap->descriptor_size * heap->total_descriptors);
    VGPU_ASSERT((handle.ptr - heap->cpu_start[heap->heap_index].ptr) % heap->descriptor_size == 0);
    return uint32_t(handle.ptr - heap->cpu_start[heap->heap_index].ptr) / heap->descriptor_size;
}

static void d3d12_free_persistent(d3d12_descriptor_heap* heap, uint32_t& index) {
    if (index == uint32_t(-1))
        return;

    VGPU_ASSERT(index < heap->num_persistent);
    VGPU_ASSERT(heap->heaps[0] != nullptr);

    AcquireSRWLockExclusive(&heap->lock);

    VGPU_ASSERT(heap->persistent_allocated > 0);
    heap->dead_list[heap->persistent_allocated - 1] = index;
    --heap->persistent_allocated;

    ReleaseSRWLockExclusive(&heap->lock);

    index = uint32_t(-1);
}

static void d3d12_free_persistent(d3d12_descriptor_heap* heap, D3D12_CPU_DESCRIPTOR_HANDLE& handle) {
    VGPU_ASSERT(heap->num_heaps == 1);
    if (handle.ptr != 0)
    {
        uint32_t index = d3d12_index_from_handle(heap, handle);
        d3d12_free_persistent(heap, index);
        handle = {};
    }
}

/* Command List helpers */
static void _vgpu_d3d12_transition_resource(
    ID3D12GraphicsCommandList* command_list,
    ID3D12Resource* resource,
    D3D12_RESOURCE_STATES before,
    D3D12_RESOURCE_STATES after,
    uint32_t subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    barrier.Transition.Subresource = subResource;
    command_list->ResourceBarrier(1, &barrier);
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

    // Create descriptor heaps.
    {
        d3d12.rtv_heap = d3d12_create_descriptor_heap(256u, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
        d3d12.dsv_heap = d3d12_create_descriptor_heap(256u, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
    }

    if (config->swapchain.native_handle) {
        uint32_t factory_caps = DXGIFACTORY_CAPS_FLIP_PRESENT;
        if (d3d12.tearing_support) {
            factory_caps |= DXGIFACTORY_CAPS_TEARING;
        }

        d3d12.num_backbuffers = 2u;

        IDXGISwapChain* tempSwapChain = vgpu_d3d_create_swapchain(
            d3d12.factory,
            factory_caps,
            d3d12.direct_command_queue,
            config->swapchain.native_handle,
            config->swapchain.width, config->swapchain.height,
            config->swapchain.color_format,
            d3d12.num_backbuffers,
            config->swapchain.is_fullscreen
        );
        VHR(tempSwapChain->QueryInterface(IID_PPV_ARGS(&d3d12.swapchain)));
        SAFE_RELEASE(tempSwapChain);

        vgpu_texture_info texture_info = {};
        texture_info.type = VGPU_TEXTURE_TYPE_2D;
        texture_info.format = config->swapchain.color_format;
        texture_info.width = config->swapchain.width;
        texture_info.height = config->swapchain.height;
        texture_info.usage = VGPU_TEXTURE_USAGE_RENDER_TARGET;

        for (uint32_t index = 0; index < d3d12.num_backbuffers; ++index)
        {
            ID3D12Resource* resource;
            VHR(d3d12.swapchain->GetBuffer(index, IID_PPV_ARGS(&resource)));

            texture_info.external_handle = (uintptr_t)resource;
            d3d12.backbuffer_textures[index] = vgpu_create_texture(&texture_info);
        }

        if (config->swapchain.depth_stencil_format != VGPU_PIXEL_FORMAT_UNDEFINED) {
            vgpu_texture_info depth_stencil_texture_info = {};
            depth_stencil_texture_info.type = VGPU_TEXTURE_TYPE_2D;
            depth_stencil_texture_info.format = config->swapchain.depth_stencil_format;
            depth_stencil_texture_info.width = config->swapchain.width;
            depth_stencil_texture_info.height = config->swapchain.height;
            depth_stencil_texture_info.usage = VGPU_TEXTURE_USAGE_RENDER_TARGET;
            d3d12.depth_stencil_texture = vgpu_create_texture(&depth_stencil_texture_info);
        }

        d3d12.backbuffer_index = d3d12.swapchain->GetCurrentBackBufferIndex();
    }

    // Initialize caps
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

    HRESULT hr = d3d12.device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
    if (SUCCEEDED(hr))
    {
        d3d12.feature_level = featLevels.MaxSupportedFeatureLevel;
    }
    else
    {
        d3d12.feature_level = d3d12.min_feature_level;
    }

    // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

    if (FAILED(d3d12.device->CheckFeatureSupport(
        D3D12_FEATURE_ROOT_SIGNATURE,
        &featureData, sizeof(featureData))))
    {
        d3d12.root_signature_version = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS5 d3d12options5 = {};
    if (SUCCEEDED(d3d12.device->CheckFeatureSupport(
        D3D12_FEATURE_D3D12_OPTIONS5,
        &d3d12options5, sizeof(d3d12options5)))
        && d3d12options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
    {
        //caps.features.raytracing = true;
    }
    else
    {
        //caps.features.raytracing = false;
    }

    d3d12.render_pass_support = false;
    if (d3d12options5.RenderPassesTier > D3D12_RENDER_PASS_TIER_0/* &&
        static_cast<GPUVendorId>(caps.vendorId) != GPUVendorId::Intel*/)
    {
        d3d12.render_pass_support = true;
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
            VHR(d3d12.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&d3d12.command_allocators[i])));
        }

        VHR(d3d12.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, d3d12.command_allocators[0], nullptr, IID_PPV_ARGS(&d3d12.command_list)));
        d3d12.command_list->SetName(L"Frame Command List");

        if (FAILED(d3d12.command_list->QueryInterface(&d3d12.command_list4))) {
            d3d12.render_pass_support = false;
        }

        VHR(d3d12.command_list->Close());
    }

    return true;
}

static void d3d12_wait_frame(void) {
    d3d12.direct_command_queue->Signal(d3d12.frame_fence, ++d3d12.frame_number);
    d3d12.frame_fence->SetEventOnCompletion(d3d12.frame_number, d3d12.frame_fence_event);
    WaitForSingleObject(d3d12.frame_fence_event, INFINITE);

    d3d12_execute_deferred_releases();
    d3d12.frame_index = d3d12.frame_number % d3d12.render_latency;

    //GPUDescriptorHeaps[d3d12.frame_index].Size = 0;
    //GPUUploadMemoryHeaps[d3d12.frame_index].Size = 0;
}

static void d3d12_shutdown(void) {
    d3d12_wait_frame();
    d3d12.shutting_down = true;
    SAFE_RELEASE(d3d12.direct_command_queue);
    CloseHandle(d3d12.frame_fence_event);
    SAFE_RELEASE(d3d12.frame_fence);
    for (uint32_t index = 0; index < d3d12.num_backbuffers; ++index)
    {
        vgpu_destroy_texture(d3d12.backbuffer_textures[index]);
    }
    if (d3d12.depth_stencil_texture) {
        vgpu_destroy_texture(d3d12.depth_stencil_texture);
    }

    SAFE_RELEASE(d3d12.swapchain);

    for (uint64_t i = 0; i < d3d12.render_latency; ++i)
    {
        SAFE_RELEASE(d3d12.command_allocators[i]);
    }
    SAFE_RELEASE(d3d12.command_list4);
    SAFE_RELEASE(d3d12.command_list);
    d3d12_destroy_descriptor_heap(&d3d12.rtv_heap);
    d3d12_destroy_descriptor_heap(&d3d12.dsv_heap);

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
    VHR(d3d12.command_allocators[d3d12.frame_index]->Reset());
    VHR(d3d12.command_list->Reset(d3d12.command_allocators[d3d12.frame_index], nullptr));
    //d3d12.command_list->SetDescriptorHeaps(1, &d3d12->CbvSrvUavGpuHeaps[d3d12.frame_index].Heap);

    // Indicate that the back buffer will be used as a render target.
    if (d3d12.swapchain) {
        d3d12_texture* texture = (d3d12_texture*)d3d12.backbuffer_textures[d3d12.backbuffer_index];
        _vgpu_d3d12_transition_resource(d3d12.command_list, texture->handle, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    }
}

static void d3d12_end_frame(void) {
    // Indicate that the back buffer will now be used to present.
    if (d3d12.swapchain) {
        d3d12_texture* texture = (d3d12_texture*)d3d12.backbuffer_textures[d3d12.backbuffer_index];
        _vgpu_d3d12_transition_resource(d3d12.command_list, texture->handle, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    }

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

/* Texture */
static vgpu_texture d3d12_texture_create(const vgpu_texture_info* info) {
    d3d12_texture* texture = VGPU_ALLOC(d3d12_texture);
    texture->info = *info;

    DXGI_FORMAT dxgi_format = _vgpu_d3d_format_with_usage(info->format, info->usage);

    if (info->external_handle) {
        texture->allocation = nullptr;
        texture->handle = (ID3D12Resource*)info->external_handle;
        texture->state = D3D12_RESOURCE_STATE_COMMON;
    }
    else {
        D3D12MA::ALLOCATION_DESC allocation_desc = {};
        allocation_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resource_desc = {};
        resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // D3D12GetResourceDimension(desc.type);
        resource_desc.Alignment = 0;
        resource_desc.Width = info->width;
        resource_desc.Height = info->height;
        resource_desc.DepthOrArraySize = 1;

        if (info->type == VGPU_TEXTURE_TYPE_CUBE)
        {
            resource_desc.DepthOrArraySize = info->array_layers * 6;
        }
        else
        {
            resource_desc.DepthOrArraySize = info->depth;
        }

        resource_desc.MipLevels = info->mip_levels;
        resource_desc.Format = dxgi_format;
        resource_desc.SampleDesc.Count = 1u;
        resource_desc.SampleDesc.Quality = 0;
        resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON;
        D3D12_CLEAR_VALUE clear_value = {};
        D3D12_CLEAR_VALUE* p_clear_value = nullptr;

        if (info->usage & VGPU_TEXTURE_USAGE_RENDER_TARGET) {
            // Render and Depth/Stencil targets are always committed resources
            allocation_desc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;

            clear_value.Format = dxgi_format;
            if (vgpu_is_depth_stencil_format(info->format))
            {
                initial_state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
                resource_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

                if (!(info->usage & VGPU_TEXTURE_USAGE_SAMPLED))
                {
                    resource_desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
                }

                clear_value.DepthStencil.Depth = 1.0f;
            }
            else
            {
                initial_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
                resource_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            }

            p_clear_value = &clear_value;
        }

        texture->state = info->content != nullptr ? D3D12_RESOURCE_STATE_COPY_DEST : initial_state;

        HRESULT hr = d3d12.allocator->CreateResource(
            &allocation_desc,
            &resource_desc,
            texture->state,
            p_clear_value,
            &texture->allocation,
            IID_PPV_ARGS(&texture->handle)
        );

        if (FAILED(hr)) {
            //LOG_ERROR("Direct3D12: Failed to create texture");
            return nullptr;
        }
    }

    if (info->usage & VGPU_TEXTURE_USAGE_RENDER_TARGET) {
        if (vgpu_is_depth_stencil_format(info->format)) {
            texture->dsv = d3d12_allocate_persistent(&d3d12.dsv_heap).handles[0];
            d3d12.device->CreateDepthStencilView(texture->handle, nullptr, texture->dsv);
        }
        else {
            texture->rtv = d3d12_allocate_persistent(&d3d12.rtv_heap).handles[0];

            D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = { };
            rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            rtv_desc.Format = dxgi_format;
            rtv_desc.Texture2D.MipSlice = 0;
            rtv_desc.Texture2D.PlaneSlice = 0;
            d3d12.device->CreateRenderTargetView(texture->handle, &rtv_desc, texture->rtv);
        }
    }

    return (vgpu_texture)texture;
}

static void d3d12_texture_destroy(vgpu_texture handle) {
    d3d12_texture* texture = (d3d12_texture*)handle;
    if (texture->info.usage & VGPU_TEXTURE_USAGE_RENDER_TARGET) {
        if (vgpu_is_depth_stencil_format(texture->info.format)) {
            d3d12_free_persistent(&d3d12.dsv_heap, texture->dsv);
        }
        else {
            d3d12_free_persistent(&d3d12.rtv_heap, texture->rtv);
        }
    }

    SAFE_RELEASE(texture->allocation);
    d3d12_release_resource(texture->handle);
    VGPU_FREE(texture);
}

static vgpu_texture_info d3d12_query_texture_info(vgpu_texture handle) {
    d3d12_texture* texture = (d3d12_texture*)handle;
    return texture->info;
}
/* Commands */
static vgpu_texture d3d12_get_backbuffer_texture(void) {
    return d3d12.backbuffer_textures[d3d12.backbuffer_index];
}

static void d3d12_begin_pass(const vgpu_pass_begin_info* info) {
    if (d3d12.render_pass_support) {
        D3D12_RENDER_PASS_RENDER_TARGET_DESC colorRenderPassTargets[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};

        D3D12_RENDER_PASS_FLAGS renderPassFlags = D3D12_RENDER_PASS_FLAG_NONE;
        //d3d12.command_list4->BeginRenderPass(framebuffer->color_rtvs_count, colorRenderPassTargets, nullptr, renderPassFlags);
    }
    else {
        uint32_t color_rtvs_count = 0;
        D3D12_CPU_DESCRIPTOR_HANDLE color_rtvs[VGPU_MAX_COLOR_ATTACHMENTS] = {};

        for (uint32_t i = 0; i < VGPU_MAX_COLOR_ATTACHMENTS; i++)
        {
            const vgpu_color_attachment_info* attachment = &info->color_attachments[i];
            if (!attachment->texture) {
                break;
            }

            d3d12_texture* texture = (d3d12_texture*)attachment->texture;
            //TransitionResource(texture, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
            if (attachment->load_op == VGPU_LOAD_OP_CLEAR)
            {
                d3d12.command_list->ClearRenderTargetView(texture->rtv, &attachment->clear_color.r, 0, nullptr);
            }
            color_rtvs[color_rtvs_count++] = texture->rtv;
        }

        d3d12.command_list->OMSetRenderTargets(color_rtvs_count, color_rtvs, FALSE, NULL);
    }
}

static void d3d12_end_pass(void) {
    if (d3d12.render_pass_support) {
        d3d12.command_list4->EndRenderPass();
    }
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

    renderer.texture_create = d3d12_texture_create;
    renderer.texture_destroy = d3d12_texture_destroy;
    renderer.query_texture_info = d3d12_query_texture_info;

    renderer.get_backbuffer_texture = d3d12_get_backbuffer_texture;
    renderer.begin_pass = d3d12_begin_pass;
    renderer.end_pass = d3d12_end_pass;

    return &renderer;
}

vgpu_driver d3d12_driver = {
    VGPU_BACKEND_TYPE_D3D12,
    d3d12_is_supported,
    d3d12_init_renderer
};

#endif /* defined(VGPU_DRIVER_D3D12) */
