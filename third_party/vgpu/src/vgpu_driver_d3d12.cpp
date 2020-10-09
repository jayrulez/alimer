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

#if defined(AGPU_DRIVER_D3D12)

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define D3D11_NO_HELPERS
#include <d3d12.h>
#include <D3D12MemAlloc.h>
#include "agpu_driver_d3d_common.h"

// To use graphics and CPU markup events with the latest version of PIX, change this to include <pix3.h>
// then add the NuGet package WinPixEventRuntime to the project.
#include <pix.h>

#include <inttypes.h>
#include <mutex>

struct D3D12SwapChain
{

    uint32_t                width;
    uint32_t                height;
    agpu_texture_format     color_format;
    bool                    is_fullscreen;
    bool                    is_primary;


    // HDR Support
    DXGI_COLOR_SPACE_TYPE colorSpace;

    IDXGISwapChain3* handle;

    agpu_texture backbufferTextures[AGPU_MAX_INFLIGHT_FRAMES];
    agpu_texture depthStencilTexture;
};

struct D3D12Buffer
{
    ID3D12Resource* handle;
};

struct D3D12Texture
{
    ID3D12Resource* handle;
    D3D12_RESOURCE_STATES state;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvOrDsvHandle;
};

struct D3D12_Fence
{
    ID3D12Fence* handle;
    HANDLE          fenceEvent;
};

struct D3D12_DescriptorHeap
{
    ID3D12DescriptorHeap* Heap;
    D3D12_CPU_DESCRIPTOR_HANDLE CpuStart;
    D3D12_GPU_DESCRIPTOR_HANDLE GpuStart;
    uint32_t Size;
    uint32_t Capacity;
    uint32_t DescriptorSize;
};

/* Global data */
static struct {
    bool available_initialized;
    bool available;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    HMODULE dxgiDLL;
    HMODULE d3d12DLL;

    PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2 = nullptr;
    PFN_GET_DXGI_DEBUG_INTERFACE1 DXGIGetDebugInterface1 = nullptr;
    PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface = nullptr;
    PFN_D3D12_CREATE_DEVICE D3D12CreateDevice = nullptr;
#endif

    bool debug;
    bool GPUBasedValidation;
    agpu_caps caps;

    DWORD dxgiFactoryFlags;
    IDXGIFactory4* factory;
    DXGIFactoryCaps factoryCaps;
    bool isTearingSupported = false;
    D3D_FEATURE_LEVEL minFeatureLevel;

    ID3D12Device* device;
    D3D12MA::Allocator* allocator;
    D3D_FEATURE_LEVEL feature_level;
    bool isLost;
    bool shuttingDown;

    D3D12_DescriptorHeap RtvHeap;
    D3D12_DescriptorHeap DsvHeap;
    D3D12_DescriptorHeap CbvSrvUavCpuHeap;


    ID3D12CommandQueue* graphicsQueue;
    ID3D12CommandQueue* computeQueue;
    ID3D12CommandAllocator* commandAllocators[AGPU_NUM_INFLIGHT_FRAMES] = {};
    ID3D12GraphicsCommandList4* commandList;

    /* Frame data */
    D3D12_Fence frameFence;
    uint64_t    currentCPUFrame = 0;
    uint64_t    currentGPUFrame = 0;
    uint32_t    frameIndex = 0;

    agpu_swapchain                  main_swapchain;

    agpu::Array<D3D12SwapChain>     swapchains;
    agpu::Array<D3D12Buffer>        buffers;
    agpu::Array<D3D12Texture>       textures;

    agpu::Array<IUnknown*>          deferredReleases[AGPU_NUM_INFLIGHT_FRAMES];
} d3d12;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#   define agpuCreateDXGIFactory2 d3d12.CreateDXGIFactory2
#   define agpuDXGIGetDebugInterface1 d3d12.DXGIGetDebugInterface1
#   define agpuD3D12GetDebugInterface d3d12.D3D12GetDebugInterface
#   define agpuD3D12CreateDevice d3d12.D3D12CreateDevice
#else
#   define agpuCreateDXGIFactory2 CreateDXGIFactory2
#   define agpuDXGIGetDebugInterface1 DXGIGetDebugInterface1
#   define agpuD3D12GetDebugInterface D3D12GetDebugInterface
#   define agpuD3D12CreateDevice D3D12CreateDevice
#endif

/* Deferred release logic */
void _agpu_d3d12_DeferredRelease_(IUnknown* resource)
{
    if (resource == nullptr)
        return;

    if ((d3d12.currentCPUFrame == d3d12.currentGPUFrame) || d3d12.shuttingDown || d3d12.device == nullptr)
    {
        // Safe to release.
        resource->Release();
        return;
    }

    d3d12.deferredReleases[d3d12.frameIndex].Add(resource);
}


template<typename T> void _agpu_d3d12_DeferredRelease(T*& resource)
{
    IUnknown* base = resource;
    _agpu_d3d12_DeferredRelease_(base);
    resource = nullptr;
}

static void ProcessDeferredReleases(uint32_t frameIndex)
{
    for (uint32_t i = 0; i < d3d12.deferredReleases[frameIndex].Size; ++i)
    {
        d3d12.deferredReleases[frameIndex][i]->Release();
    }

    d3d12.deferredReleases[frameIndex].Clear();
}

static void _agpu_d3d12_barrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES old_state, D3D12_RESOURCE_STATES new_state) {
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = old_state;
    barrier.Transition.StateAfter = new_state;
    d3d12.commandList->ResourceBarrier(1, &barrier);
}
static void d3d12_texture_barrier(D3D12Texture& texture, D3D12_RESOURCE_STATES new_state);

/* Device/Renderer */
static bool d3d12_CreateFactory(void)
{
    SAFE_RELEASE(d3d12.factory);

    HRESULT hr = S_OK;

#if defined(_DEBUG)
    if (d3d12.debug)
    {
        ID3D12Debug* debugController;
        if (SUCCEEDED(agpuD3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            ID3D12Debug1* debugController1 = nullptr;
            if (SUCCEEDED(debugController->QueryInterface(&debugController1)))
            {
                debugController1->SetEnableGPUBasedValidation(d3d12.GPUBasedValidation);
            }

            SAFE_RELEASE(debugController);
            SAFE_RELEASE(debugController1);
        }
        else
        {
            OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
        }

        IDXGIInfoQueue* dxgiInfoQueue;
        if (SUCCEEDED(agpuDXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
        {
            d3d12.dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

            VHR(dxgiInfoQueue->SetBreakOnSeverity(D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE));
            VHR(dxgiInfoQueue->SetBreakOnSeverity(D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE));
            VHR(dxgiInfoQueue->SetBreakOnSeverity(D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, FALSE));

            DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
            {
                80 // IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides.
            };

            DXGI_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            dxgiInfoQueue->AddStorageFilterEntries(D3D_DXGI_DEBUG_DXGI, &filter);
            dxgiInfoQueue->Release();
        }
    }

#endif
    hr = agpuCreateDXGIFactory2(d3d12.dxgiFactoryFlags, IID_PPV_ARGS(&d3d12.factory));
    if (FAILED(hr)) {
        return false;
    }

    d3d12.factoryCaps = DXGIFactoryCaps::FlipPresent | DXGIFactoryCaps::HDR;

    // Check tearing support.
    {
        BOOL allowTearing = FALSE;
        IDXGIFactory5* factory5 = nullptr;
        HRESULT hr = d3d12.factory->QueryInterface(IID_PPV_ARGS(&factory5));
        if (SUCCEEDED(hr))
        {
            hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
        }

        SAFE_RELEASE(factory5);

        if (FAILED(hr) || !allowTearing)
        {
#ifdef _DEBUG
            OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
        }
        else
        {
            d3d12.factoryCaps |= DXGIFactoryCaps::Tearing;
            d3d12.isTearingSupported = true;
        }
    }

    return true;
}

static IDXGIAdapter1* d3d12_GetAdapter(bool lowPower)
{
    /* Detect adapter now. */
    IDXGIAdapter1* adapter = NULL;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
    IDXGIFactory6* factory6 = nullptr;
    HRESULT hr = d3d12.factory->QueryInterface(IID_PPV_ARGS(&factory6));
    if (SUCCEEDED(hr))
    {
        // By default prefer high performance
        DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
        if (lowPower) {
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

            // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
            if (SUCCEEDED(agpuD3D12CreateDevice(adapter, d3d12.minFeatureLevel, _uuidof(ID3D12Device), nullptr)))
            {
#ifdef _DEBUG
                wchar_t buff[256] = {};
                swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", i, desc.VendorId, desc.DeviceId, desc.Description);
                OutputDebugStringW(buff);
#endif
                break;
            }
        }
    }

    SAFE_RELEASE(factory6);
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
            if (SUCCEEDED(agpuD3D12CreateDevice(adapter, d3d12.minFeatureLevel, _uuidof(ID3D12Device), nullptr)))
            {
#ifdef _DEBUG
                wchar_t buff[256] = {};
                swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", i, desc.VendorId, desc.DeviceId, desc.Description);
                OutputDebugStringW(buff);
#endif
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
            agpu_log(AGPU_LOG_LEVEL_ERROR, "WARP12 not available. Enable the 'Graphics Tools' optional feature");
        }

        OutputDebugStringA("Direct3D Adapter - WARP12\n");
    }
#endif

    if (!adapter)
    {
        agpu_log(AGPU_LOG_LEVEL_ERROR, "No Direct3D 12 device found");
    }

    return adapter;
}

static D3D12_DescriptorHeap _agpu_d3d12_CreateDescriptorHeap(uint32_t capacity, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
    AGPU_ASSERT(d3d12.device && capacity > 0);

    D3D12_DescriptorHeap descriptorHeap = {};
    descriptorHeap.Capacity = capacity;

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = capacity;
    heapDesc.Type = type;
    heapDesc.Flags = flags;
    VHR(d3d12.device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap.Heap)));

    descriptorHeap.CpuStart = descriptorHeap.Heap->GetCPUDescriptorHandleForHeapStart();
    if (flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
    {
        descriptorHeap.GpuStart = descriptorHeap.Heap->GetGPUDescriptorHandleForHeapStart();
    }

    descriptorHeap.DescriptorSize = d3d12.device->GetDescriptorHandleIncrementSize(type);
    return descriptorHeap;
}

static D3D12_CPU_DESCRIPTOR_HANDLE _agpu_d3d12_AllocateCpuDescriptorsFromHeap(D3D12_DescriptorHeap* descriptorHeap, uint32_t count)
{
    AGPU_ASSERT(descriptorHeap && (descriptorHeap->Size + count) < descriptorHeap->Capacity);

    D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle;
    CpuHandle.ptr = descriptorHeap->CpuStart.ptr + (uint64_t)descriptorHeap->Size * descriptorHeap->DescriptorSize;
    descriptorHeap->Size += count;
    return CpuHandle;
}

static D3D12_CPU_DESCRIPTOR_HANDLE _agpu_d3d12_AllocateCpuDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count)
{
    if (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
    {
        return _agpu_d3d12_AllocateCpuDescriptorsFromHeap(&d3d12.RtvHeap, count);
    }
    if (type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
    {
        return _agpu_d3d12_AllocateCpuDescriptorsFromHeap(&d3d12.DsvHeap, count);
    }
    if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    {
        return _agpu_d3d12_AllocateCpuDescriptorsFromHeap(&d3d12.CbvSrvUavCpuHeap, count);
    }

    AGPU_ASSERT(0);
    return D3D12_CPU_DESCRIPTOR_HANDLE{ 0 };
}

/* Fence */
static D3D12_Fence _agpu_d3d12_CreateFence(void)
{
    AGPU_ASSERT(d3d12.device);

    D3D12_Fence fence = {};
    VHR(d3d12.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence.handle)));
    fence.fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
    AGPU_ASSERT(fence.fenceEvent != NULL);
    return fence;
}

static void _agpu_d3d12_DestroyFence(D3D12_Fence* fence)
{
    CloseHandle(fence->fenceEvent);
    _agpu_d3d12_DeferredRelease(fence->handle);
}

static void _agpu_d3d12_SignalFence(D3D12_Fence* fence, ID3D12CommandQueue* queue, uint64_t fenceValue)
{
    AGPU_ASSERT(queue);
    VHR(queue->Signal(fence->handle, fenceValue));
}

static void _agpu_d3d12_WaitFence(D3D12_Fence* fence, uint64_t fenceValue)
{
    uint64_t gpuValue = fence->handle->GetCompletedValue();
    if (gpuValue < fenceValue)
    {
        VHR(fence->handle->SetEventOnCompletion(fenceValue, fence->fenceEvent));
        WaitForSingleObject(fence->fenceEvent, INFINITE);
    }
}

static bool d3d12_init(agpu_init_flags flags, const agpu_swapchain_info* swapchain_info)
{
    d3d12.debug = (flags & AGPU_INIT_FLAGS_DEBUG) || (flags & AGPU_INIT_FLAGS_GPU_VALIDATION);
    d3d12.GPUBasedValidation = (flags & AGPU_INIT_FLAGS_GPU_VALIDATION);
    d3d12.dxgiFactoryFlags = 0;
    d3d12.minFeatureLevel = D3D_FEATURE_LEVEL_11_0;

    if (!d3d12_CreateFactory()) {
        return false;
    }

    const bool lowPower = (flags & AGPU_INIT_FLAGS_LOW_POWER_GPU);
    IDXGIAdapter1* adapter = d3d12_GetAdapter(lowPower);

    // Create the DX12 API device object.
    {
        VHR(agpuD3D12CreateDevice(adapter, d3d12.minFeatureLevel, IID_PPV_ARGS(&d3d12.device)));

#ifndef NDEBUG
        // Configure debug device (if active).
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
    }

    // Create memory allocator.
    {
        D3D12MA::ALLOCATOR_DESC desc = {};
        desc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
        desc.pDevice = d3d12.device;
        desc.pAdapter = adapter;

        VHR(D3D12MA::CreateAllocator(&desc, &d3d12.allocator));
    }

    /* Init caps first. */
    {
        DXGI_ADAPTER_DESC1 adapterDesc;
        VHR(adapter->GetDesc1(&adapterDesc));

        /* Log some info */
        agpu_log(AGPU_LOG_LEVEL_INFO, "GPU driver: D3D12");
        agpu_log(AGPU_LOG_LEVEL_INFO, "Direct3D Adapter: VID:%04X, PID:%04X - %ls", adapterDesc.VendorId, adapterDesc.DeviceId, adapterDesc.Description);

        d3d12.caps.backend = AGPU_BACKEND_TYPE_D3D12;
        d3d12.caps.vendorID = adapterDesc.VendorId;
        d3d12.caps.deviceID = adapterDesc.DeviceId;

        d3d12.caps.features.independentBlend = true;
        d3d12.caps.features.computeShader = true;
        d3d12.caps.features.indexUInt32 = true;
        d3d12.caps.features.fillModeNonSolid = true;
        d3d12.caps.features.samplerAnisotropy = true;
        d3d12.caps.features.textureCompressionETC2 = false;
        d3d12.caps.features.textureCompressionASTC_LDR = false;
        d3d12.caps.features.textureCompressionBC = true;
        d3d12.caps.features.textureCubeArray = true;
        d3d12.caps.features.raytracing = false;

        // Limits
        d3d12.caps.limits.maxVertexAttributes = AGPU_MAX_VERTEX_ATTRIBUTES;
        d3d12.caps.limits.maxVertexBindings = AGPU_MAX_VERTEX_ATTRIBUTES;
        d3d12.caps.limits.maxVertexAttributeOffset = AGPU_MAX_VERTEX_ATTRIBUTE_OFFSET;
        d3d12.caps.limits.maxVertexBindingStride = AGPU_MAX_VERTEX_BUFFER_STRIDE;

        d3d12.caps.limits.maxTextureDimension2D = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
        d3d12.caps.limits.maxTextureDimension3D = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
        d3d12.caps.limits.maxTextureDimensionCube = D3D12_REQ_TEXTURECUBE_DIMENSION;
        d3d12.caps.limits.maxTextureArrayLayers = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
        d3d12.caps.limits.maxColorAttachments = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
        d3d12.caps.limits.max_uniform_buffer_size = D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
        d3d12.caps.limits.min_uniform_buffer_offset_alignment = 256u;
        d3d12.caps.limits.max_storage_buffer_size = UINT32_MAX;
        d3d12.caps.limits.min_storage_buffer_offset_alignment = 16;
        d3d12.caps.limits.max_sampler_anisotropy = D3D12_MAX_MAXANISOTROPY;
        d3d12.caps.limits.max_viewports = D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        d3d12.caps.limits.max_viewport_width = D3D12_VIEWPORT_BOUNDS_MAX;
        d3d12.caps.limits.max_viewport_height = D3D12_VIEWPORT_BOUNDS_MAX;
        d3d12.caps.limits.max_tessellation_patch_size = D3D12_IA_PATCH_MAX_CONTROL_POINT_COUNT;
        d3d12.caps.limits.point_size_range_min = 1.0f;
        d3d12.caps.limits.point_size_range_max = 1.0f;
        d3d12.caps.limits.line_width_range_min = 1.0f;
        d3d12.caps.limits.line_width_range_max = 1.0f;
        d3d12.caps.limits.max_compute_shared_memory_size = D3D12_CS_THREAD_LOCAL_TEMP_REGISTER_POOL;
        d3d12.caps.limits.max_compute_work_group_count_x = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        d3d12.caps.limits.max_compute_work_group_count_y = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        d3d12.caps.limits.max_compute_work_group_count_z = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        d3d12.caps.limits.max_compute_work_group_invocations = D3D12_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
        d3d12.caps.limits.max_compute_work_group_size_x = D3D12_CS_THREAD_GROUP_MAX_X;
        d3d12.caps.limits.max_compute_work_group_size_y = D3D12_CS_THREAD_GROUP_MAX_Y;
        d3d12.caps.limits.max_compute_work_group_size_z = D3D12_CS_THREAD_GROUP_MAX_Z;

#if TODO
        /* see: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_format_support */
        UINT dxgi_fmt_caps = 0;
        for (int fmt = (VGPUTextureFormat_Undefined + 1); fmt < VGPUTextureFormat_Count; fmt++)
        {
            DXGI_FORMAT dxgi_fmt = _vgpu_d3d_get_format((VGPUTextureFormat)fmt);
            HRESULT hr = ID3D11Device1_CheckFormatSupport(renderer->d3d_device, dxgi_fmt, &dxgi_fmt_caps);
            VGPU_ASSERT(SUCCEEDED(hr));
            /*sg_pixelformat_info* info = &_sg.formats[fmt];
            info->sample = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_TEXTURE2D);
            info->filter = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE);
            info->render = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_RENDER_TARGET);
            info->blend = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_BLENDABLE);
            info->msaa = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET);
            info->depth = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL);
            if (info->depth) {
                info->render = true;
            }*/
        }
#endif // TODO

    }

    /* Release adapter */
    SAFE_RELEASE(adapter);

    // Descriptor Heaps
    {
        d3d12.RtvHeap = _agpu_d3d12_CreateDescriptorHeap(1024, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
        d3d12.DsvHeap = _agpu_d3d12_CreateDescriptorHeap(256, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
        d3d12.CbvSrvUavCpuHeap = _agpu_d3d12_CreateDescriptorHeap(16 * 1024, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
    }

    /* Create command queue's. */
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.NodeMask = 0;
        VHR(d3d12.device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&d3d12.graphicsQueue)));
        d3d12.graphicsQueue->SetName(L"Graphics Command Queue");

        /* Compute queue instead of copy so we can generate mip levels */
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        VHR(d3d12.device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&d3d12.computeQueue)));
        d3d12.computeQueue->SetName(L"Compute Command Queue");
    }

    for (uint32_t i = 0; i < AGPU_NUM_INFLIGHT_FRAMES; ++i)
    {
        VHR(d3d12.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&d3d12.commandAllocators[i])));
    }

    VHR(d3d12.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, d3d12.commandAllocators[0], nullptr, IID_PPV_ARGS(&d3d12.commandList)));
    VHR(d3d12.commandList->SetName(L"Primary Graphics Command List"));
    VHR(d3d12.commandList->Close());

    /* Frame fence */
    d3d12.frameFence = _agpu_d3d12_CreateFence();

    /* Init pools*/
    d3d12.swapchains.Reserve(8);
    d3d12.buffers.Reserve(256);
    d3d12.textures.Reserve(256);

    /* Create swap chain if required. */
    if (swapchain_info != nullptr)
    {
        d3d12.main_swapchain = agpu_create_swapchain(swapchain_info);
    }

    d3d12.shuttingDown = false;

    return true;
}

static void d3d12_wait_for_gpu(void)
{
    // Wait for the GPU to fully catch up with the CPU
    AGPU_ASSERT(d3d12.currentCPUFrame >= d3d12.currentGPUFrame);
    if (d3d12.currentCPUFrame > d3d12.currentGPUFrame)
    {
        _agpu_d3d12_WaitFence(&d3d12.frameFence, d3d12.currentCPUFrame);
        d3d12.currentGPUFrame = d3d12.currentCPUFrame;
    }

    //Gfx->GPUDescriptorHeaps[Gfx->FrameIndex].Size = 0;
    //Gfx->GPUUploadMemoryHeaps[Gfx->FrameIndex].Size = 0;

    // Clean up what we can now
    for (uint32_t i = 1; i < AGPU_NUM_INFLIGHT_FRAMES; ++i)
    {
        uint32_t frameIndex = (i + d3d12.frameIndex) % AGPU_NUM_INFLIGHT_FRAMES;
        ProcessDeferredReleases(frameIndex);
    }
}

static void d3d12_shutdown()
{
    d3d12_wait_for_gpu();

    AGPU_ASSERT(d3d12.currentCPUFrame == d3d12.currentGPUFrame);
    d3d12.shuttingDown = true;

    if (d3d12.main_swapchain.id != AGPU_INVALID_ID)
    {
        agpu_destroy_swapchain(d3d12.main_swapchain);
    }

    for (uint32_t i = 0; i < _countof(d3d12.deferredReleases); ++i)
    {
        ProcessDeferredReleases(i);
    }

    for (uint32_t i = 0; i < AGPU_NUM_INFLIGHT_FRAMES; ++i)
    {
        SAFE_RELEASE(d3d12.commandAllocators[i]);
        //SAFE_RELEASE(Gfx->GPUDescriptorHeaps[i].Heap);
        //SAFE_RELEASE(Gfx->GPUUploadMemoryHeaps[i].Heap);
    }

    SAFE_RELEASE(d3d12.RtvHeap.Heap);
    SAFE_RELEASE(d3d12.DsvHeap.Heap);
    SAFE_RELEASE(d3d12.CbvSrvUavCpuHeap.Heap);

    SAFE_RELEASE(d3d12.commandList);
    SAFE_RELEASE(d3d12.graphicsQueue);
    SAFE_RELEASE(d3d12.computeQueue);

    _agpu_d3d12_DestroyFence(&d3d12.frameFence);

    // Allocator.
    if (d3d12.allocator != nullptr)
    {
        D3D12MA::Stats stats;
        d3d12.allocator->CalculateStats(&stats);

        if (stats.Total.UsedBytes > 0) {
            agpu_log(AGPU_LOG_LEVEL_ERROR, "Total device memory leaked: %" PRIu64 "bytes.", stats.Total.UsedBytes);
        }

        SAFE_RELEASE(d3d12.allocator);
    }


#if !defined(NDEBUG)
    ULONG refCount = d3d12.device->Release();
    if (refCount > 0)
    {
        agpu_log(AGPU_LOG_LEVEL_WARN, "Direct3D12: There are %d unreleased references left on the device", refCount);

        ID3D12DebugDevice* debugDevice = nullptr;
        if (SUCCEEDED(d3d12.device->QueryInterface(IID_PPV_ARGS(&debugDevice))))
        {
            debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
            debugDevice->Release();
        }
    }
    else
    {
        agpu_log(AGPU_LOG_LEVEL_INFO, "Direct3D12: No leaks detected");
    }
#else
    d3d12.device->Release();
#endif

    SAFE_RELEASE(d3d12.factory);

#ifdef _DEBUG
    IDXGIDebug1* dxgiDebug1;
    if (SUCCEEDED(agpuDXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug1))))
    {
        dxgiDebug1->ReportLiveObjects(D3D_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        dxgiDebug1->Release();
    }
#endif
}

static void d3d12_update_color_space(D3D12SwapChain& swapchain)
{
    swapchain.colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;

    bool isDisplayHDR10 = false;

#if defined(NTDDI_WIN10_RS2)
    IDXGIOutput* output;
    if (SUCCEEDED(swapchain.handle->GetContainingOutput(&output)))
    {
        IDXGIOutput6* output6;
        if (SUCCEEDED(output->QueryInterface(IID_PPV_ARGS(&output6))))
        {
            DXGI_OUTPUT_DESC1 desc;
            VHR(output6->GetDesc1(&desc));

            if (desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
            {
                // Display output is HDR10.
                isDisplayHDR10 = true;
            }

            output6->Release();
        }

        output->Release();
    }
#endif

    if (isDisplayHDR10)
    {
        switch (swapchain.color_format)
        {
        case AGPU_TEXTURE_FORMAT_RGBA16_UNORM:
            // The application creates the HDR10 signal.
            swapchain.colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
            break;

        case AGPU_TEXTURE_FORMAT_RGBA32_FLOAT:
            // The system creates the HDR10 signal; application uses linear values.
            swapchain.colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
            break;

        default:
            break;
        }
    }

    UINT colorSpaceSupport = 0;
    if (SUCCEEDED(swapchain.handle->CheckColorSpaceSupport(swapchain.colorSpace, &colorSpaceSupport))
        && (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
    {
        VHR(swapchain.handle->SetColorSpace1(swapchain.colorSpace));
    }
}

static void d3d11_AfterReset(D3D12SwapChain& swapchain)
{
    d3d12_update_color_space(swapchain);

    DXGI_SWAP_CHAIN_DESC1 swapchainDesc;
    VHR(swapchain.handle->GetDesc1(&swapchainDesc));
    swapchain.width = swapchainDesc.Width;
    swapchain.height = swapchainDesc.Height;

    agpu_texture_info backbuffer_texture_info{};
    backbuffer_texture_info.usage = AGPU_TEXTURE_USAGE_RENDER_TARGET;
    backbuffer_texture_info.format = swapchain.color_format;
    backbuffer_texture_info.width = swapchain.width;
    backbuffer_texture_info.height = swapchain.height;

    for (uint32_t i = 0; i < swapchainDesc.BufferCount; i++)
    {
        ID3D12Resource* backbuffer;
        VHR(swapchain.handle->GetBuffer(i, IID_PPV_ARGS(&backbuffer)));

        backbuffer_texture_info.external_handle = backbuffer;
        swapchain.backbufferTextures[i] = agpu_create_texture(&backbuffer_texture_info);
        backbuffer->Release();
    }
}

static bool d3d12_frame_begin(void)
{
    if (d3d12.isLost)
        return false;

    // Prepare the command buffers to be used for the next frame
    VHR(d3d12.commandAllocators[d3d12.frameIndex]->Reset());
    VHR(d3d12.commandList->Reset(d3d12.commandAllocators[d3d12.frameIndex], nullptr));
    //d3d12.commandList->SetDescriptorHeaps(1, &d3d12.GPUDescriptorHeaps[d3d12.frameIndex].Heap);

    return true;
}

static void d3d12_frame_finish(void)
{
    HRESULT hr = S_OK;
    // TODO: Add ResourceUploadBatch


    VHR(d3d12.commandList->Close());

    ID3D12CommandList* commandLists[] = { d3d12.commandList };
    d3d12.graphicsQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    ++d3d12.currentCPUFrame;

    // Signal the fence with the current frame number, so that we can check back on it
    _agpu_d3d12_SignalFence(&d3d12.frameFence, d3d12.graphicsQueue, d3d12.currentCPUFrame);

    // Wait for the GPU to catch up before we stomp an executing command buffer
    const uint64_t gpuLag = d3d12.currentCPUFrame - d3d12.currentGPUFrame;
    AGPU_ASSERT(gpuLag <= AGPU_NUM_INFLIGHT_FRAMES);
    if (gpuLag >= AGPU_NUM_INFLIGHT_FRAMES)
    {
        // Make sure that the previous frame is finished
        _agpu_d3d12_WaitFence(&d3d12.frameFence, d3d12.currentGPUFrame + 1);
        ++d3d12.currentGPUFrame;
    }

    d3d12.frameIndex = d3d12.currentCPUFrame % AGPU_NUM_INFLIGHT_FRAMES;

    // See if we have any deferred releases to process
    ProcessDeferredReleases(d3d12.frameIndex);

    // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
    if (!d3d12.factory->IsCurrent())
    {
        d3d12_CreateFactory();
    }
}

static void d3d12_query_caps(agpu_caps* caps)
{
    *caps = d3d12.caps;
}

static agpu_swapchain d3d12_create_swapchain(const agpu_swapchain_info* info)
{
    D3D12SwapChain swapchain{};
    swapchain.colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
    swapchain.color_format = info->color_format;
    swapchain.is_primary = info->is_primary;
    swapchain.is_fullscreen = info->is_fullscreen;

    /* TODO: Multisample */

    // Create swapchain if required.
    IDXGISwapChain* tempSwapChain = agpu_d3dCreateSwapChain(
        d3d12.factory, d3d12.factoryCaps, d3d12.graphicsQueue,
        info->window_handle,
        agpu_ToDXGISwapChainFormat(info->color_format),
        info->width, info->height,
        AGPU_NUM_INFLIGHT_FRAMES);

    VHR(tempSwapChain->QueryInterface(IID_PPV_ARGS(&swapchain.handle)));
    SAFE_RELEASE(tempSwapChain);

    d3d11_AfterReset(swapchain);

    d3d12.swapchains.Add(swapchain);
    return { d3d12.swapchains.Size };
}

static void d3d12_destroy_swapchain(agpu_swapchain handle)
{
    d3d12_wait_for_gpu();

    D3D12SwapChain& swapchain = d3d12.swapchains[handle.id - 1];
    for (uint32_t i = 0; i < AGPU_COUNT_OF(swapchain.backbufferTextures); i++)
    {
        agpu_destroy_texture(swapchain.backbufferTextures[i]);
    }

    agpu_destroy_texture(swapchain.depthStencilTexture);
    SAFE_RELEASE(swapchain.handle);

    // Unset primary id
    if (handle.id == d3d12.main_swapchain.id)
    {
        d3d12.main_swapchain.id = AGPU_INVALID_ID;
    }
}

static agpu_swapchain d3d12_get_main_swapchain(void)
{
    return d3d12.main_swapchain;
}

static agpu_texture d3d12_get_current_texture(agpu_swapchain handle)
{
    D3D12SwapChain& swapchain = d3d12.swapchains[handle.id - 1];
    const UINT backbufferIndex = swapchain.handle->GetCurrentBackBufferIndex();
    return swapchain.backbufferTextures[backbufferIndex];
}

static void d3d12_present(agpu_swapchain handle, bool vsync)
{
    D3D12SwapChain& swapchain = d3d12.swapchains[handle.id - 1];
    D3D12Texture& texture = d3d12.textures[agpu_get_current_texture(handle).id - 1];
    d3d12_texture_barrier(texture, D3D12_RESOURCE_STATE_PRESENT);

    UINT syncInterval = vsync ? 1 : 0;
    UINT present_flags = (!vsync && !swapchain.is_fullscreen && d3d12.isTearingSupported) ? DXGI_PRESENT_ALLOW_TEARING : 0;

    HRESULT hr = swapchain.handle->Present(syncInterval, present_flags);
    if (hr == DXGI_ERROR_DEVICE_REMOVED
        || hr == DXGI_ERROR_DEVICE_HUNG
        || hr == DXGI_ERROR_DEVICE_RESET
        || hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR
        || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
    {
        d3d12.isLost = true;
        return;
    }

    VHR(hr);
}

static agpu_buffer d3d12_createBuffer(const agpu_buffer_info* info)
{
    D3D12Buffer buffer{};
    buffer.handle = nullptr;
    /*HRESULT hr = d3d11.device->CreateBuffer(&bufferDesc, initialDataPtr, &buffer.handle);
    if (FAILED(hr))
    {
        logError("Direct3D11: Failed to create buffer");
        return kInvalidBuffer;
    }*/

    d3d12.buffers.Add(buffer);
    return { d3d12.buffers.Size };
}

static void d3d12_destroyBuffer(agpu_buffer handle)
{

}

static agpu_texture d3d12_create_texture(const agpu_texture_info* info)
{
    D3D12Texture texture{};

    texture.state = D3D12_RESOURCE_STATE_COMMON;
    if (info->external_handle)
    {
        texture.handle = (ID3D12Resource*)info->external_handle;
        texture.handle->AddRef();
    }
    else
    {

    }

    if (info->usage & AGPU_TEXTURE_USAGE_RENDER_TARGET)
    {
        texture.rtvOrDsvHandle = _agpu_d3d12_AllocateCpuDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1);
        d3d12.device->CreateRenderTargetView(texture.handle, nullptr, texture.rtvOrDsvHandle);
    }

    d3d12.textures.Add(texture);
    return { d3d12.textures.Size };
}

static void d3d12_destroy_texture(agpu_texture handle)
{
    D3D12Texture& texture = d3d12.textures[handle.id - 1];
    _agpu_d3d12_DeferredRelease(texture.handle);
}

/* Commands */
static void d3d12_texture_barrier(D3D12Texture& texture, D3D12_RESOURCE_STATES new_state)
{
    if (texture.state == new_state)
        return;

    _agpu_d3d12_barrier(texture.handle, texture.state, new_state);
    texture.state = new_state;
}

static void d3d12_PushDebugGroup(const char* name)
{
    PIXBeginEvent(d3d12.commandList, PIX_COLOR_DEFAULT, name);
}

static void d3d12_PopDebugGroup(void)
{
    PIXEndEvent(d3d12.commandList);
}

static void d3d12_InsertDebugMarker(const char* name)
{
    PIXSetMarker(d3d12.commandList, PIX_COLOR_DEFAULT, name);
}

static void d3d12_begin_render_pass(const agpu_render_pass_info* info)
{
    D3D12_CPU_DESCRIPTOR_HANDLE color_rtvs[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT]{};

    for (uint32_t i = 0; i < info->num_color_attachments; i++)
    {
        D3D12Texture& texture = d3d12.textures[info->color_attachments[i].texture.id - 1];
        d3d12_texture_barrier(texture, D3D12_RESOURCE_STATE_RENDER_TARGET);

        color_rtvs[i] = texture.rtvOrDsvHandle;

        switch (info->color_attachments[i].load_op)
        {
        case AGPU_LOAD_OP_CLEAR:
            d3d12.commandList->ClearRenderTargetView(color_rtvs[i], &info->color_attachments[i].clear_color.r, 0, nullptr);
            break;

        default:
            break;
        }

    }

    d3d12.commandList->OMSetRenderTargets(info->num_color_attachments, color_rtvs, FALSE, nullptr);
}

static void d3d12_end_render_pass(void)
{

}

/* Driver */
static bool d3d12_IsSupported(void)
{
    if (d3d12.available_initialized) {
        return d3d12.available;
    }

    d3d12.available_initialized = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
    d3d12.dxgiDLL = LoadLibraryA("dxgi.dll");
    if (!d3d12.dxgiDLL) {
        return false;
    }

    d3d12.CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(d3d12.dxgiDLL, "CreateDXGIFactory2");
    if (!d3d12.CreateDXGIFactory2)
    {
        return false;
    }

    d3d12.DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(d3d12.dxgiDLL, "DXGIGetDebugInterface1");

    d3d12.d3d12DLL = LoadLibraryA("d3d12.dll");
    if (!d3d12.d3d12DLL) {
        return false;
    }

    d3d12.D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(d3d12.d3d12DLL, "D3D12GetDebugInterface");
    d3d12.D3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(d3d12.d3d12DLL, "D3D12CreateDevice");
    if (!d3d12.D3D12CreateDevice) {
        return false;
    }
#endif

    d3d12.available = true;
    return true;
};

static agpu_renderer* d3d12_CreateRenderer(void)
{
    static agpu_renderer renderer = { 0 };
    ASSIGN_DRIVER(d3d12);
    return &renderer;
}

agpu_driver d3d12_driver = {
    AGPU_BACKEND_TYPE_D3D12,
    d3d12_IsSupported,
    d3d12_CreateRenderer
};

#endif /* defined(AGPU_DRIVER_D3D11)  */
