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

#if defined(VGPU_DRIVER_D3D11) 
#define CINTERFACE
#define COBJMACROS
#define D3D11_NO_HELPERS
#include <d3d11_1.h>
#include "vgpu_d3d_common.h"
#include <stdio.h>

/* D3D11 guids */
static const IID D3D11_IID_ID3D11DeviceContext1 = { 0xbb2c6faa, 0xb5fb, 0x4082, {0x8e, 0x6b, 0x38, 0x8b, 0x8c, 0xfa, 0x90, 0xe1} };
static const IID D3D11_IID_ID3D11Device1 = { 0xa04bfb29, 0x08ef, 0x43d6, {0xa4, 0x9c, 0xa9, 0xbd, 0xbd, 0xcb, 0xe6, 0x86} };
static const IID D3D11_IID_ID3DUserDefinedAnnotation = { 0xb2daad8b, 0x03d4, 0x4dbf, {0x95, 0xeb, 0x32, 0xab, 0x4b, 0x63, 0xd0, 0xab} };
static const IID D3D11_IID_ID3D11Texture2D = { 0x6f15aaf2,0xd208,0x4e89, {0x9a,0xb4,0x48,0x95,0x35,0xd3,0x4f,0x9c } };

#ifdef _DEBUG
static const IID D3D11_WKPDID_D3DDebugObjectName = { 0x429b8c22, 0x9188, 0x4b0c, { 0x87,0x42,0xac,0xb0,0xbf,0x85,0xc2,0x00 } };
static const IID D3D11_IID_ID3D11Debug = { 0x79cf2233, 0x7536, 0x4948, {0x9d, 0x36, 0x1e, 0x46, 0x92, 0xdc, 0x57, 0x60} };
static const IID D3D11_IID_ID3D11InfoQueue = { 0x6543dbb6, 0x1b48, 0x42f5, {0xab,0x82,0xe9,0x7e,0xc7,0x43,0x26,0xf6} };
#endif

typedef struct d3d11_texture {
    ID3D11Resource* handle;
    DXGI_FORMAT dxgi_format;

    uint32_t width;
    uint32_t height;
    uint32_t depth_or_layers;
    uint32_t mipLevels;
    vgpu_texture_type type;
    uint32_t sample_count;
} d3d11_texture;

typedef struct d3d11_framebuffer {
    uint32_t num_rtvs;
    ID3D11RenderTargetView* rtvs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
    ID3D11DepthStencilView* dsv;

    uint32_t width;
    uint32_t height;
    uint32_t layers;
} d3d11_framebuffer;

typedef struct d3d11_swapchain {
    uintptr_t window_handle;
    vgpu_texture_format color_format;
    vgpu_texture_format depth_stencil_format;
    IDXGISwapChain1* handle;
    uint32_t width;
    uint32_t height;
    vgpu_texture backbuffer;
    vgpu_texture depth_stencil_texture;
    vgpu_framebuffer framebuffer;
} d3d11_swapchain;

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

    vgpu_caps caps;

    bool validation;
    IDXGIFactory2* factory;
    uint32_t factory_caps;

    ID3D11Device1* device;
    ID3D11DeviceContext1* context;
    ID3DUserDefinedAnnotation* d3d_annotation;
    D3D_FEATURE_LEVEL feature_level;

    uint8_t  num_backbuffers;
    uint8_t  max_frame_latency;
    uint32_t sync_interval;
    uint32_t present_flags;

    vgpu_swapchain main_swapchain;
    bool is_lost;
} d3d11;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#   define _vgpu_CreateDXGIFactory1 d3d11.dxgi.CreateDXGIFactory1
#   define _vgpu_CreateDXGIFactory2 d3d11.dxgi.CreateDXGIFactory2
#   define _vgpu_DXGIGetDebugInterface1 d3d11.dxgi.DXGIGetDebugInterface1
#   define _vgpu_D3D11CreateDevice d3d11.D3D11CreateDevice
#else
#   define _vgpu_CreateDXGIFactory1 CreateDXGIFactory1
#   define _vgpu_CreateDXGIFactory2 CreateDXGIFactory2
#   define _vgpu_DXGIGetDebugInterface1 DXGIGetDebugInterface1
#   define _vgpu_D3D11CreateDevice D3D11CreateDevice
#endif

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


static bool d3d11_create_factory()
{
    if (d3d11.factory) {
        IDXGIFactory2_Release(d3d11.factory);
        d3d11.factory = NULL;
    }

    HRESULT hr = S_OK;

#if defined(_DEBUG)
    bool debugDXGI = false;

    if (d3d11.validation)
    {
        IDXGIInfoQueue* dxgi_info_queue;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (d3d11.dxgi.DXGIGetDebugInterface1 &&
            SUCCEEDED(_vgpu_DXGIGetDebugInterface1(0, &D3D_IID_IDXGIInfoQueue, (void**)&dxgi_info_queue)))
#else
        if (SUCCEEDED(_vgpu_DXGIGetDebugInterface1(0, &D3D_IID_IDXGIInfoQueue, (void**)&dxgi_info_queue)))
#endif
        {
            debugDXGI = true;
            hr = _vgpu_CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, &D3D_IID_IDXGIFactory2, (void**)&d3d11.factory);
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
        hr = _vgpu_CreateDXGIFactory1(&D3D_IID_IDXGIFactory2, (void**)&d3d11.factory);
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

static IDXGIAdapter1* d3d11_get_adapter(vgpu_device_preference device_preference)
{
    /* Detect adapter now. */
    IDXGIAdapter1* adapter = NULL;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
    if (device_preference != VGPU_DEVICE_PREFERENCE_DONT_CARE) {
        IDXGIFactory6* factory6;
        HRESULT hr = IDXGIFactory2_QueryInterface(d3d11.factory, &D3D_IID_IDXGIFactory6, (void**)&factory6);
        if (SUCCEEDED(hr))
        {
            // By default prefer high performance
            DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
            if (device_preference == VGPU_DEVICE_PREFERENCE_LOW_POWER) {
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

static bool d3d11_init(const vgpu_config* config) {
    d3d11.validation = config->debug || config->profile;

    if (!d3d11_create_factory()) {
        return false;
    }

    IDXGIAdapter1* dxgi_adapter = d3d11_get_adapter(config->device_preference);

    /* Setup present flags. */
    d3d11.num_backbuffers = 2;
    d3d11.max_frame_latency = 3;
    d3d11.sync_interval = _vgpu_d3d_sync_interval(config->present_mode);
    d3d11.present_flags = 0;
    if (config->present_mode == VGPU_PRESENT_MODE_IMMEDIATE
        && d3d11.factory_caps & DXGIFACTORY_CAPS_TEARING) {
        d3d11.present_flags |= DXGI_PRESENT_ALLOW_TEARING;
    }

    /* Create d3d11 device */
    {
        UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

        if (d3d11.validation && _vgpu_d3d11_sdk_layers_available())
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
            //vgpu_log(AGPULogLevel_Error, "No Direct3D hardware device found");
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
            //gpu_device_destroy(device);
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
        {
            IDXGIDevice1* dxgiDevice1;
            if (SUCCEEDED(ID3D11Device_QueryInterface(temp_d3d_device, &D3D_IID_IDXGIDevice1, (void**)&dxgiDevice1)))
            {
                /* Default to 3. */
                VHR(IDXGIDevice1_SetMaximumFrameLatency(dxgiDevice1, d3d11.max_frame_latency));
                IDXGIDevice1_Release(dxgiDevice1);
            }
        }
#endif

        VHR(ID3D11Device_QueryInterface(temp_d3d_device, &D3D11_IID_ID3D11Device1, (void**)&d3d11.device));
        VHR(ID3D11DeviceContext_QueryInterface(temp_d3d_context, &D3D11_IID_ID3D11DeviceContext1, (void**)&d3d11.context));
        VHR(ID3D11DeviceContext_QueryInterface(temp_d3d_context, &D3D11_IID_ID3DUserDefinedAnnotation, (void**)&d3d11.d3d_annotation));
        ID3D11DeviceContext_Release(temp_d3d_context);
        ID3D11Device_Release(temp_d3d_device);
    }

    if (config->window_handle) {
        d3d11.main_swapchain = vgpu_swapchain_create(config->window_handle, config->color_format, config->depth_stencil_format);
    }

    /* TODO: Init caps first. */
    IDXGIAdapter1_Release(dxgi_adapter);

    return true;
}

static void d3d11_shutdown(void) {
    if (d3d11.main_swapchain) {
        vgpu_swapchain_destroy(d3d11.main_swapchain);
    }

    ID3D11DeviceContext1_Release(d3d11.context);
    ID3DUserDefinedAnnotation_Release(d3d11.d3d_annotation);

    ULONG ref_count = ID3D11Device1_Release(d3d11.device);
#if !defined(NDEBUG)
    if (ref_count > 0)
    {
        //gpuLog(GPULogLevel_Error, "Direct3D11: There are %d unreleased references left on the device", refCount);

        ID3D11Debug* d3d11_debug = NULL;
        if (SUCCEEDED(ID3D11Device1_QueryInterface(d3d11.device, &D3D11_IID_ID3D11Debug, (void**)&d3d11_debug)))
        {
            ID3D11Debug_ReportLiveDeviceObjects(d3d11_debug, D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
            ID3D11Debug_Release(d3d11_debug);
        }
    }
#else
    (void)ref_count; // avoid warning
#endif

    IDXGIFactory2_Release(d3d11.factory);

#ifdef _DEBUG
    IDXGIDebug1* dxgi_debug1;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    if (d3d11.dxgi.DXGIGetDebugInterface1 &&
        SUCCEEDED(_vgpu_DXGIGetDebugInterface1(0, &D3D_IID_IDXGIDebug1, (void**)&dxgi_debug1)))
#else
    if (SUCCEEDED(_vgpu_DXGIGetDebugInterface1(0, &D3D_IID_IDXGIDebug1, (void**)&dxgi_debug1)))
#endif
    {
        IDXGIDebug1_ReportLiveObjects(dxgi_debug1, D3D_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL);
        IDXGIDebug1_Release(dxgi_debug1);
    }
#endif

    memset(&d3d11, 0, sizeof(d3d11));
}

static void d3d11_get_caps(vgpu_caps* caps) {
    *caps = d3d11.caps;
}

static bool d3d11_frame_begin(void) {
    return true;
}

static void d3d11_frame_end(void) {

    // Present sequentally all swap chains.
    HRESULT hr = S_OK;
    if (d3d11.main_swapchain) {
        vgpu_swapchain_present(d3d11.main_swapchain);
    }

    if (d3d11.is_lost)
    {
#ifdef _DEBUG
        //char buff[64] = {};
        //sprintf_s(buff, "Device Lost on ResizeBuffers: Reason code 0x%08X\n",
        //    static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? d3d11.d3dDevice->GetDeviceRemovedReason() : hr));
        //OutputDebugStringA(buff);
#endif
    }

    VHR(hr);
}

static void d3d11_insertDebugMarker(const char* name)
{
    wchar_t wideName[128];
    int num = MultiByteToWideChar(CP_UTF8, 0, name, -1, NULL, 0);
    if (num > 0)
    {
        MultiByteToWideChar(CP_UTF8, 0, name, -1, &wideName[0], num);
    }

    if (num > 0) {
        ID3DUserDefinedAnnotation_SetMarker(d3d11.d3d_annotation, wideName);
    }
}

static void d3d11_pushDebugGroup(const char* name)
{
    wchar_t wideName[128];
    int num = MultiByteToWideChar(CP_UTF8, 0, name, -1, NULL, 0);
    if (num > 0)
    {
        MultiByteToWideChar(CP_UTF8, 0, name, -1, &wideName[0], num);
    }

    if (num > 0) {
        ID3DUserDefinedAnnotation_BeginEvent(d3d11.d3d_annotation, wideName);
    }
}

static void d3d11_popDebugGroup(void)
{
    ID3DUserDefinedAnnotation_EndEvent(d3d11.d3d_annotation);
}

static void d3d11_render_begin(vgpu_framebuffer framebuffer) {
#if TODO
    D3D11Framebuffer& framebuffer = d3d11.framebuffers[beginDesc->framebuffer.id];

    for (uint32_t i = 0; i < framebuffer.colorAttachmentCount; i++)
    {
        if (beginDesc->colorAttachments[i].loadAction == VGPULoadAction_Clear)
        {
            d3d11.commandBuffers[commandBuffer].context->ClearRenderTargetView(
                framebuffer.colorAttachments[i],
                &beginDesc->colorAttachments[i].clear_color.r
            );
        }
    }

    if (framebuffer.depthStencilAttachment != nullptr) {
        UINT clearFlags = 0;
        if (beginDesc->depthStencilAttachment.depthLoadAction == VGPULoadAction_Clear)
        {
            clearFlags |= D3D11_CLEAR_DEPTH;
        }

        if (beginDesc->depthStencilAttachment.stencilLoadAction == VGPULoadAction_Clear)
        {
            clearFlags |= D3D11_CLEAR_STENCIL;
        }

        d3d11.commandBuffers[commandBuffer].context->ClearDepthStencilView(
            framebuffer.depthStencilAttachment,
            clearFlags,
            beginDesc->depthStencilAttachment.clearDepth, beginDesc->depthStencilAttachment.clearStencil
        );
    }

    d3d11.commandBuffers[commandBuffer].context->OMSetRenderTargets(
        framebuffer.colorAttachmentCount,
        framebuffer.colorAttachments,
        framebuffer.depthStencilAttachment
    );

    /* set viewport and scissor rect to cover render target size */
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = (FLOAT)framebuffer.width;
    viewport.Height = (FLOAT)framebuffer.height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    D3D11_RECT scissorRect;
    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = (LONG)framebuffer.width;
    scissorRect.bottom = (LONG)framebuffer.height;

    d3d11.commandBuffers[commandBuffer].context->RSSetViewports(1, &viewport);
    d3d11.commandBuffers[commandBuffer].context->RSSetScissorRects(1, &scissorRect);
#endif // TODO
}

static void d3d11_render_finish(void) {

}

/* Texture */
static vgpu_texture d3d11_texture_create(const vgpu_texture_info* info) {
    d3d11_texture* texture = VGPU_ALLOC_HANDLE(d3d11_texture);
    texture->width = info->width;
    texture->height = info->height;
    texture->depth_or_layers = info->depth;
    texture->mipLevels = info->mip_levels;
    texture->type = info->type;
    texture->sample_count = info->sample_count;

    texture->dxgi_format = _vgpu_d3d_format_with_usage(info->format, info->usage);

    HRESULT hr = S_OK;
    if (info->external_handle) {
        texture->handle = (ID3D11Resource*)info->external_handle;
    }
    else {
        D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
        UINT bindFlags = 0;
        UINT CPUAccessFlags = 0;
        UINT miscFlags = 0;
        UINT arraySizeMultiplier = 1;
        if (info->type == VGPU_TEXTURE_TYPE_CUBE) {
            arraySizeMultiplier = 6;
            miscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
        }

        if (info->usage & VGPU_TEXTURE_USAGE_SAMPLED) {
            bindFlags |= D3D11_BIND_SHADER_RESOURCE;
        }
        if (info->usage & VGPU_TEXTURE_USAGE_STORAGE) {
            bindFlags |= D3D11_BIND_UNORDERED_ACCESS;
        }

        if (info->usage & VGPU_TEXTURE_USAGE_RENDER_TARGET) {
            if (vgpu_is_depth_stencil_format(info->format)) {
                bindFlags |= D3D11_BIND_DEPTH_STENCIL;
            }
            else {
                bindFlags |= D3D11_BIND_RENDER_TARGET;
            }
        }

        const bool generateMipmaps = false;
        if (generateMipmaps)
        {
            bindFlags |= D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            miscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
        }

        if (info->type == VGPU_TEXTURE_TYPE_2D || info->type == VGPU_TEXTURE_TYPE_CUBE) {
            const D3D11_TEXTURE2D_DESC d3d11_desc = {
                .Width = info->width,
                .Height = info->height,
                .MipLevels = info->mip_levels,
                .ArraySize = info->array_layers * arraySizeMultiplier,
                .Format = texture->dxgi_format,
                .SampleDesc = {
                    .Count = 1,
                    .Quality = 0
                },
                .Usage = usage,
                .BindFlags = bindFlags,
                .CPUAccessFlags = CPUAccessFlags,
                .MiscFlags = miscFlags
            };

            hr = ID3D11Device_CreateTexture2D(
                d3d11.device,
                &d3d11_desc,
                NULL,
                (ID3D11Texture2D**)&texture->handle
            );
        }
        else if (info->type == VGPU_TEXTURE_TYPE_3D) {
            const D3D11_TEXTURE3D_DESC d3d11_desc = {
                .Width = info->width,
                .Height = info->height,
                .Depth = info->depth,
                .MipLevels = info->mip_levels,
                .Format = texture->dxgi_format,
                .Usage = usage,
                .BindFlags = bindFlags,
                .CPUAccessFlags = CPUAccessFlags,
                .MiscFlags = miscFlags
            };

            hr = ID3D11Device_CreateTexture3D(
                d3d11.device,
                &d3d11_desc,
                NULL,
                (ID3D11Texture3D**)&texture->handle
            );
        }
    }

    return (vgpu_texture)texture;
}

static void d3d11_texture_destroy(vgpu_texture handle) {
    d3d11_texture* texture = (d3d11_texture*)handle;
    ID3D11Resource_Release(texture->handle);
    VGPU_FREE(texture);
}

static uint32_t d3d11_texture_get_width(vgpu_texture handle, uint32_t mipLevel) {
    d3d11_texture* texture = (d3d11_texture*)handle;
    return _vgpu_max(1, texture->width >> mipLevel);
}

static uint32_t d3d11_texture_get_height(vgpu_texture handle, uint32_t mipLevel) {
    d3d11_texture* texture = (d3d11_texture*)handle;
    return _vgpu_max(1, texture->height >> mipLevel);
}

/* Framebuffer */
static vgpu_framebuffer d3d11_framebuffer_create(const vgpu_framebuffer_info* info) {
    d3d11_framebuffer* framebuffer = VGPU_ALLOC_HANDLE(d3d11_framebuffer);

    for (uint32_t i = 0; i < _vgpu_count_of(info->color_attachments) && info->color_attachments[i].texture; i++, framebuffer->num_rtvs++) {
        d3d11_texture* texture = (d3d11_texture*)info->color_attachments[i].texture;
        uint32_t mip_level = info->color_attachments[i].level;
        uint32_t slice = info->color_attachments[i].slice;

        /* TODO: Understand SwapChain RTV format when using RGBA8UNormSrgb */
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
        memset(&rtvDesc, 0, sizeof(rtvDesc));
        rtvDesc.Format = texture->dxgi_format;
        switch (texture->type)
        {
        case VGPU_TEXTURE_TYPE_2D:

            if (texture->sample_count <= 1)
            {
                if (texture->depth_or_layers > 1)
                {
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    rtvDesc.Texture2DArray.MipSlice = mip_level;
                    rtvDesc.Texture2DArray.FirstArraySlice = slice;
                    rtvDesc.Texture2DArray.ArraySize = texture->depth_or_layers;
                }
                else
                {
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                    rtvDesc.Texture2D.MipSlice = mip_level;
                }
            }
            else
            {
                if (texture->depth_or_layers > 1)
                {
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                    rtvDesc.Texture2DMSArray.FirstArraySlice = slice;
                    rtvDesc.Texture2DMSArray.ArraySize = texture->depth_or_layers;
                }
                else
                {
                    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
                }
            }

            break;
        case VGPU_TEXTURE_TYPE_3D:
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
            rtvDesc.Texture3D.MipSlice = mip_level;
            rtvDesc.Texture3D.FirstWSlice = slice;
            rtvDesc.Texture3D.WSize = (UINT)-1;
            break;
        case VGPU_TEXTURE_TYPE_CUBE:
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.MipSlice = mip_level;
            rtvDesc.Texture2DArray.FirstArraySlice = slice;
            rtvDesc.Texture2DArray.ArraySize = texture->depth_or_layers;
            break;

        default:
            break;
        }

        VHR(ID3D11Device1_CreateRenderTargetView(
            d3d11.device,
            texture->handle,
            &rtvDesc,
            &framebuffer->rtvs[framebuffer->num_rtvs])
        );
    }

    if (info->depth_stencil_attachment.texture)
    {
        d3d11_texture* texture = (d3d11_texture*)info->depth_stencil_attachment.texture;
        uint32_t mip_level = info->depth_stencil_attachment.level;
        uint32_t slice = info->depth_stencil_attachment.slice;

        D3D11_DEPTH_STENCIL_VIEW_DESC d3d11_dsv_desc;
        memset(&d3d11_dsv_desc, 0, sizeof(d3d11_dsv_desc));
        d3d11_dsv_desc.Format = texture->dxgi_format;

        switch (texture->type)
        {
        case VGPU_TEXTURE_TYPE_2D:

            if (texture->sample_count <= 1)
            {
                if (texture->depth_or_layers > 1)
                {
                    d3d11_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                    d3d11_dsv_desc.Texture2DArray.MipSlice = mip_level;
                    d3d11_dsv_desc.Texture2DArray.FirstArraySlice = slice;
                    d3d11_dsv_desc.Texture2DArray.ArraySize = texture->depth_or_layers;
                }
                else
                {
                    d3d11_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                    d3d11_dsv_desc.Texture2D.MipSlice = mip_level;
                }
            }
            else
            {
                if (texture->depth_or_layers > 1)
                {
                    d3d11_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
                    d3d11_dsv_desc.Texture2DMSArray.FirstArraySlice = slice;
                    d3d11_dsv_desc.Texture2DMSArray.ArraySize = texture->depth_or_layers;
                }
                else
                {
                    d3d11_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
                }
            }

            break;
        case VGPU_TEXTURE_TYPE_CUBE:
            d3d11_dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
            d3d11_dsv_desc.Texture2DArray.MipSlice = mip_level;
            d3d11_dsv_desc.Texture2DArray.FirstArraySlice = slice;
            d3d11_dsv_desc.Texture2DArray.ArraySize = texture->depth_or_layers;
            break;

        default:
            break;
        }

        VHR(ID3D11Device1_CreateDepthStencilView(
            d3d11.device,
            texture->handle,
            &d3d11_dsv_desc,
            &framebuffer->dsv)
        );
    }

    return (vgpu_framebuffer)framebuffer;
}

static void d3d11_framebuffer_destroy(vgpu_framebuffer handle) {
    d3d11_framebuffer* framebuffer = (d3d11_framebuffer*)handle;
    for (uint32_t i = 0; i < framebuffer->num_rtvs; i++) {
        ID3D11RenderTargetView_Release(framebuffer->rtvs[i]);
    }

    if (framebuffer->dsv) {
        ID3D11DepthStencilView_Release(framebuffer->dsv);
    }

    VGPU_FREE(framebuffer);
}

/* Swapchain */
static vgpu_swapchain d3d11_swapchain_create(uintptr_t window_handle, vgpu_texture_format color_format, vgpu_texture_format depth_stencil_format) {
    d3d11_swapchain* swapchain = VGPU_ALLOC_HANDLE(d3d11_swapchain);

    swapchain->window_handle = window_handle;
    swapchain->color_format = _vgpu_def(color_format, VGPU_TEXTURE_FORMAT_BGRA8);
    swapchain->depth_stencil_format = _vgpu_def(depth_stencil_format, VGPU_TEXTURE_FORMAT_UNDEFINED);

    vgpu_swapchain_resize((vgpu_swapchain)swapchain, 0, 0);
    return (vgpu_swapchain)swapchain;
}

static void d3d11_swapchain_destroy(vgpu_swapchain handle) {
    d3d11_swapchain* swapchain = (d3d11_swapchain*)handle;
    vgpu_framebuffer_destroy(swapchain->framebuffer);
    vgpu_texture_destroy(swapchain->backbuffer);
    if (swapchain->depth_stencil_texture) {
        vgpu_texture_destroy(swapchain->depth_stencil_texture);
    }
    IDXGISwapChain1_Release(swapchain->handle);
    VGPU_FREE(swapchain);
}

static void d3d11_swapchain_resize(vgpu_swapchain handle, uint32_t width, uint32_t height) {
    d3d11_swapchain* swapchain = (d3d11_swapchain*)handle;

    if (swapchain->handle) {
        UINT flags = 0;
        if (d3d11.factory_caps & DXGIFACTORY_CAPS_TEARING)
        {
            flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }

        HRESULT hr = IDXGISwapChain1_ResizeBuffers(
            swapchain->handle,
            d3d11.num_backbuffers,
            width,
            height,
            DXGI_FORMAT_UNKNOWN,
            flags
        );

        if (hr == DXGI_ERROR_DEVICE_REMOVED
            || hr == DXGI_ERROR_DEVICE_RESET)
        {
            d3d11.is_lost = true;
            return;
        }
    }
    else {
        swapchain->handle = vgpu_d3d_create_swapchain(
            d3d11.factory,
            d3d11.factory_caps,
            (IUnknown*)d3d11.device,
            swapchain->window_handle,
            0, 0,
            swapchain->color_format,
            d3d11.num_backbuffers,  /* 3 for triple buffering */
            true
        );
    }

    DXGI_SWAP_CHAIN_DESC1 swapchain_desc;
    VHR(IDXGISwapChain1_GetDesc1(swapchain->handle, &swapchain_desc));
    swapchain->width = swapchain_desc.Width;
    swapchain->height = swapchain_desc.Height;

    ID3D11Texture2D* backbuffer;
    VHR(IDXGISwapChain1_GetBuffer(swapchain->handle, 0, &D3D11_IID_ID3D11Texture2D, (void**)&backbuffer));

    const vgpu_texture_info texture_info = {
        .width = swapchain->width,
        .height = swapchain->height,
        .array_layers = 1u,
        .mip_levels = 1,
        .format = swapchain->color_format,
        .type = VGPU_TEXTURE_TYPE_2D,
        .usage = VGPU_TEXTURE_USAGE_RENDER_TARGET,
        .sample_count = 1u,
        .external_handle = backbuffer
    };

    swapchain->backbuffer = vgpu_texture_create(&texture_info);

    if (swapchain->depth_stencil_format != VGPU_TEXTURE_FORMAT_UNDEFINED) {
        const vgpu_texture_info depth_texture_info = {
            .width = swapchain->width,
            .height = swapchain->height,
            .array_layers = 1u,
            .mip_levels = 1,
            .format = swapchain->depth_stencil_format,
            .type = VGPU_TEXTURE_TYPE_2D,
            .usage = VGPU_TEXTURE_USAGE_RENDER_TARGET,
            .sample_count = 1u,
        };

        swapchain->depth_stencil_texture = vgpu_texture_create(&depth_texture_info);
    }

    /* Create framebuffer */
    vgpu_framebuffer_info fbo_info;
    memset(&fbo_info, 0, sizeof(fbo_info));
    fbo_info.color_attachments[0].texture = swapchain->backbuffer;

    if (swapchain->depth_stencil_format != VGPU_TEXTURE_FORMAT_UNDEFINED) {
        fbo_info.depth_stencil_attachment.texture = swapchain->depth_stencil_texture;
    }

    swapchain->framebuffer = vgpu_framebuffer_create(&fbo_info);
}

static void d3d11_swapchain_present(vgpu_swapchain handle) {
    d3d11_swapchain* swapchain = (d3d11_swapchain*)handle;
    HRESULT hr = IDXGISwapChain1_Present(swapchain->handle, d3d11.sync_interval, d3d11.present_flags);
    if (hr == DXGI_ERROR_DEVICE_REMOVED
        || hr == DXGI_ERROR_DEVICE_HUNG
        || hr == DXGI_ERROR_DEVICE_RESET
        || hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR
        || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
    {
        d3d11.is_lost = true;
    }
}

/* Driver functions */
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

    HRESULT hr = _vgpu_D3D11CreateDevice(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        s_featureLevels,
        _vgpu_count_of(s_featureLevels),
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

static vgpu_context* d3d11_create_context(void) {
    static vgpu_context context = { 0 };
    ASSIGN_DRIVER(d3d11);
    return &context;
}

vgpu_driver d3d11_driver = {
    VGPU_BACKEND_TYPE_D3D11,
    d3d11_is_supported,
    d3d11_create_context
};

#endif /* defined(VGPU_DRIVER_VULKAN) */
