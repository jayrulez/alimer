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

#if defined(AGPU_DRIVER_D3D11)

#define D3D11_NO_HELPERS
#define CINTERFACE
#define COBJMACROS
#include <d3d11_1.h>
#include "agpu_driver_d3d_common.h"
//#include "stb_ds.h"

/* D3D11 guids */
static const IID D3D11_IID_ID3D11DeviceContext1 = { 0xbb2c6faa, 0xb5fb, 0x4082, {0x8e, 0x6b, 0x38, 0x8b, 0x8c, 0xfa, 0x90, 0xe1} };
static const IID D3D11_IID_ID3D11Device1 = { 0xa04bfb29, 0x08ef, 0x43d6, {0xa4, 0x9c, 0xa9, 0xbd, 0xbd, 0xcb, 0xe6, 0x86} };
static const IID D3D11_IID_ID3DUserDefinedAnnotation = { 0xb2daad8b, 0x03d4, 0x4dbf, {0x95, 0xeb, 0x32, 0xab, 0x4b, 0x63, 0xd0, 0xab} };
static const IID D3D11_IID_ID3D11Texture2D = { 0x6f15aaf2,0xd208,0x4e89, {0x9a,0xb4,0x48,0x95,0x35,0xd3,0x4f,0x9c } };

#ifndef NDEBUG
static const IID D3D11_WKPDID_D3DDebugObjectName = { 0x429b8c22, 0x9188, 0x4b0c, { 0x87,0x42,0xac,0xb0,0xbf,0x85,0xc2,0x00 } };
static const IID D3D11_IID_ID3D11Debug = { 0x79cf2233, 0x7536, 0x4948, {0x9d, 0x36, 0x1e, 0x46, 0x92, 0xdc, 0x57, 0x60} };
static const IID D3D11_IID_ID3D11InfoQueue = { 0x6543dbb6, 0x1b48, 0x42f5, {0xab,0x82,0xe9,0x7e,0xc7,0x43,0x26,0xf6} };
#endif

typedef struct d3d11_context {
    uint32_t width;
    uint32_t height;
    agpu_texture_format color_format;

    IDXGISwapChain1* swapchain;
    uint32_t sync_interval;
    uint32_t present_flags;
    // HDR Support
    DXGI_COLOR_SPACE_TYPE color_space;

    ID3D11RenderTargetView* rtv;
    ID3D11DepthStencilView* dsv;
} d3d11_context;

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

    agpu_device_caps caps;

    bool debug;
    IDXGIFactory2* factory;
    uint32_t factory_caps;

    ID3D11Device1* device;
    ID3D11DeviceContext1* context;
    ID3DUserDefinedAnnotation* d3d_annotation;
    D3D_FEATURE_LEVEL feature_level;
    bool is_lost;
} d3d11;

AGPU_THREADLOCAL d3d11_context* d3d11_current_context = NULL;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#   define _agpu_CreateDXGIFactory1 d3d11.dxgi.CreateDXGIFactory1
#   define _agpu_CreateDXGIFactory2 d3d11.dxgi.CreateDXGIFactory2
#   define _agpu_D3D11CreateDevice d3d11.D3D11CreateDevice
#else
#   define _agpu_CreateDXGIFactory1 CreateDXGIFactory1
#   define _agpu_CreateDXGIFactory2 CreateDXGIFactory2
#   define _agpu_D3D11CreateDevice D3D11CreateDevice
#endif


/* Device/Renderer */
static bool _agpu_d3d11_sdk_layers_available() {
    HRESULT hr = _agpu_D3D11CreateDevice(
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

static bool d3d11_create_factory(void)
{
    if (d3d11.factory) {
        IDXGIFactory2_Release(d3d11.factory);
        d3d11.factory = NULL;
    }

    HRESULT hr = S_OK;

#if defined(_DEBUG)
    bool debugDXGI = false;

    if (d3d11.debug)
    {
        IDXGIInfoQueue* dxgi_info_queue;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (d3d11.dxgi.DXGIGetDebugInterface1 &&
            SUCCEEDED(d3d11.dxgi.DXGIGetDebugInterface1(0, &D3D_IID_IDXGIInfoQueue, (void**)&dxgi_info_queue)))
#else
        if (SUCCEEDED(DXGIGetDebugInterface1(0, &D3D_IID_IDXGIInfoQueue, (void**)&dxgi_info_queue)))
#endif
        {
            debugDXGI = true;
            hr = _agpu_CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, &D3D_IID_IDXGIFactory2, (void**)&d3d11.factory);
            if (FAILED(hr)) {
                return false;
            }

            VHR(IDXGIInfoQueue_SetBreakOnSeverity(dxgi_info_queue, D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE));
            VHR(IDXGIInfoQueue_SetBreakOnSeverity(dxgi_info_queue, D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE));
            VHR(IDXGIInfoQueue_SetBreakOnSeverity(dxgi_info_queue, D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, FALSE));

            DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
            {
                80 // IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides.
            };

            DXGI_INFO_QUEUE_FILTER filter;
            memset(&filter, 0, sizeof(filter));
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            IDXGIInfoQueue_AddStorageFilterEntries(dxgi_info_queue, D3D_DXGI_DEBUG_DXGI, &filter);
            IDXGIInfoQueue_Release(dxgi_info_queue);
        }
    }

    if (!debugDXGI)
#endif
    {
        hr = _agpu_CreateDXGIFactory1(&D3D_IID_IDXGIFactory2, (void**)&d3d11.factory);
        if (FAILED(hr)) {
            return false;
        }
    }

    d3d11.factory_caps = 0;

    // Disable FLIP if not on a supporting OS
    d3d11.factory_caps |= DXGIFACTORY_CAPS_FLIP_PRESENT;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    {
        IDXGIFactory4* factory4;
        HRESULT hr = IDXGIFactory2_QueryInterface(d3d11.factory, &D3D_IID_IDXGIFactory4, (void**)&factory4);
        if (FAILED(hr))
        {
            d3d11.factory_caps &= DXGIFACTORY_CAPS_FLIP_PRESENT;
        }
        IDXGIFactory4_Release(factory4);
    }
#endif

    // Check tearing support.
    {
        BOOL allowTearing = FALSE;
        IDXGIFactory5* factory5;
        HRESULT hr = IDXGIFactory2_QueryInterface(d3d11.factory, &D3D_IID_IDXGIFactory5, (void**)&factory5);
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
            d3d11.factory_caps |= DXGIFACTORY_CAPS_TEARING;
        }
    }

    return true;
}
static IDXGIAdapter1* d3d11_get_adapter(agpu_device_preference device_preference)
{
    /* Detect adapter now. */
    IDXGIAdapter1* adapter = NULL;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
    IDXGIFactory6* factory6;
    HRESULT hr = IDXGIFactory2_QueryInterface(d3d11.factory, &D3D_IID_IDXGIFactory6, (void**)&factory6);
    if (SUCCEEDED(hr))
    {
        // By default prefer high performance
        DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
        if (device_preference == AGPU_DEVICE_PREFERENCE_LOW_POWER) {
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

            break;
        }

        IDXGIFactory6_Release(factory6);
    }
#endif

    if (!adapter)
    {
        for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != IDXGIFactory2_EnumAdapters1(d3d11.factory, i, &adapter); i++)
        {
            DXGI_ADAPTER_DESC1 desc;
            VHR(IDXGIAdapter1_GetDesc1(adapter, &desc));

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                IDXGIAdapter1_Release(adapter);

                continue;
            }

            break;
        }
    }

    return adapter;
}

static bool d3d11_init(const agpu_init_info* info)
{
    d3d11.debug = info->debug;

    if (!d3d11_create_factory()) {
        return false;
    }

    IDXGIAdapter1* dxgi_adapter = d3d11_get_adapter(info->device_preference);

    /* Create d3d11 device */
    {
        UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

        if (d3d11.debug && _agpu_d3d11_sdk_layers_available())
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
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0
        };

        // Create the Direct3D 11 API device object and a corresponding context.
        ID3D11Device* temp_d3d_device;
        ID3D11DeviceContext* temp_d3d_context;

        HRESULT hr = E_FAIL;
        if (dxgi_adapter)
        {
            hr = _agpu_D3D11CreateDevice(
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
            agpu_log_error("No Direct3D hardware device found");
            AGPU_UNREACHABLE();
        }
#else
        if (FAILED(hr))
        {
            // If the initialization fails, fall back to the WARP device.
            // For more information on WARP, see:
            // http://go.microsoft.com/fwlink/?LinkId=286690
            hr = _agpu_D3D11CreateDevice(
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
        if (SUCCEEDED(ID3D11Device_QueryInterface(temp_d3d_device, &D3D11_IID_ID3D11Debug, (void**)&d3d_debug)))
        {
            ID3D11InfoQueue* d3d_info_queue;
            if (SUCCEEDED(ID3D11Debug_QueryInterface(d3d_debug, &D3D11_IID_ID3D11InfoQueue, (void**)&d3d_info_queue)))
            {
#ifdef _DEBUG
                ID3D11InfoQueue_SetBreakOnSeverity(d3d_info_queue, D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
                ID3D11InfoQueue_SetBreakOnSeverity(d3d_info_queue, D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
                D3D11_MESSAGE_ID hide[] =
                {
                    D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
                };
                D3D11_INFO_QUEUE_FILTER filter;
                memset(&filter, 0, sizeof(D3D11_INFO_QUEUE_FILTER));
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                ID3D11InfoQueue_AddStorageFilterEntries(d3d_info_queue, &filter);
                ID3D11InfoQueue_Release(d3d_info_queue);
            }

            ID3D11Debug_Release(d3d_debug);
        }
#endif

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        /*{
            IDXGIDevice1* dxgiDevice1;
            if (SUCCEEDED(ID3D11Device_QueryInterface(temp_d3d_device, &D3D_IID_IDXGIDevice1, (void**)&dxgiDevice1)))
            {
                // Default to 3.
                VHR(IDXGIDevice1_SetMaximumFrameLatency(dxgiDevice1, renderer->max_frame_latency));
                IDXGIDevice1_Release(dxgiDevice1);
            }
        }*/
#endif

        VHR(ID3D11Device_QueryInterface(temp_d3d_device, &D3D11_IID_ID3D11Device1, (void**)&d3d11.device));
        VHR(ID3D11DeviceContext_QueryInterface(temp_d3d_context, &D3D11_IID_ID3D11DeviceContext1, (void**)&d3d11.context));
        VHR(ID3D11DeviceContext_QueryInterface(temp_d3d_context, &D3D11_IID_ID3DUserDefinedAnnotation, (void**)&d3d11.d3d_annotation));
        ID3D11DeviceContext_Release(temp_d3d_context);
        ID3D11Device_Release(temp_d3d_device);
    }

    /* Init caps first. */
    {
        DXGI_ADAPTER_DESC1 adapter_desc;
        VHR(IDXGIAdapter1_GetDesc1(dxgi_adapter, &adapter_desc));

        /* Log some info */
        agpu_log_info("GPU driver: D3D11");
        agpu_log_info("Direct3D Adapter: VID:%04X, PID:%04X - %ls", adapter_desc.VendorId, adapter_desc.DeviceId, adapter_desc.Description);

        d3d11.caps.backend_type = AGPU_BACKEND_TYPE_D3D11;
        d3d11.caps.vendor_id = adapter_desc.VendorId;
        d3d11.caps.device_id = adapter_desc.DeviceId;

        d3d11.caps.features.independent_blend = (d3d11.feature_level >= D3D_FEATURE_LEVEL_10_0);
        d3d11.caps.features.compute_shader = (d3d11.feature_level >= D3D_FEATURE_LEVEL_10_0);
        d3d11.caps.features.tessellation_shader = (d3d11.feature_level >= D3D_FEATURE_LEVEL_11_0);
        d3d11.caps.features.multi_viewport = true;
        d3d11.caps.features.index_uint32 = true;
        d3d11.caps.features.multi_draw_indirect = (d3d11.feature_level >= D3D_FEATURE_LEVEL_11_0);
        d3d11.caps.features.fill_mode_non_solid = true;
        d3d11.caps.features.sampler_anisotropy = true;
        d3d11.caps.features.texture_compression_ETC2 = false;
        d3d11.caps.features.texture_compression_ASTC_LDR = false;
        d3d11.caps.features.texture_compression_BC = true;
        d3d11.caps.features.texture_cube_array = (d3d11.feature_level >= D3D_FEATURE_LEVEL_10_1);
        d3d11.caps.features.raytracing = false;

        // Limits
        d3d11.caps.limits.max_vertex_attributes = AGPU_MAX_VERTEX_ATTRIBUTES;
        d3d11.caps.limits.max_vertex_bindings = AGPU_MAX_VERTEX_ATTRIBUTES;
        d3d11.caps.limits.max_vertex_attribute_offset = AGPU_MAX_VERTEX_ATTRIBUTE_OFFSET;
        d3d11.caps.limits.max_vertex_binding_stride = AGPU_MAX_VERTEX_BUFFER_STRIDE;

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
    IDXGIAdapter1_Release(dxgi_adapter);

    return true;
}

static void d3d11_shutdown(void)
{
    if (d3d11_current_context) {
        agpu_destroy_context((agpu_context)d3d11_current_context);
    }

    ID3D11DeviceContext1_Release(d3d11.context);
    ID3DUserDefinedAnnotation_Release(d3d11.d3d_annotation);

    ULONG refCount = ID3D11Device1_Release(d3d11.device);
#if !defined(NDEBUG)
    if (refCount > 0)
    {
        agpu_log_error("Direct3D11: There are %d unreleased references left on the device", refCount);

        ID3D11Debug* d3d11_debug = NULL;
        if (SUCCEEDED(ID3D11Device1_QueryInterface(d3d11.device, &D3D11_IID_ID3D11Debug, (void**)&d3d11_debug)))
        {
            ID3D11Debug_ReportLiveDeviceObjects(d3d11_debug, D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
            ID3D11Debug_Release(d3d11_debug);
        }
    }
#else
    (void)refCount; // avoid warning
#endif


    IDXGIFactory2_Release(d3d11.factory);

#ifdef _DEBUG
    IDXGIDebug1* dxgi_debug1;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    if (d3d11.dxgi.DXGIGetDebugInterface1 &&
        SUCCEEDED(d3d11.dxgi.DXGIGetDebugInterface1(0, &D3D_IID_IDXGIDebug1, (void**)&dxgi_debug1)))
#else
    if (SUCCEEDED(DXGIGetDebugInterface1(0, &D3D_IID_IDXGIDebug1, (void**)&dxgi_debug1)))
#endif
    {
        IDXGIDebug1_ReportLiveObjects(dxgi_debug1, D3D_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL);
        IDXGIDebug1_Release(dxgi_debug1);
    }
#endif
}

static void d3d11_update_color_space(d3d11_context* context)
{
    context->color_space = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;

    bool isDisplayHDR10 = false;

#if defined(NTDDI_WIN10_RS2)
    IDXGIOutput* output;
    if (SUCCEEDED(IDXGISwapChain1_GetContainingOutput(context->swapchain, &output)))
    {
        IDXGIOutput6* output6;
        if (SUCCEEDED(IDXGIOutput_QueryInterface(output, &D3D_IID_IDXGIOutput6, (void**)&output6)))
        {
            DXGI_OUTPUT_DESC1 desc;
            VHR(IDXGIOutput6_GetDesc1(output6, &desc));

            if (desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
            {
                // Display output is HDR10.
                isDisplayHDR10 = true;
            }

            IDXGIOutput6_Release(output6);
        }

        IDXGIOutput_Release(output);
    }
#endif

    if (isDisplayHDR10)
    {
        switch (context->color_format)
        {
        case AGPU_TEXTURE_FORMAT_RGB10A2_UNORM:
            // The application creates the HDR10 signal.
            context->color_space = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
            break;

        case AGPU_TEXTURE_FORMAT_RGBA32_FLOAT:
            // The system creates the HDR10 signal; application uses linear values.
            context->color_space = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
            break;

        default:
            break;
        }
    }

    IDXGISwapChain3* swapChain3;
    if (SUCCEEDED(IDXGISwapChain1_QueryInterface(context->swapchain, &D3D_IID_IDXGISwapChain3, (void**)&swapChain3)))
    {
        UINT colorSpaceSupport = 0;
        if (SUCCEEDED(IDXGISwapChain3_CheckColorSpaceSupport(swapChain3, context->color_space, &colorSpaceSupport))
            && (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
        {
            VHR(IDXGISwapChain3_SetColorSpace1(swapChain3, context->color_space));
        }

        IDXGISwapChain3_Release(swapChain3);
    }
}

static void d3d11_after_reset(d3d11_context* context)
{
    d3d11_update_color_space(context);

    DXGI_SWAP_CHAIN_DESC1 swapchain_desc;
    VHR(IDXGISwapChain1_GetDesc1(context->swapchain, &swapchain_desc));
    context->width = swapchain_desc.Width;
    context->height = swapchain_desc.Height;

    /* TODO: Until we support textures. */
    ID3D11Texture2D* backbuffer;
    VHR(IDXGISwapChain1_GetBuffer(context->swapchain, 0, &D3D11_IID_ID3D11Texture2D, (void**)&backbuffer));

    ID3D11Device1_CreateRenderTargetView(d3d11.device, (ID3D11Resource*)backbuffer, NULL, &context->rtv);
    ID3D11Texture2D_Release(backbuffer);
}

static agpu_context d3d11_create_context(const agpu_context_info* info) {
    d3d11_context* context = AGPU_ALLOC_HANDLE(d3d11_context);
    context->color_space = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;

    // Create swapchain if required.
    if (info->swapchain_info)
    {
        context->swapchain = _agpu_d3d_create_swapchain(
            d3d11.factory, d3d11.factory_caps,
            (IUnknown*)d3d11.device,
            info->swapchain_info->window_handle,
            info->swapchain_info->width, info->swapchain_info->height,
            info->swapchain_info->color_format,
            2, // renderer->num_backbuffers,  /* 3 for triple buffering */
            info->swapchain_info->fullscreen);

        context->width = info->swapchain_info->width;
        context->height = info->swapchain_info->height;
        context->color_format = info->swapchain_info->color_format;

        context->sync_interval = 1;
        context->present_flags = 0;
        if (!info->swapchain_info->vsync)
        {
            context->sync_interval = 0;
            if (d3d11.factory_caps & DXGIFACTORY_CAPS_TEARING) {
                context->present_flags |= DXGI_PRESENT_ALLOW_TEARING;
            }
        }

        d3d11_after_reset(context);
    }
    else
    {
        // TODO: Create offscreen context.
    }

    // Set as active context (if none)
    if (!d3d11_current_context) {
        d3d11_current_context = context;
    }

    return (agpu_context)context;
}

static void d3d11_destroy_context(agpu_context handle) {
    d3d11_context* context = (d3d11_context*)handle;

    if (context->swapchain)
    {
        IDXGISwapChain1_Release(context->swapchain);
    }

    if (context->rtv)
    {
        ID3D11RenderTargetView_Release(context->rtv);
    }

    if (d3d11_current_context == context) {
        d3d11_current_context = NULL;
    }

    agpu_free(context);
}

static void d3d11_set_context(agpu_context context) {
    d3d11_current_context = (d3d11_context*)context;
}

static void d3d11_begin_frame(void) {
    AGPU_ASSERT(d3d11_current_context);

    float clear_color[4] = { 0.392156899f, 0.584313750f, 0.929411829f, 1.0f };

    ID3D11DeviceContext1_ClearRenderTargetView(d3d11.context, d3d11_current_context->rtv, clear_color);
    //ID3D11DeviceContext1_ClearDepthStencilView(d3d11.context, d3d11_current_context->dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    ID3D11DeviceContext1_OMSetRenderTargets(d3d11.context, 1, &d3d11_current_context->rtv, d3d11_current_context->dsv);

    // Set the viewport.
    //auto viewport = m_deviceResources->GetScreenViewport();
    //context->RSSetViewports(1, &viewport);
}

static void d3d11_end_frame(void)
{
    AGPU_ASSERT(d3d11_current_context);

    if (d3d11_current_context->swapchain)
    {
        HRESULT hr = IDXGISwapChain1_Present(d3d11_current_context->swapchain, d3d11_current_context->sync_interval, d3d11_current_context->present_flags);
        if (hr == DXGI_ERROR_DEVICE_REMOVED
            || hr == DXGI_ERROR_DEVICE_HUNG
            || hr == DXGI_ERROR_DEVICE_RESET
            || hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR
            || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
        {
            d3d11.is_lost = true;
        }
    }

    if (!d3d11.is_lost)
    {
        if (!IDXGIFactory2_IsCurrent(d3d11.factory))
        {
            // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
            d3d11_create_factory();
        }
    }
}

static agpu_device_caps d3d11_query_caps(void) {
    return d3d11.caps;
}

static agpu_texture_format_info d3d11_query_texture_format_info(agpu_texture_format format) {
    agpu_texture_format_info info;
    memset(&info, 0, sizeof(info));
    return info;
}

/* Driver */
static bool d3d11_is_supported(void)
{
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
    if (!d3d11.dxgi.CreateDXGIFactory1)
    {
        return false;
    }

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
    if (!d3d11.D3D11CreateDevice) {
        return false;
    }
#endif

    static const D3D_FEATURE_LEVEL s_featureLevels[] =
    {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };

    HRESULT hr = _agpu_D3D11CreateDevice(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        s_featureLevels,
        AGPU_COUNT_OF(s_featureLevels),
        D3D11_SDK_VERSION,
        NULL,
        NULL,
        NULL
    );

    if (FAILED(hr))
    {
        return false;
    }

    d3d11.available = true;
    return true;
};

static agpu_renderer* d3d11_create_renderer(void)
{
    static agpu_renderer renderer = { 0 };
    ASSIGN_DRIVER(d3d11);
    return &renderer;
}

agpu_driver d3d11_driver = {
    AGPU_BACKEND_TYPE_D3D11,
    d3d11_is_supported,
    d3d11_create_renderer
};

#endif /* defined(AGPU_DRIVER_D3D11)  */
