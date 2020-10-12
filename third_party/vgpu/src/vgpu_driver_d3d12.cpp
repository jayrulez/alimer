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

#define WIN32_LEAN_AND_MEAN
#include <d3d12.h>
#include <D3D12MemAlloc.h>
#include "vgpu_driver_d3d_common.h"

// To use graphics and CPU markup events with the latest version of PIX, change this to include <pix3.h>
// then add the NuGet package WinPixEventRuntime to the project.
#include <pix.h>


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
    vgpu_caps caps;

    DWORD factory_flags;
    IDXGIFactory4* factory;
    DXGIFactoryCaps factory_caps;
    bool tearing_supported = false;
    D3D_FEATURE_LEVEL min_feature_level;

    ID3D12Device* device;
    D3D12MA::Allocator* memory_allocator;
    D3D_FEATURE_LEVEL   feature_level;
    bool                is_lost;
    bool                shutting_down;
} d3d12;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#   define vgpuCreateDXGIFactory2 d3d12.CreateDXGIFactory2
#   define vgpuDXGIGetDebugInterface1 d3d12.DXGIGetDebugInterface1
#   define vgpuD3D12GetDebugInterface d3d12.D3D12GetDebugInterface
#   define vgpuD3D12CreateDevice d3d12.D3D12CreateDevice
#else
#   define vgpuCreateDXGIFactory2 CreateDXGIFactory2
#   define vgpuDXGIGetDebugInterface1 DXGIGetDebugInterface1
#   define vgpuD3D12GetDebugInterface D3D12GetDebugInterface
#   define vgpuD3D12CreateDevice D3D12CreateDevice
#endif

static bool _vgpu_d3d12_create_factory(void) {
    SAFE_RELEASE(d3d12.factory);

    HRESULT hr = S_OK;

#if defined(_DEBUG)
    if (d3d12.debug)
    {
        ID3D12Debug* debugController;
        if (SUCCEEDED(vgpuD3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
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
        if (SUCCEEDED(vgpuDXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
        {
            d3d12.factory_flags = DXGI_CREATE_FACTORY_DEBUG;

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
    hr = vgpuCreateDXGIFactory2(d3d12.factory_flags, IID_PPV_ARGS(&d3d12.factory));
    if (FAILED(hr)) {
        return false;
    }

    d3d12.factory_caps = DXGIFactoryCaps::FlipPresent | DXGIFactoryCaps::HDR;

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
            d3d12.factory_caps |= DXGIFactoryCaps::Tearing;
            d3d12.tearing_supported = true;
        }
    }

    return true;
}

static IDXGIAdapter1* _vgpu_d3d12_get_adapter(bool lowPower) {
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
            if (SUCCEEDED(vgpuD3D12CreateDevice(adapter, d3d12.min_feature_level, _uuidof(ID3D12Device), nullptr)))
            {
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
            if (SUCCEEDED(vgpuD3D12CreateDevice(adapter, d3d12.min_feature_level, _uuidof(ID3D12Device), nullptr)))
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
            vgpu_log(VGPU_LOG_LEVEL_ERROR, "WARP12 not available. Enable the 'Graphics Tools' optional feature");
        }

        OutputDebugStringA("Direct3D Adapter - WARP12\n");
    }
#endif

    if (!adapter)
    {
        vgpu_log(VGPU_LOG_LEVEL_ERROR, "No Direct3D 12 device found");
    }

    return adapter;
}

static bool d3d12_init(const char* app_name, const vgpu_config* config)
{
    VGPU_UNUSED(app_name);

    d3d12.debug = config->debug;
    d3d12.GPUBasedValidation = false;

    d3d12.factory_flags = 0;
    d3d12.min_feature_level = D3D_FEATURE_LEVEL_11_0;

    if (!_vgpu_d3d12_create_factory()) {
        return false;
    }

    const bool lowPower = config->device_preference == VGPU_ADAPTER_TYPE_INTEGRATED_GPU;
    IDXGIAdapter1* adapter = _vgpu_d3d12_get_adapter(lowPower);

    // Create the DX12 API device object.
    {
        VHR(vgpuD3D12CreateDevice(adapter, d3d12.min_feature_level, IID_PPV_ARGS(&d3d12.device)));

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

        VHR(D3D12MA::CreateAllocator(&desc, &d3d12.memory_allocator));
    }

    /* Init caps first. */
    {
        DXGI_ADAPTER_DESC1 adapterDesc;
        VHR(adapter->GetDesc1(&adapterDesc));

        /* Log some info */
        vgpu_log(VGPU_LOG_LEVEL_INFO, "GPU driver: D3D12");
        vgpu_log(VGPU_LOG_LEVEL_INFO, "Direct3D Adapter: VID:%04X, PID:%04X - %ls", adapterDesc.VendorId, adapterDesc.DeviceId, adapterDesc.Description);

        d3d12.caps.backend = VGPU_BACKEND_TYPE_D3D12;
        d3d12.caps.vendor_id = adapterDesc.VendorId;
        d3d12.caps.adapter_id = adapterDesc.DeviceId;

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
        d3d12.caps.limits.maxVertexAttributes = VGPU_MAX_VERTEX_ATTRIBUTES;
        d3d12.caps.limits.maxVertexBindings = VGPU_MAX_VERTEX_ATTRIBUTES;
        d3d12.caps.limits.maxVertexAttributeOffset = VGPU_MAX_VERTEX_ATTRIBUTE_OFFSET;
        d3d12.caps.limits.maxVertexBindingStride = VGPU_MAX_VERTEX_BUFFER_STRIDE;

        d3d12.caps.limits.maxTextureDimension2D = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
        d3d12.caps.limits.maxTextureDimension3D = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
        d3d12.caps.limits.maxTextureDimensionCube = D3D12_REQ_TEXTURECUBE_DIMENSION;
        d3d12.caps.limits.maxTextureArrayLayers = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
        d3d12.caps.limits.maxColorAttachments = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
        d3d12.caps.limits.max_uniform_buffer_range = D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
        d3d12.caps.limits.min_uniform_buffer_offset_alignment = 256u;
        d3d12.caps.limits.max_storage_buffer_range = UINT32_MAX;
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
        d3d12.caps.limits.max_compute_work_group_count[0] = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        d3d12.caps.limits.max_compute_work_group_count[1] = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        d3d12.caps.limits.max_compute_work_group_count[2] = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        d3d12.caps.limits.max_compute_work_group_invocations = D3D12_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
        d3d12.caps.limits.max_compute_work_group_size[0] = D3D12_CS_THREAD_GROUP_MAX_X;
        d3d12.caps.limits.max_compute_work_group_size[1] = D3D12_CS_THREAD_GROUP_MAX_Y;
        d3d12.caps.limits.max_compute_work_group_size[2] = D3D12_CS_THREAD_GROUP_MAX_Z;

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

    return true;
}

static void d3d12_shutdown(void) {
}

static bool d3d12_frame_begin(void) {
    return true;
}

static void d3d12_frame_finish(void) {

    // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
    if (!d3d12.factory->IsCurrent())
    {
        _vgpu_d3d12_create_factory();
    }
}

static void d3d12_query_caps(vgpu_caps* caps) {
    *caps = d3d12.caps;
}

/* Buffer */
static vgpu_buffer d3d12_buffer_create(const vgpu_buffer_info* info) {
    return nullptr;
    //vk_buffer* buffer = VGPU_ALLOC(vk_buffer);
    //return (vgpu_buffer)buffer;
}

static void d3d12_buffer_destroy(vgpu_buffer handle) {
}

/* Shader */
static vgpu_shader d3d12_shader_create(const vgpu_shader_info* info) {
    return nullptr;
    //vk_shader* shader = VGPU_ALLOC(vk_shader);
    //return (vgpu_shader)shader;
}

static void d3d12_shader_destroy(vgpu_shader handle) {

}

/* Texture */
static vgpu_texture d3d12_texture_create(const vgpu_texture_info* info) {
    return NULL;
}

static void d3d12_texture_destroy(vgpu_texture handle) {
}

static uint64_t d3d12_texture_get_native_handle(vgpu_texture handle) {
    return 0;
    //vk_texture* texture = (vk_texture*)handle;
    //return VOIDP_TO_U64(texture->handle);
}

/* Pipeline */
static vgpu_pipeline d3d12_pipeline_create(const vgpu_pipeline_info* info)
{
    return nullptr;
    //vk_pipeline* pipeline = VGPU_ALLOC(vk_pipeline);
    //return (vgpu_pipeline)pipeline;
}

static void d3d12_pipeline_destroy(vgpu_pipeline handle) {
}

/* Commands */
static void d3d12_push_debug_group(const char* name) {
}

static void d3d12_pop_debug_group(void) {
}

static void d3d12_insert_debug_marker(const char* name) {
}

static void d3d12_begin_render_pass(const vgpu_render_pass_info* info) {

}

static void d3d12_end_render_pass(void) {
}

static void d3d12_bind_pipeline(vgpu_pipeline handle) {
    //vk_pipeline* pipeline = (vk_pipeline*)handle;
}

static void d3d12_draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex) {
}

/* Driver */
static bool d3d12_Is_supported(void)
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

    if (SUCCEEDED(vgpuD3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
        d3d12.available = true;
        return true;
    }

    return false;
};

static vgpu_renderer* d3d12_create_renderer(void)
{
    static vgpu_renderer renderer = { 0 };
    ASSIGN_DRIVER(d3d12);
    return &renderer;
}

vgpu_driver d3d12_driver = {
    VGPU_BACKEND_TYPE_D3D12,
    d3d12_Is_supported,
    d3d12_create_renderer
};

#endif /* defined(VGPU_DRIVER_D3D12)  */
