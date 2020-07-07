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

#if defined(VGPU_DRIVER_D3D11)

#include "vgpu_d3d_common.h"
#define D3D11_NO_HELPERS
#include <d3d11_1.h>
#include <vector>

typedef struct d3d11_texture {
    ID3D11Resource* handle;
    std::vector<ID3D11RenderTargetView*> renderTargetViews;
} d3d11_texture;

typedef struct d3d11_framebuffer {
    IDXGISwapChain1* swapchain;
    bool destroy_textures;
    vgpu_texture color_textures[VGPU_MAX_COLOR_ATTACHMENTS];
    vgpu_texture depth_stencil_texture;
    uint32_t color_rtvs_count;
    ID3D11RenderTargetView* color_rtvs;
} d3d11_framebuffer;

/* Global data */
static struct {
    bool available_initialized;
    bool available;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    struct
    {
        HMODULE instance;
        PFN_CREATE_DXGI_FACTORY1 CreateDXGIFactory1;
        PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2;
        PFN_GET_DXGI_DEBUG_INTERFACE1 DXGIGetDebugInterface1;
    } dxgi;

    HMODULE instance;
    PFN_D3D11_CREATE_DEVICE D3D11CreateDevice;
#endif

    bool debug;
    IDXGIFactory2* factory;
    uint32_t factory_caps;
    bool is_lost;
    uint32_t num_backbuffers;

    ID3D11Device1* device;
    ID3D11DeviceContext1* immediate_context;
    D3D_FEATURE_LEVEL feature_level;

    vgpu_caps caps;

    vgpu_framebuffer main_swapchain;
    std::vector<vgpu_framebuffer> swapchain;
} d3d11;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#   define _vgpu_CreateDXGIFactory1 d3d11.dxgi.CreateDXGIFactory1
#   define _vgpu_CreateDXGIFactory2 d3d11.dxgi.CreateDXGIFactory2
#   define _vgpu_D3D11CreateDevice d3d11.D3D11CreateDevice
#else
#   define _vgpu_CreateDXGIFactory1 CreateDXGIFactory1
#   define _vgpu_CreateDXGIFactory2 CreateDXGIFactory2
#   define _vgpu_D3D11CreateDevice D3D11CreateDevice
#endif

/* Renderer */
static bool d3d11_create_factory()
{
    SAFE_RELEASE(d3d11.factory);

#if defined(_DEBUG) && (_WIN32_WINNT >= 0x0603 /*_WIN32_WINNT_WINBLUE*/)
    bool debugDXGI = false;
    if (d3d11.debug)
    {
        IDXGIInfoQueue* dxgiInfoQueue;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (d3d11.dxgi.DXGIGetDebugInterface1 != nullptr && SUCCEEDED(d3d11.dxgi.DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
#else
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue))))
#endif
        {
            debugDXGI = true;

            VHR(_vgpu_CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&d3d11.factory)));

            dxgiInfoQueue->SetBreakOnSeverity(vgpu_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
            dxgiInfoQueue->SetBreakOnSeverity(vgpu_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

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
    }

    if (!debugDXGI)
#endif
    {
        VHR(_vgpu_CreateDXGIFactory1(IID_PPV_ARGS(&d3d11.factory)));
    }

    // Determines whether tearing support is available for fullscreen borderless windows.
    {
        BOOL allowTearing = FALSE;

        IDXGIFactory5* factory5 = nullptr;
        HRESULT hr = d3d11.factory->QueryInterface(IID_PPV_ARGS(&factory5));
        if (SUCCEEDED(hr))
        {
            hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
        }

        if (FAILED(hr) || !allowTearing)
        {
#ifdef _DEBUG
            OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
        }
        else
        {
            d3d11.factory_caps |= DXGIFACTORY_CAPS_TEARING;
        }

        SAFE_RELEASE(factory5);
    }

    // Disable FLIP if not on a supporting OS
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    {
        IDXGIFactory4* factory4 = nullptr;
        if (FAILED(d3d11.factory->QueryInterface(IID_PPV_ARGS(&factory4))))
        {
#ifdef _DEBUG
            OutputDebugStringA("INFO: Flip swap effects not supported");
#endif
        }
        else {
            d3d11.factory_caps |= DXGIFACTORY_CAPS_FLIP_PRESENT;
        }

        SAFE_RELEASE(factory4);
    }
#endif

    return true;
}

static IDXGIAdapter1* d3d11_get_adapter(vgpu_device_preference device_preference) {
    /* Detect adapter now. */
    IDXGIAdapter1* adapter = nullptr;


#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
    if (device_preference != VGPU_DEVICE_PREFERENCE_DONT_CARE) {
        IDXGIFactory6* dxgiFactory6;
        HRESULT hr = d3d11.factory->QueryInterface(IID_PPV_ARGS(&dxgiFactory6));
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

                break;
            }

            dxgiFactory6->Release();
        }
    }
#endif

    if (!adapter)
    {
        for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != d3d11.factory->EnumAdapters1(i, &adapter); i++)
        {
            DXGI_ADAPTER_DESC1 desc;
            VHR(adapter->GetDesc1(&desc));

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                adapter->Release();

                continue;
            }

            break;
        }
    }

    return adapter;
}

static bool _vgpu_d3d11_sdk_layers_available() {
    HRESULT hr = _vgpu_D3D11CreateDevice(
        NULL,
        D3D_DRIVER_TYPE_NULL,
        NULL,
        D3D11_CREATE_DEVICE_DEBUG,
        NULL,
        0,
        D3D11_SDK_VERSION,
        NULL,
        NULL,
        NULL
    );

    return SUCCEEDED(hr);
}

static bool d3d11_init(const vgpu_init_info* info) {
    d3d11.debug = info->debug;
    if (!d3d11_create_factory()) {
        return false;
    }

    d3d11.num_backbuffers = 2u;

    IDXGIAdapter1* dxgi_adapter = d3d11_get_adapter(info->device_preference);

    /* Create d3d11 device */
    {
        UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

        if (info->debug && _vgpu_d3d11_sdk_layers_available())
        {
            // If the project is in a debug build, enable debugging via SDK Layers with this flag.
            creation_flags |= D3D11_CREATE_DEVICE_DEBUG;
        }
#if defined(_DEBUG)
        else
        {
            OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
        }
#endif

        static const D3D_FEATURE_LEVEL s_featureLevels[] =
        {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0
        };

        // Create the Direct3D 11 API device object and a corresponding context.
        ID3D11Device* temp_d3d_device;
        ID3D11DeviceContext* temp_d3d_context;

        HRESULT hr = E_FAIL;
        if (dxgi_adapter)
        {
            hr = _vgpu_D3D11CreateDevice(
                (IDXGIAdapter*)dxgi_adapter,
                D3D_DRIVER_TYPE_UNKNOWN,
                NULL,
                creation_flags,
                s_featureLevels,
                _countof(s_featureLevels),
                D3D11_SDK_VERSION,
                &temp_d3d_device,
                &d3d11.feature_level,
                &temp_d3d_context
            );
        }
#if defined(NDEBUG)
        else
        {
            vgpu_log_error("No Direct3D hardware device found");
            VGPU_UNREACHABLE();
        }
#else
        if (FAILED(hr))
        {
            // If the initialization fails, fall back to the WARP device.
            // For more information on WARP, see:
            // http://go.microsoft.com/fwlink/?LinkId=286690
            hr = _vgpu_D3D11CreateDevice(
                NULL,
                D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
                NULL,
                creation_flags,
                s_featureLevels,
                _countof(s_featureLevels),
                D3D11_SDK_VERSION,
                &temp_d3d_device,
                &d3d11.feature_level,
                &temp_d3d_context
            );

            if (SUCCEEDED(hr))
            {
                OutputDebugStringA("Direct3D Adapter - WARP\n");
            }
        }
#endif

        if (FAILED(hr)) {
            return false;
        }

#ifndef NDEBUG
        ID3D11Debug* d3d_debug;
        if (SUCCEEDED(temp_d3d_device->QueryInterface(IID_PPV_ARGS(&d3d_debug))))
        {
            ID3D11InfoQueue* d3d_info_queue;
            if (SUCCEEDED(d3d_debug->QueryInterface(IID_PPV_ARGS(&d3d_info_queue))))
            {
#ifdef _DEBUG
                d3d_info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
                d3d_info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
                D3D11_MESSAGE_ID hide[] =
                {
                    D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
                };
                D3D11_INFO_QUEUE_FILTER filter;
                memset(&filter, 0, sizeof(D3D11_INFO_QUEUE_FILTER));
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                d3d_info_queue->AddStorageFilterEntries(&filter);
                d3d_info_queue->Release();
            }

            d3d_debug->Release();
        }
#endif

        VHR(temp_d3d_device->QueryInterface(IID_PPV_ARGS(&d3d11.device)));
        VHR(temp_d3d_context->QueryInterface(IID_PPV_ARGS(&d3d11.immediate_context)));
        //VHR(temp_d3d_context->QueryInterface(&D3D11_IID_ID3DUserDefinedAnnotation, (void**)&renderer->d3d_annotation));
        temp_d3d_context->Release();
        temp_d3d_device->Release();
    }

    /* Init caps first. */
    {
        DXGI_ADAPTER_DESC1 adapter_desc;
        VHR(dxgi_adapter->GetDesc1(&adapter_desc));

        /* Log some info */
        vgpu_log_info("vgpu driver: D3D11");
        vgpu_log_info("Direct3D Adapter: VID:%04X, PID:%04X - %ls", adapter_desc.VendorId, adapter_desc.DeviceId, adapter_desc.Description);

        d3d11.caps.backend_type = VGPU_BACKEND_TYPE_D3D11;
        d3d11.caps.vendor_id = adapter_desc.VendorId;
        d3d11.caps.device_id = adapter_desc.DeviceId;

        d3d11.caps.features.independent_blend = (d3d11.feature_level >= D3D_FEATURE_LEVEL_10_0);
        d3d11.caps.features.computeShader = (d3d11.feature_level >= D3D_FEATURE_LEVEL_10_0);
        d3d11.caps.features.tessellationShader = (d3d11.feature_level >= D3D_FEATURE_LEVEL_11_0);
        d3d11.caps.features.multiViewport = true;
        d3d11.caps.features.indexUInt32 = true;
        d3d11.caps.features.multiDrawIndirect = (d3d11.feature_level >= D3D_FEATURE_LEVEL_11_0);
        d3d11.caps.features.fillModeNonSolid = true;
        d3d11.caps.features.samplerAnisotropy = true;
        d3d11.caps.features.textureCompressionETC2 = false;
        d3d11.caps.features.textureCompressionASTC_LDR = false;
        d3d11.caps.features.textureCompressionBC = true;
        d3d11.caps.features.textureCubeArray = (d3d11.feature_level >= D3D_FEATURE_LEVEL_10_1);
        d3d11.caps.features.raytracing = false;

        // Limits
        d3d11.caps.limits.max_vertex_attributes = VGPU_MAX_VERTEX_ATTRIBUTES;
        d3d11.caps.limits.max_vertex_bindings = VGPU_MAX_VERTEX_ATTRIBUTES;
        d3d11.caps.limits.max_vertex_attribute_offset = VGPU_MAX_VERTEX_ATTRIBUTE_OFFSET;
        d3d11.caps.limits.max_vertex_binding_stride = VGPU_MAX_VERTEX_BUFFER_STRIDE;

        d3d11.caps.limits.max_texture_size_1d = D3D11_REQ_TEXTURE1D_U_DIMENSION;
        d3d11.caps.limits.max_texture_size_2d = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
        d3d11.caps.limits.max_texture_size_3d = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
        d3d11.caps.limits.max_texture_size_cube = D3D11_REQ_TEXTURECUBE_DIMENSION;
        d3d11.caps.limits.max_texture_array_layers = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
        d3d11.caps.limits.max_color_attachments = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
        d3d11.caps.limits.max_uniform_buffer_size = D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
        d3d11.caps.limits.min_uniform_buffer_offset_alignment = 256u;
        d3d11.caps.limits.max_storage_buffer_size = UINT32_MAX;
        d3d11.caps.limits.min_storage_buffer_offset_alignment = 16;
        d3d11.caps.limits.max_sampler_anisotropy = D3D11_MAX_MAXANISOTROPY;
        d3d11.caps.limits.max_viewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        d3d11.caps.limits.max_viewport_width = D3D11_VIEWPORT_BOUNDS_MAX;
        d3d11.caps.limits.max_viewport_height = D3D11_VIEWPORT_BOUNDS_MAX;
        d3d11.caps.limits.max_tessellation_patch_size = D3D11_IA_PATCH_MAX_CONTROL_POINT_COUNT;
        d3d11.caps.limits.point_size_range_min = 1.0f;
        d3d11.caps.limits.point_size_range_max = 1.0f;
        d3d11.caps.limits.line_width_range_min = 1.0f;
        d3d11.caps.limits.line_width_range_max = 1.0f;
        d3d11.caps.limits.max_compute_shared_memory_size = D3D11_CS_THREAD_LOCAL_TEMP_REGISTER_POOL;
        d3d11.caps.limits.max_compute_work_group_count_x = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        d3d11.caps.limits.max_compute_work_group_count_y = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        d3d11.caps.limits.max_compute_work_group_count_z = D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        d3d11.caps.limits.max_compute_work_group_invocations = D3D11_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
        d3d11.caps.limits.max_compute_work_group_size_x = D3D11_CS_THREAD_GROUP_MAX_X;
        d3d11.caps.limits.max_compute_work_group_size_y = D3D11_CS_THREAD_GROUP_MAX_Y;
        d3d11.caps.limits.max_compute_work_group_size_z = D3D11_CS_THREAD_GROUP_MAX_Z;

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
    dxgi_adapter->Release();

    if (info->swapchain.native_handle) {
        d3d11.main_swapchain = vgpu_create_framebuffer_swapchain(&info->swapchain);
    }

    return true;
}

static void d3d11_shutdown(void) {
    if (d3d11.main_swapchain) {
        vgpu_destroy_framebuffer(d3d11.main_swapchain);
    }

    SAFE_RELEASE(d3d11.immediate_context);

    ULONG refCount = d3d11.device->Release();
#if !defined(NDEBUG)
    if (refCount > 0) {
        vgpu_log_error("Direct3D11: There are %d unreleased references left on the device", refCount);

        ID3D11Debug* d3d11_debug = NULL;
        if (SUCCEEDED(d3d11.device->QueryInterface(IID_PPV_ARGS(&d3d11_debug)))) {
            d3d11_debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
            d3d11_debug->Release();
        }
    }
#else
    (void)refCount; // avoid warning
#endif

    // Release factory at last.
    SAFE_RELEASE(d3d11.factory);

#ifdef _DEBUG
    {
        IDXGIDebug1* dxgiDebug;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (d3d11.dxgi.DXGIGetDebugInterface1 && SUCCEEDED(d3d11.dxgi.DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
#else
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
#endif
        {
            dxgiDebug->ReportLiveObjects(vgpu_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            dxgiDebug->Release();
        }
    }
#endif

    memset(&d3d11, 0, sizeof(d3d11));
}

static vgpu_caps d3d11_query_caps(void) {
    return d3d11.caps;
}

static void d3d11_begin_frame(void) {
}

static void d3d11_end_frame(void) {
    if (d3d11.is_lost)
        return;

    HRESULT hr = S_OK;


    if (d3d11.main_swapchain) {
        d3d11_framebuffer* framebuffer = (d3d11_framebuffer*)d3d11.main_swapchain;
        // Present main swapchain with vsync on
        hr = framebuffer->swapchain->Present(1, 0);
    }

    if (!d3d11.factory->IsCurrent())
    {
        // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
        d3d11_create_factory();
    }
}


/* Texture */
static vgpu_texture d3d11_texture_create(const vgpu_texture_info* info) {
    d3d11_texture* texture = VGPU_ALLOC_HANDLE(d3d11_texture);

    if (info->external_handle) {
        texture->handle = (ID3D11Resource*)info->external_handle;
    }
    else {
        DXGI_FORMAT dxgi_format = _vgpu_d3d_format_with_usage(info->format, info->usage);

        D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
        UINT d3d11_bind_flags = 0;
        UINT CPUAccessFlags = 0;
        UINT d3d11_misc_flags = 0;
        uint32_t array_size = info->array_layers;

        if (info->mip_levels == 0)
        {
            d3d11_bind_flags |= D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
            d3d11_misc_flags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
        }

        if (info->usage & VGPU_TEXTURE_USAGE_SAMPLED) {
            d3d11_bind_flags |= D3D11_BIND_SHADER_RESOURCE;
        }

        if (info->usage & VGPU_TEXTURE_USAGE_STORAGE) {
            d3d11_bind_flags |= D3D11_BIND_UNORDERED_ACCESS;
        }

        if (info->type == VGPU_TEXTURE_TYPE_CUBE) {
            array_size *= 6;
            d3d11_misc_flags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
        }

        HRESULT hr = S_OK;
        if (info->type == VGPU_TEXTURE_TYPE_2D || info->type == VGPU_TEXTURE_TYPE_CUBE) {

            D3D11_TEXTURE2D_DESC d3d11_desc = {};
            d3d11_desc.Width = _vgpu_align_to(vgpu_get_format_width_compression_ratio(info->format), info->width);
            d3d11_desc.Height = _vgpu_align_to(vgpu_get_format_width_compression_ratio(info->format), info->height);
            d3d11_desc.MipLevels = info->mip_levels;
            d3d11_desc.ArraySize = array_size;
            d3d11_desc.Format = dxgi_format;
            d3d11_desc.SampleDesc.Count = 1;
            d3d11_desc.SampleDesc.Quality = 0;
            d3d11_desc.Usage = usage;
            d3d11_desc.BindFlags = d3d11_bind_flags;
            d3d11_desc.CPUAccessFlags = CPUAccessFlags;
            d3d11_desc.MiscFlags = d3d11_misc_flags;

            hr = d3d11.device->CreateTexture2D(&d3d11_desc, nullptr, (ID3D11Texture2D**)&texture->handle);
        }

        if (FAILED(hr)) {
            VGPU_FREE(texture);
            return nullptr;
        }
    }

    if (info->usage & VGPU_TEXTURE_USAGE_RENDER_TARGET) {
        if (vgpu_is_depth_stencil_format(info->format)) {

        }
        else {
            // Create a render target view for each mip
            D3D11_TEXTURE2D_DESC d3d11_desc;
            ((ID3D11Texture2D*)texture->handle)->GetDesc(&d3d11_desc);

            for (uint32_t mip_index = 0; mip_index < d3d11_desc.MipLevels; mip_index++)
            {
                D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
                memset(&rtv_desc, 0, sizeof(rtv_desc));
            }
        }
    }

    return (vgpu_texture)texture;
}

static void d3d11_texture_destroy(vgpu_texture handle) {
    d3d11_texture* texture = (d3d11_texture*)handle;
    SAFE_RELEASE(texture->handle);
    VGPU_FREE(texture);
}

/* Framebuffer */
static vgpu_framebuffer d3d11_create_framebuffer(const vgpu_framebuffer_info* info) {
    d3d11_framebuffer* framebuffer = VGPU_ALLOC(d3d11_framebuffer);
    framebuffer->destroy_textures = false;
    return (vgpu_framebuffer)framebuffer;
}

static vgpu_framebuffer d3d11_create_framebuffer_swapchain(const vgpu_swapchain_info* info) {
    d3d11_framebuffer* framebuffer = VGPU_ALLOC_HANDLE(d3d11_framebuffer);
    framebuffer->destroy_textures = true;

    framebuffer->swapchain = vgpu_d3d_create_swapchain(
        d3d11.factory, d3d11.factory_caps,
        d3d11.device,
        info->native_handle,
        info->width, info->height,
        info->color_format,
        d3d11.num_backbuffers,
        info->is_fullscreen
    );

    ID3D11Texture2D* backbuffer;
    VHR(framebuffer->swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer)));

    vgpu_texture_info texture_info = {};
    texture_info.type = VGPU_TEXTURE_TYPE_2D;
    texture_info.format = info->color_format;
    texture_info.width = info->width;
    texture_info.height = info->height;
    texture_info.usage = VGPU_TEXTURE_USAGE_RENDER_TARGET;
    texture_info.external_handle = (uintptr_t)backbuffer;
    framebuffer->color_textures[0] = vgpu_create_texture(&texture_info);

    return (vgpu_framebuffer)framebuffer;
}

static void d3d11_destroy_framebuffer(vgpu_framebuffer handle) {
    d3d11_framebuffer* framebuffer = (d3d11_framebuffer*)handle;
    if (framebuffer->destroy_textures) {
        for (uint32_t i = 0; i < VGPU_MAX_COLOR_ATTACHMENTS; i++) {
            if (framebuffer->color_textures[i]) {
                vgpu_destroy_texture(framebuffer->color_textures[i]);
            }
        }

        if (framebuffer->depth_stencil_texture) {
            vgpu_destroy_texture(framebuffer->depth_stencil_texture);
        }
    }

    SAFE_RELEASE(framebuffer->swapchain);
    VGPU_FREE(framebuffer);
}

/* Driver */
static bool d3d11_is_supported(void) {
    if (d3d11.available_initialized) {
        return d3d11.available;
    }

    d3d11.available_initialized = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
    d3d11.dxgi.instance = LoadLibraryA("dxgi.dll");
    if (!d3d11.dxgi.instance) {
        return false;
    }

    d3d11.dxgi.CreateDXGIFactory1 = (PFN_CREATE_DXGI_FACTORY1)GetProcAddress(d3d11.dxgi.instance, "CreateDXGIFactory1");
    d3d11.dxgi.CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(d3d11.dxgi.instance, "CreateDXGIFactory2");
    if (!d3d11.dxgi.CreateDXGIFactory2)
    {
        return false;
    }
    d3d11.dxgi.DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(d3d11.dxgi.instance, "DXGIGetDebugInterface1");

    d3d11.instance = LoadLibraryA("d3d11.dll");
    if (!d3d11.instance) {
        return false;
    }

    d3d11.D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11.instance, "D3D11CreateDevice");
    if (d3d11.D3D11CreateDevice == nullptr) {
        return false;
    }

#endif

    // Determine DirectX hardware feature levels this app will support.
    static const D3D_FEATURE_LEVEL s_featureLevels[] =
    {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };

    HRESULT hr = _vgpu_D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        s_featureLevels,
        _countof(s_featureLevels),
        D3D11_SDK_VERSION,
        nullptr,
        nullptr,
        nullptr
    );

    if (FAILED(hr))
    {
        return false;
    }

    d3d11.available = true;
    return true;
};

static vgpu_renderer* d3d11_init_renderer(void) {
    static vgpu_renderer renderer = { nullptr };
    renderer.init = d3d11_init;
    renderer.shutdown = d3d11_shutdown;
    renderer.query_caps = d3d11_query_caps;
    renderer.begin_frame = d3d11_begin_frame;
    renderer.end_frame = d3d11_end_frame;

    renderer.create_texture = d3d11_texture_create;
    renderer.texture_destroy = d3d11_texture_destroy;

    renderer.create_framebuffer = d3d11_create_framebuffer;
    renderer.create_framebuffer_swapchain = d3d11_create_framebuffer_swapchain;
    renderer.destroy_framebuffer = d3d11_destroy_framebuffer;

    /*renderer.get_backbuffer_texture = d3d11_get_backbuffer_texture;
    renderer.begin_pass = d3d11_begin_pass;
    renderer.end_pass = d3d11_end_pass;*/

    return &renderer;
}

vgpu_driver d3d11_driver = {
    VGPU_BACKEND_TYPE_D3D11,
    d3d11_is_supported,
    d3d11_init_renderer
};

#endif /* defined(VGPU_DRIVER_D3D12) */
