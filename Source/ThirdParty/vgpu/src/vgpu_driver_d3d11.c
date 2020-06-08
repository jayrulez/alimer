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
#define D3D11_NO_HELPERS
#define CINTERFACE
#define COBJMACROS
#include <d3d11_1.h>
#include "vgpu_d3d_common.h"

/* D3D11 guids */
static const IID D3D11_IID_ID3D11DeviceContext1 = { 0xbb2c6faa, 0xb5fb, 0x4082, {0x8e, 0x6b, 0x38, 0x8b, 0x8c, 0xfa, 0x90, 0xe1} };
static const IID D3D11_IID_ID3D11Device1 = { 0xa04bfb29, 0x08ef, 0x43d6, {0xa4, 0x9c, 0xa9, 0xbd, 0xbd, 0xcb, 0xe6, 0x86} };
static const IID D3D11_IID_ID3DUserDefinedAnnotation = { 0xb2daad8b, 0x03d4, 0x4dbf, {0x95, 0xeb, 0x32, 0xab, 0x4b, 0x63, 0xd0, 0xab} };
static const IID D3D11_IID_ID3D11Texture2D = { 0x6f15aaf2,0xd208,0x4e89, {0x9a,0xb4,0x48,0x95,0x35,0xd3,0x4f,0x9c } };

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

} d3d11;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#   define vgpuCreateDXGIFactory1 d3d11.dxgi.CreateDXGIFactory1
#   define vgpuCreateDXGIFactory2 d3d11.dxgi.CreateDXGIFactory2
#   define vgpuDXGIGetDebugInterface1 d3d11.dxgi.DXGIGetDebugInterface1
#   define vgpuD3D11CreateDevice d3d11.D3D11CreateDevice
#else
#   define vgpuCreateDXGIFactory1 CreateDXGIFactory1
#   define vgpuCreateDXGIFactory2 CreateDXGIFactory2
#   define vgpuDXGIGetDebugInterface1 DXGIGetDebugInterface1
#   define vgpuD3D11CreateDevice D3D11CreateDevice
#endif

#ifdef _DEBUG
static const IID D3D11_WKPDID_D3DDebugObjectName = { 0x429b8c22, 0x9188, 0x4b0c, { 0x87,0x42,0xac,0xb0,0xbf,0x85,0xc2,0x00 } };
static const IID D3D11_IID_ID3D11Debug = { 0x79cf2233, 0x7536, 0x4948, {0x9d, 0x36, 0x1e, 0x46, 0x92, 0xdc, 0x57, 0x60} };
static const IID D3D11_IID_ID3D11InfoQueue = { 0x6543dbb6, 0x1b48, 0x42f5, {0xab,0x82,0xe9,0x7e,0xc7,0x43,0x26,0xf6} };
#endif

static bool d3d11_sdk_layers_available() {
    HRESULT hr = vgpuD3D11CreateDevice(
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

typedef struct d3d11_renderer {
    IDXGIFactory2* factory;
    uint32_t factory_caps;

    uint32_t backbuffer_count;

    ID3D11Device1* device;
    ID3D11DeviceContext1* context;
    ID3DUserDefinedAnnotation* d3d_annotation;
    D3D_FEATURE_LEVEL feature_level;
} d3d11_renderer;

typedef struct D3D11BackendContext {
    UINT syncInterval;
    UINT presentFlags;
    IDXGISwapChain1* swapChain;
    VGPUTexture backbuffer;
} D3D11BackendContext;

typedef struct D3D11Texture {
    DXGI_FORMAT format;
    ID3D11Resource* handle;
} D3D11Texture;

/* Device functions */
static bool d3d11_create_factory(d3d11_renderer* renderer, bool validation)
{
    if (renderer->factory) {
        IDXGIFactory2_Release(renderer->factory);
        renderer->factory = NULL;
    }

    HRESULT hr = S_OK;

#if defined(_DEBUG)
    bool debugDXGI = false;

    if (validation)
    {
        IDXGIInfoQueue* dxgiInfoQueue;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (d3d11.dxgi.DXGIGetDebugInterface1 &&
            SUCCEEDED(vgpuDXGIGetDebugInterface1(0, &D3D_IID_IDXGIInfoQueue, (void**)&dxgiInfoQueue)))
#else
        if (SUCCEEDED(vgpuDXGIGetDebugInterface1(0, &D3D_IID_IDXGIInfoQueue, (void**)&dxgiInfoQueue)))
#endif
        {
            debugDXGI = true;
            hr = vgpuCreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, &D3D_IID_IDXGIFactory2, (void**)&renderer->factory);
            if (FAILED(hr)) {
                return false;
            }

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

    if (!debugDXGI)
#endif
    {
        hr = vgpuCreateDXGIFactory1(&D3D_IID_IDXGIFactory1, (void**)&renderer->factory);
        if (FAILED(hr)) {
            return false;
        }
    }

    renderer->factory_caps = 0;

    // Disable FLIP if not on a supporting OS
    renderer->factory_caps |= DXGIFACTORY_CAPS_FLIP_PRESENT;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    {
        IDXGIFactory4* factory4;
        HRESULT hr = IDXGIFactory2_QueryInterface(renderer->factory, &D3D_IID_IDXGIFactory4, (void**)&factory4);
        if (FAILED(hr))
        {
            renderer->factory_caps &= DXGIFACTORY_CAPS_FLIP_PRESENT;
        }
        IDXGIFactory4_Release(factory4);
    }
#endif

    // Check tearing support.
    {
        BOOL allowTearing = FALSE;
        IDXGIFactory5* factory5;
        HRESULT hr = IDXGIFactory2_QueryInterface(renderer->factory, &D3D_IID_IDXGIFactory5, (void**)&factory5);
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

static void d3d11_destroy(vgpu_device device)
{
    d3d11_renderer* renderer = (d3d11_renderer*)device->renderer;

    ID3D11DeviceContext1_Release(renderer->context);
    ID3DUserDefinedAnnotation_Release(renderer->d3d_annotation);

    ULONG refCount = ID3D11Device1_Release(renderer->device);
#if !defined(NDEBUG)
    if (refCount > 0)
    {
        //gpuLog(GPULogLevel_Error, "Direct3D11: There are %d unreleased references left on the device", refCount);

        ID3D11Debug* d3dDebug = NULL;
        if (SUCCEEDED(ID3D11Device1_QueryInterface(renderer->device, &D3D11_IID_ID3D11Debug, (void**)&d3dDebug)))
        {
            ID3D11Debug_ReportLiveDeviceObjects(d3dDebug, D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
            ID3D11Debug_Release(d3dDebug);
        }
    }
#else
    (void)refCount; // avoid warning
#endif

    IDXGIFactory2_Release(renderer->factory);

    VGPU_FREE(renderer);
    VGPU_FREE(device);
}

/* Texture */
static VGPUTexture d3d11_create_texture(vgpu_renderer driver_data, const VGPUTextureDescription* desc) {
    d3d11_renderer* renderer = (d3d11_renderer*)driver_data;
    D3D11Texture* result = VGPU_ALLOC_HANDLE(D3D11Texture);

    // If depth and either ua or sr, set to typeless
    if (vgpuIsDepthOrStencilFormat(desc->format)
        && ((desc->usage & (VGPUTextureUsage_Sampled | VGPUTextureUsage_Storage)) != 0))
    {
        result->format = _vgpuGetTypelessFormatFromDepthFormat(desc->format);
    }
    else {
        result->format = _vgpuGetDXGIFormat(desc->format);
    }

    HRESULT hr = S_OK;
    if (desc->externalHandle) {
        result->handle = (ID3D11Resource*)desc->externalHandle;
    }
    else {
        D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
        UINT bindFlags = 0;
        UINT CPUAccessFlags = 0;
        UINT miscFlags = 0;
        UINT arraySizeMultiplier = 1;
        if (desc->type == VGPUTextureType_Cube) {
            arraySizeMultiplier = 6;
            miscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
        }

        if (desc->usage & VGPUTextureUsage_Sampled) {
            bindFlags |= D3D11_BIND_SHADER_RESOURCE;
        }
        if (desc->usage & VGPUTextureUsage_Storage) {
            bindFlags |= D3D11_BIND_UNORDERED_ACCESS;
        }

        if (desc->usage & VGPUTextureUsage_RenderTarget) {
            if (vgpuIsDepthOrStencilFormat(desc->format)) {
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

        if (desc->type == VGPUTextureType_2D || desc->type == VGPUTextureType_Cube) {
            const D3D11_TEXTURE2D_DESC d3d11_desc = {
                .Width = desc->width,
                .Height = desc->height,
                .MipLevels = desc->mipLevels,
                .ArraySize = desc->arrayLayers * arraySizeMultiplier,
                .Format = result->format,
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
                renderer->device,
                &d3d11_desc,
                NULL,
                (ID3D11Texture2D**)&result->handle
            );
        }
        else if (desc->type == VGPUTextureType_3D) {
            const D3D11_TEXTURE3D_DESC d3d11_desc = {
                .Width = desc->width,
                .Height = desc->height,
                .Depth = desc->depth,
                .MipLevels = desc->mipLevels,
                .Format = result->format,
                .Usage = usage,
                .BindFlags = bindFlags,
                .CPUAccessFlags = CPUAccessFlags,
                .MiscFlags = miscFlags
            };

            hr = ID3D11Device_CreateTexture3D(
                renderer->device,
                &d3d11_desc,
                NULL,
                (ID3D11Texture3D**)&result->handle
            );
        }
    }

    return (VGPUTexture)result;
}

static void d3d11_destroy_texture(vgpu_renderer driver_data, VGPUTexture handle) {
    d3d11_renderer* renderer = (d3d11_renderer*)driver_data;
    D3D11Texture* texture = (D3D11Texture*)handle;
    ID3D11Resource_Release(texture->handle);
    VGPU_FREE(handle);
}

/* Context */
static void d3d11_BeginFrame(vgpu_renderer driverData, VGPUBackendContext* contextData);
static void d3d11_EndFrame(vgpu_renderer driverData, VGPUBackendContext* contextData);

static void d3d11_UpdateSwapChain(vgpu_renderer driverData, D3D11BackendContext* context, VGPUPixelFormat format) {
    ID3D11Texture2D* backbuffer;
    VHR(IDXGISwapChain1_GetBuffer(context->swapChain, 0, &D3D11_IID_ID3D11Texture2D, (void**)&backbuffer));

    D3D11_TEXTURE2D_DESC d3d_desc;
    ID3D11Texture2D_GetDesc(backbuffer, &d3d_desc);

    const VGPUTextureDescription textureDesc = {
        .width = d3d_desc.Width,
        .height = d3d_desc.Height,
        .depth = 1u,
        .mipLevels = d3d_desc.MipLevels,
        .arrayLayers = d3d_desc.ArraySize,
        .format = format,
        .type = VGPUTextureType_2D,
        .usage = VGPUTextureUsage_RenderTarget,
        .sampleCount = (VGPUTextureSampleCount)d3d_desc.SampleDesc.Count,
        .externalHandle = backbuffer
    };
    context->backbuffer = d3d11_create_texture(driverData, &textureDesc);
}

static VGPUContext d3d11_createContext(vgpu_renderer driverData, const VGPUContextDescription* desc) {
    d3d11_renderer* renderer = (d3d11_renderer*)driverData;
    D3D11BackendContext* backend = VGPU_ALLOC_HANDLE(D3D11BackendContext);

    /* Create optional swap chain */
    if (desc->swapchain.windowHandle) {
        backend->syncInterval = _vgpuGetSyncInterval(desc->swapchain.presentMode);
        backend->presentFlags = 0;

        if (desc->swapchain.presentMode == VGPUPresentMode_Immediate
            && renderer->factory_caps & DXGIFACTORY_CAPS_TEARING) {
            backend->presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
        }

        backend->swapChain = vgpu_d3d_create_swapchain(
            renderer->factory,
            (IUnknown*)renderer->device,
            renderer->factory_caps,
            desc->swapchain.windowHandle,
            desc->width, desc->height,
            desc->colorFormat,
            renderer->backbuffer_count /* 3 for triple buffering */
        );

        d3d11_UpdateSwapChain(driverData, backend, desc->colorFormat);
    }

    /* Create and return the context */
    VGPUContext result = VGPU_ALLOC(VGPUContext_T);
    result->BeginFrame = d3d11_BeginFrame;
    result->EndFrame = d3d11_EndFrame;
    result->renderer = driverData;
    result->backend = (VGPUBackendContext*)backend;
    return result;
}

static void d3d11_destroyContext(vgpu_renderer driver_data, VGPUContext handle) {
    D3D11BackendContext* backend = (D3D11BackendContext*)handle->backend;

    if (backend->swapChain) {
        d3d11_destroy_texture(driver_data, backend->backbuffer);
        IDXGISwapChain1_Release(backend->swapChain);
    }

    VGPU_FREE(handle->backend);
    VGPU_FREE(handle);
}

static void d3d11_BeginFrame(vgpu_renderer driverData, VGPUBackendContext* contextData) {
    D3D11BackendContext* context = (D3D11BackendContext*)driverData;
}

static void d3d11_EndFrame(vgpu_renderer driverData, VGPUBackendContext* contextData) {
    D3D11BackendContext* context = (D3D11BackendContext*)driverData;

    if (context->swapChain) {
        IDXGISwapChain1_Present(context->swapChain, context->syncInterval, context->presentFlags);
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

    HRESULT hr = vgpuD3D11CreateDevice(
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
}

static IDXGIAdapter1* d3d11_get_adapter(IDXGIFactory2* factory, bool lowPower)
{
    /* Detect adapter now. */
    IDXGIAdapter1* adapter = NULL;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
    IDXGIFactory6* factory6;
    HRESULT hr = IDXGIFactory2_QueryInterface(factory, &D3D_IID_IDXGIFactory6, (void**)&factory6);
    if (SUCCEEDED(hr))
    {
        // By default prefer high performance
        DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
        if (lowPower) {
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
        for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != IDXGIFactory2_EnumAdapters1(factory, i, &adapter); i++)
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

static vgpu_device d3d11_create_device(const vgpu_device_desc* desc) {
    vgpu_device_t* result;
    d3d11_renderer* renderer;

    /* Allocate and zero out the renderer */
    renderer = VGPU_ALLOC_HANDLE(d3d11_renderer);
    renderer->backbuffer_count = 2u;

    if (!d3d11_create_factory(renderer, desc->debug)) {
        return NULL;
    }

    IDXGIAdapter1* adapter = d3d11_get_adapter(renderer->factory, false);

    /* Create d3d11 device */
    {
        UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

        if (desc->debug && d3d11_sdk_layers_available())
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
        if (adapter)
        {
            hr = vgpuD3D11CreateDevice(
                (IDXGIAdapter*)adapter,
                D3D_DRIVER_TYPE_UNKNOWN,
                NULL,
                creation_flags,
                s_featureLevels,
                _countof(s_featureLevels),
                D3D11_SDK_VERSION,
                &temp_d3d_device,
                &renderer->feature_level,
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
            hr = vgpuD3D11CreateDevice(
                NULL,
                D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
                NULL,
                creation_flags,
                s_featureLevels,
                _countof(s_featureLevels),
                D3D11_SDK_VERSION,
                &temp_d3d_device,
                &renderer->feature_level,
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

        VHR(ID3D11Device_QueryInterface(temp_d3d_device, &D3D11_IID_ID3D11Device1, (void**)&renderer->device));
        VHR(ID3D11DeviceContext_QueryInterface(temp_d3d_context, &D3D11_IID_ID3D11DeviceContext1, (void**)&renderer->context));
        VHR(ID3D11DeviceContext_QueryInterface(temp_d3d_context, &D3D11_IID_ID3DUserDefinedAnnotation, (void**)&renderer->d3d_annotation));
        ID3D11DeviceContext_Release(temp_d3d_context);
        ID3D11Device_Release(temp_d3d_device);
    }

    IDXGIAdapter1_Release(adapter);


    /* Create and return the gpu_device */
    result = VGPU_ALLOC(vgpu_device_t);
    result->renderer = (vgpu_renderer)renderer;
    ASSIGN_DRIVER(d3d11);
    return result;
}

vgpu_driver d3d11_driver = {
    VGPUBackendType_D3D11,
    d3d11_is_supported,
    d3d11_create_device
};

#endif /* defined(VGPU_DRIVER_VULKAN) */
