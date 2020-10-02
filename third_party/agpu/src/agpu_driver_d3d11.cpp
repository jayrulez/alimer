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

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define D3D11_NO_HELPERS
#include <d3d11_3.h>
#include "agpu_driver_d3d_common.h"
//#include "stb_ds.h"
#include <mutex>

#ifndef NDEBUG
static const IID D3D11_WKPDID_D3DDebugObjectName = { 0x429b8c22, 0x9188, 0x4b0c, { 0x87,0x42,0xac,0xb0,0xbf,0x85,0xc2,0x00 } };
#endif

struct d3d11_swapchain
{
    IDXGISwapChain1* handle;
    uint32_t width;
    uint32_t height;
    agpu_texture_format color_format;
    bool is_fullscreen;

    // HDR Support
    DXGI_COLOR_SPACE_TYPE colorSpace;

    UINT sync_interval;
    UINT present_flags;

    agpu_texture backbufferTexture;
    agpu_texture depthStencilTexture;
};

struct d3d11_buffer
{
    ID3D11Buffer* handle;
};

struct D3D11Texture
{
    union {
        ID3D11Resource* handle;
        ID3D11Texture2D* tex2D;
        ID3D11Texture3D* tex3D;
    };

    union {
        ID3D11RenderTargetView* rtv;
        ID3D11DepthStencilView* dsv;
    };
};

/* Global data */
static struct {
    bool available_initialized;
    bool available;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    HMODULE dxgiDLL;
    HMODULE d3d11DLL;

    PFN_CREATE_DXGI_FACTORY1 CreateDXGIFactory1 = nullptr;
    PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2 = nullptr;
    PFN_GET_DXGI_DEBUG_INTERFACE1 DXGIGetDebugInterface1 = nullptr;
    PFN_D3D11_CREATE_DEVICE D3D11CreateDevice = nullptr;
#endif

    agpu_config config;

    IDXGIFactory2* factory;
    DXGIFactoryCaps factoryCaps;
    bool isTearingSupported;

    ID3D11Device1* device;
    ID3D11DeviceContext1* context;
    ID3DUserDefinedAnnotation* annotation;
    D3D_FEATURE_LEVEL feature_level;
    bool is_lost;

    agpu_caps caps;

    /* Swapchain */
    d3d11_swapchain swapchain;

    /* Helper fields*/
    ID3D11RenderTargetView* zero_rtvs[AGPU_MAX_COLOR_ATTACHMENTS];
} d3d11;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#   define agpuCreateDXGIFactory1 d3d11.CreateDXGIFactory1
#   define agpuCreateDXGIFactory2 d3d11.CreateDXGIFactory2
#   define agpuD3D11CreateDevice d3d11.D3D11CreateDevice
#else
#   define agpuCreateDXGIFactory2 CreateDXGIFactory2
#   define agpuD3D11CreateDevice D3D11CreateDevice
#endif

#define GPU_THROW(s) if (d3d11.config.callback) { d3d11.config.callback(d3d11.config.context, AGPU_LOG_LEVEL_ERROR, s); }
#define GPU_CHECK(c, s) if (!(c)) { GPU_THROW(s); }

/* Device/Renderer */
static void _agpu_d3d11_init_swapchain(d3d11_swapchain* swapchain, const agpu_swapchain_info* info);
static void _agpu_d3d11_shutdown_swapchain(d3d11_swapchain* swapchain);

static bool _agpu_d3d11_sdk_layers_available() {
    HRESULT hr = agpuD3D11CreateDevice(
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

static bool d3d11_CreateFactory(void)
{
    SAFE_RELEASE(d3d11.factory);

    HRESULT hr = S_OK;

#if defined(_DEBUG)
    bool debugDXGI = false;

    if (d3d11.config.debug)
    {
        IDXGIInfoQueue* dxgiInfoQueue;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (d3d11.DXGIGetDebugInterface1 &&
            SUCCEEDED(d3d11.DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
#else
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
#endif
        {
            debugDXGI = true;
            hr = agpuCreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&d3d11.factory));
            if (FAILED(hr)) {
                return false;
            }

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

    if (!debugDXGI)
#endif
    {
        hr = agpuCreateDXGIFactory1(IID_PPV_ARGS(&d3d11.factory));
        if (FAILED(hr)) {
            return false;
        }
    }

    d3d11.factoryCaps = DXGIFactoryCaps::None;

    // Disable FLIP if not on a supporting OS
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    {
        IDXGIFactory4* factory4;
        HRESULT hr = d3d11.factory->QueryInterface(IID_PPV_ARGS(&factory4));
        if (SUCCEEDED(hr))
        {
            d3d11.factoryCaps |= DXGIFactoryCaps::FlipPresent;
        }
        else
        {
        }

        factory4->Release();
    }
#else
    d3d11.factoryCaps |= DXGIFactoryCaps::FlipPresent;
#endif

    // Check tearing support.
    {
        BOOL allowTearing = FALSE;
        IDXGIFactory5* factory5 = nullptr;
        HRESULT hr = d3d11.factory->QueryInterface(IID_PPV_ARGS(&factory5));
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
            d3d11.factoryCaps |= DXGIFactoryCaps::Tearing;
            d3d11.isTearingSupported = true;
        }
    }

    return true;
}

static IDXGIAdapter1* d3d11_get_adapter(bool lowPower)
{
    /* Detect adapter now. */
    IDXGIAdapter1* adapter = NULL;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
    IDXGIFactory6* factory6 = nullptr;
    HRESULT hr = d3d11.factory->QueryInterface(IID_PPV_ARGS(&factory6));
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

            break;
        }
    }

    SAFE_RELEASE(factory6);
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


static bool d3d11_init(const char* app_name, const agpu_config* config)
{
    memcpy(&d3d11.config, config, sizeof(d3d11.config));

    if (!d3d11_CreateFactory()) {
        return false;
    }

    const bool lowPower = false;
    IDXGIAdapter1* dxgiAdapter = d3d11_get_adapter(lowPower);

    /* Create d3d11 device */
    {
        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

        if (d3d11.config.debug && _agpu_d3d11_sdk_layers_available())
        {
            // If the project is in a debug build, enable debugging via SDK Layers with this flag.
            creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
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
        if (dxgiAdapter)
        {
            hr = agpuD3D11CreateDevice(
                (IDXGIAdapter*)dxgiAdapter,
                D3D_DRIVER_TYPE_UNKNOWN,
                NULL,
                creationFlags,
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
            GPU_THROW("No Direct3D hardware device found");
            AGPU_UNREACHABLE();
        }
#else
        if (FAILED(hr))
        {
            // If the initialization fails, fall back to the WARP device.
            // For more information on WARP, see:
            // http://go.microsoft.com/fwlink/?LinkId=286690
            hr = agpuD3D11CreateDevice(
                NULL,
                D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
                NULL,
                creationFlags,
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
        ID3D11Debug* d3dDebug;
        if (SUCCEEDED(temp_d3d_device->QueryInterface(IID_PPV_ARGS(&d3dDebug))))
        {
            ID3D11InfoQueue* d3dInfoQueue;
            if (SUCCEEDED(d3dDebug->QueryInterface(IID_PPV_ARGS(&d3dInfoQueue))))
            {
#ifdef _DEBUG
                d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
                d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
                D3D11_MESSAGE_ID hide[] =
                {
                    D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
                };
                D3D11_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                d3dInfoQueue->AddStorageFilterEntries(&filter);
                d3dInfoQueue->Release();
            }

            d3dDebug->Release();
        }
#endif

        VHR(temp_d3d_device->QueryInterface(IID_PPV_ARGS(&d3d11.device)));
        VHR(temp_d3d_context->QueryInterface(IID_PPV_ARGS(&d3d11.context)));
        VHR(temp_d3d_context->QueryInterface(IID_PPV_ARGS(&d3d11.annotation)));
        temp_d3d_context->Release();
        temp_d3d_device->Release();
    }

    /* Init caps first. */
    {
        DXGI_ADAPTER_DESC1 adapter_desc;
        VHR(dxgiAdapter->GetDesc1(&adapter_desc));

        /* Log some info */
        //log(LogLevel::Info, "GPU driver: D3D11");
        //log(LogLevel::Info, "Direct3D Adapter: VID:%04X, PID:%04X - %ls", adapter_desc.VendorId, adapter_desc.DeviceId, adapter_desc.Description);

        d3d11.caps.backend = AGPU_BACKEND_TYPE_D3D11;
        d3d11.caps.vendorID = adapter_desc.VendorId;
        d3d11.caps.deviceID = adapter_desc.DeviceId;

        d3d11.caps.features.independentBlend = (d3d11.feature_level >= D3D_FEATURE_LEVEL_10_0);
        d3d11.caps.features.computeShader = (d3d11.feature_level >= D3D_FEATURE_LEVEL_10_0);
        d3d11.caps.features.indexUInt32 = true;
        d3d11.caps.features.fillModeNonSolid = true;
        d3d11.caps.features.samplerAnisotropy = true;
        d3d11.caps.features.textureCompressionETC2 = false;
        d3d11.caps.features.textureCompressionASTC_LDR = false;
        d3d11.caps.features.textureCompressionBC = true;
        d3d11.caps.features.textureCubeArray = (d3d11.feature_level >= D3D_FEATURE_LEVEL_10_1);
        d3d11.caps.features.raytracing = false;

        // Limits
        d3d11.caps.limits.maxVertexAttributes = AGPU_MAX_VERTEX_ATTRIBUTES;
        d3d11.caps.limits.maxVertexBindings = AGPU_MAX_VERTEX_ATTRIBUTES;
        d3d11.caps.limits.maxVertexAttributeOffset = AGPU_MAX_VERTEX_ATTRIBUTE_OFFSET;
        d3d11.caps.limits.maxVertexBindingStride = AGPU_MAX_VERTEX_BUFFER_STRIDE;

        d3d11.caps.limits.maxTextureDimension2D = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
        d3d11.caps.limits.maxTextureDimension3D = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
        d3d11.caps.limits.maxTextureDimensionCube = D3D11_REQ_TEXTURECUBE_DIMENSION;
        d3d11.caps.limits.maxTextureArrayLayers = D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
        d3d11.caps.limits.maxColorAttachments = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
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
    dxgiAdapter->Release();

    // Create swapchain.
    _agpu_d3d11_init_swapchain(&d3d11.swapchain, &config->swapchain_info);

    return true;
}

static void d3d11_shutdown(void)
{
    _agpu_d3d11_shutdown_swapchain(&d3d11.swapchain);

    SAFE_RELEASE(d3d11.annotation);
    SAFE_RELEASE(d3d11.context);

#if !defined(NDEBUG)
    ULONG ref_count = d3d11.device->Release();
    if (ref_count > 0)
    {
        //GPU_THROW("Direct3D11: There are %d unreleased references left on the device", ref_count);

        ID3D11Debug* d3d11_debug = NULL;
        if (SUCCEEDED(d3d11.device->QueryInterface(IID_PPV_ARGS(&d3d11_debug))))
        {
            d3d11_debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
            d3d11_debug->Release();
        }
    }
    else
    {
        //log(LogLevel::Debug, "Direct3D11: No leaks detected");
    }
#else
    d3d11.device->Release();
#endif

    SAFE_RELEASE(d3d11.factory);

#ifdef _DEBUG
    IDXGIDebug1* dxgiDebug1;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    if (d3d11.DXGIGetDebugInterface1 &&
        SUCCEEDED(d3d11.DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug1))))
#else
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug1))))
#endif
    {
        dxgiDebug1->ReportLiveObjects(D3D_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        dxgiDebug1->Release();
    }
#endif
}

static bool d3d11_frame_begin(void)
{
    return !d3d11.is_lost;
}

static void d3d11_frame_finish(void)
{
    HRESULT hr = d3d11.swapchain.handle->Present(d3d11.swapchain.sync_interval, d3d11.swapchain.present_flags);
    if (hr == DXGI_ERROR_DEVICE_REMOVED
        || hr == DXGI_ERROR_DEVICE_HUNG
        || hr == DXGI_ERROR_DEVICE_RESET
        || hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR
        || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
    {
        d3d11.is_lost = true;
        return;
    }

    if (!d3d11.is_lost)
    {
        // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
        if (!d3d11.factory->IsCurrent())
        {
            d3d11_CreateFactory();
        }
    }
}

static void d3d11_query_caps(agpu_caps* caps) {
    *caps = d3d11.caps;
}

/* Swapchain functions */
static void _agpu_d3d11_after_reset_swapchain(d3d11_swapchain* swapchain);

static void _agpu_d3d11_init_swapchain(d3d11_swapchain* swapchain, const agpu_swapchain_info* info)
{
    swapchain->handle = agpu_d3d_create_swapchain(
        d3d11.factory, d3d11.factoryCaps, d3d11.device,
        info->window_handle,
        _agpu_d3d_swapchain_format(info->color_format),
        info->width, info->height,
        AGPU_NUM_INFLIGHT_FRAMES,
        info->is_fullscreen);

    swapchain->color_format = info->color_format;
    swapchain->is_fullscreen = info->is_fullscreen;

    swapchain->colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
    swapchain->sync_interval = 1u;
    swapchain->present_flags = 0;

    if (!info->vsync)
    {
        swapchain->sync_interval = 0;
        if (!swapchain->is_fullscreen && d3d11.isTearingSupported)
        {
            swapchain->present_flags |= DXGI_PRESENT_ALLOW_TEARING;
        }
    }

    _agpu_d3d11_after_reset_swapchain(swapchain);
}

static void _agpu_d3d11_shutdown_swapchain(d3d11_swapchain* swapchain)
{
    //destroyTexture(d3d11.swapchains[handle.id].backbufferTexture);
    //destroyTexture(d3d11.swapchains[handle.id].depthStencilTexture);
    SAFE_RELEASE(swapchain->handle);
}

static void _agpu_d3d11_after_reset_swapchain(d3d11_swapchain* swapchain)
{
    DXGI_SWAP_CHAIN_DESC1 swapchain_desc;
    VHR(swapchain->handle->GetDesc1(&swapchain_desc));

    swapchain->width = swapchain_desc.Width;
    swapchain->height = swapchain_desc.Height;
}

/* Buffer */
static D3D11_USAGE _agpu_d3d11_usage(agpu_buffer_usage usage) {
    switch (usage) {
    case AGPU_BUFFER_USAGE_IMMUTABLE:
        return D3D11_USAGE_IMMUTABLE;
    case AGPU_BUFFER_USAGE_DYNAMIC:
    case AGPU_BUFFER_USAGE_STREAM:
        return D3D11_USAGE_DYNAMIC;
    default:
        AGPU_UNREACHABLE();
        return D3D11_USAGE_DEFAULT;
    }
}

static UINT _agpu_d3d11_cpu_access_flags(agpu_buffer_usage usage) {
    switch (usage) {
    case AGPU_BUFFER_USAGE_IMMUTABLE:
        return 0;
    case AGPU_BUFFER_USAGE_DYNAMIC:
    case AGPU_BUFFER_USAGE_STREAM:
        return D3D11_CPU_ACCESS_WRITE;
    default:
        AGPU_UNREACHABLE();
        return 0;
    }
}

static agpu_buffer d3d11_buffer_create(const agpu_buffer_info* info)
{
    /* Verify resource limits first. */
    static constexpr uint64_t c_maxBytes = D3D11_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM * 1024u * 1024u;
    static_assert(c_maxBytes <= UINT32_MAX, "Exceeded integer limits");

    uint64_t sizeInBytes = info->size;

    if (sizeInBytes > c_maxBytes)
    {
        //log(LogLevel::Error, "Direct3D11: Resource size too large for DirectX 11 (size {})", sizeInBytes);
        return nullptr;
    }

    D3D11_BUFFER_DESC d3d11_desc = {};
    d3d11_desc.ByteWidth = static_cast<UINT>(sizeInBytes);
    d3d11_desc.Usage = _agpu_d3d11_usage(info->usage);
    //bufferDesc.BindFlags = D3D11GetBindFlags(desc.usage);
    d3d11_desc.CPUAccessFlags = _agpu_d3d11_cpu_access_flags(info->usage);
    d3d11_desc.MiscFlags = 0;
    d3d11_desc.StructureByteStride = 0;

    /*if (any(desc.usage & BufferUsage::Dynamic))
    {
        d3dDesc.Usage = D3D11_USAGE_DYNAMIC;
        d3dDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    }
    else if (any(desc.usage & BufferUsage::Staging)) {
        d3dDesc.Usage = D3D11_USAGE_STAGING;
        d3dDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
    }

    if (any(usage & BufferUsage::StorageReadOnly)
        || any(usage & BufferUsage::StorageReadWrite))
    {
        if (desc.stride != 0)
        {
            d3dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        }
        else
        {
            d3dDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
        }
    }

    if (any(desc.usage & BufferUsage::Indirect))
    {
        d3dDesc.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
    }*/

    D3D11_SUBRESOURCE_DATA* initialDataPtr = nullptr;
    D3D11_SUBRESOURCE_DATA initialResourceData = {};
    if (info->data != nullptr)
    {
        initialResourceData.pSysMem = info->data;
        initialDataPtr = &initialResourceData;
    }

    ID3D11Buffer* handle;
    HRESULT hr = d3d11.device->CreateBuffer(&d3d11_desc, initialDataPtr, &handle);
    if (FAILED(hr))
    {
        agpu_log(AGPU_LOG_LEVEL_ERROR, "Direct3D11: Failed to create buffer");
        return nullptr;
    }

    d3d11_buffer* buffer = new d3d11_buffer();
    return (agpu_buffer)buffer;
}

static void d3d11_buffer_destroy(agpu_buffer handle)
{
}

/* Shader */
static agpu_shader d3d11_shader_create(const agpu_shader_info* info)
{
    return nullptr;
}

static void d3d11_shader_destroy(agpu_shader handle)
{

}

/* Texture */
static agpu_texture d3d11_texture_create(const agpu_texture_info* info)
{
    /* 

    D3D11Texture& texture = d3d11.textures[id];
    texture = {};

    if (handle)
    {
        texture.handle = (ID3D11Resource*)handle;
        texture.handle->AddRef();
    }
    else
    {

    }

    HRESULT hr = d3d11.device->CreateRenderTargetView(texture.handle, nullptr, &texture.rtv);
*/
    return nullptr;
}

static void d3d11_texture_destroy(agpu_texture handle)
{
}

/* Pipeline */
static agpu_pipeline d3d11_pipeline_create(const agpu_pipeline_info* info)
{
    return nullptr;
}

static void d3d11_pipeline_destroy(agpu_pipeline handle)
{
}

/* Commands */
static void d3d11_push_debug_group(const char* name)
{
}

static void d3d11_pop_debug_group(void)
{
}

static void d3d11_insert_debug_marker(const char* name)
{
}

static void d3d11_begin_render_pass(const agpu_render_pass_info* info)
{

}

static void d3d11_end_render_pass(void)
{
}

static void d3d11_bind_pipeline(agpu_pipeline handle)
{

}

static void d3d11_draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex)
{
}

#if TODO

    std::lock_guard<std::mutex> lock(d3d11.handle_mutex);

    if (d3d11.textures.isFull())
    {
        log(LogLevel::Error, "D3D11: Not enough free textures slots.");
        return kInvalidTexture;
    }

    const uint16_t id = d3d11.textures.alloc();
    if (id == kInvalidPoolId) {
        return kInvalidTexture;
    }
    return { id };
}

static void d3d11_DestroyTexture(agpu_texture handle)
{
    SAFE_RELEASE(d3d11.textures[handle.id].rtv);
    SAFE_RELEASE(d3d11.textures[handle.id].handle);

    std::lock_guard<std::mutex> lock(d3d11.handle_mutex);
    d3d11.textures.dealloc(handle.id);
}

static void d3d11_PushDebugGroup(const char* name)
{
    wchar_t wName[128];
    if (StringConvert(name, wName) > 0)
    {
        d3d11.annotation->BeginEvent(wName);
    }
}

static void d3d11_PopDebugGroup(void)
{
    d3d11.annotation->EndEvent();
}

static void d3d11_InsertDebugMarker(const char* name)
{
    wchar_t wName[128];
    if (StringConvert(name, wName) > 0)
    {
        d3d11.annotation->SetMarker(wName);
    }
}

static void d3d11_BeginRenderPass(const RenderPassDescription* renderPass)
{
    for (uint32_t i = 0; i < renderPass->colorAttachmentsCount; i++)
    {
        AGPU_ASSERT(renderPass->colorAttachments[i].texture.isValid());

        D3D11Texture& texture = d3d11.textures[renderPass->colorAttachments[i].texture.id];

        switch (renderPass->colorAttachments[i].loadAction)
        {
        case LoadAction::Clear:
            d3d11.context->ClearRenderTargetView(texture.rtv, &renderPass->colorAttachments[i].clearColor.r);
            break;

        default:
            break;
        }

    }

    //ID3D11DeviceContext1_ClearDepthStencilView(d3d11.context, d3d11_current_context->dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    //ID3D11DeviceContext1_OMSetRenderTargets(d3d11.context, 1, &d3d11_current_context->rtv, d3d11_current_context->dsv);

    // Set the viewport.
    //auto viewport = m_deviceResources->GetScreenViewport();
    //context->RSSetViewports(1, &viewport);
}

static void d3d11_EndRenderPass(void)
{
    d3d11.context->OMSetRenderTargets(kMaxColorAttachments, d3d11.zero_rtvs, nullptr);
}
#endif // TODO


/* Driver */
static bool d3d11_is_supported(void)
{
    if (d3d11.available_initialized) {
        return d3d11.available;
    }

    d3d11.available_initialized = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
    d3d11.dxgiDLL = LoadLibraryA("dxgi.dll");
    if (!d3d11.dxgiDLL) {
        return false;
    }

    d3d11.CreateDXGIFactory1 = (PFN_CREATE_DXGI_FACTORY1)GetProcAddress(d3d11.dxgiDLL, "CreateDXGIFactory1");
    if (!d3d11.CreateDXGIFactory1)
    {
        return false;
    }

    d3d11.CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(d3d11.dxgiDLL, "CreateDXGIFactory2");
    d3d11.DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(d3d11.dxgiDLL, "DXGIGetDebugInterface1");

    d3d11.d3d11DLL = LoadLibraryA("d3d11.dll");
    if (!d3d11.d3d11DLL) {
        return false;
    }

    d3d11.D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11.d3d11DLL, "D3D11CreateDevice");
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

    HRESULT hr = agpuD3D11CreateDevice(
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

agpu_driver D3D11_Driver = {
    AGPU_BACKEND_TYPE_D3D11,
    d3d11_is_supported,
    d3d11_create_renderer
};

#undef GPU_THROW
#undef GPU_CHECK


#endif /* defined(AGPU_DRIVER_D3D11)  */
