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
    uint32_t device_count;

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
#endif

typedef struct d3d11_renderer
{
    IDXGIFactory2* factory;
    uint32_t factory_caps;

    ID3D11Device1* device;
    ID3D11DeviceContext1* context;
    //ID3DUserDefinedAnnotation* d3d_annotation;
    D3D_FEATURE_LEVEL feature_level;
} d3d11_renderer;

typedef struct vgpu_context_d3d11 {
    IDXGISwapChain1* swapchain;
} vgpu_context_d3d11;

/* Device functions */
static bool d3d11_create_factory(d3d11_renderer* renderer, bool validation)
{
    SAFE_RELEASE(renderer->factory);

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

    SAFE_RELEASE(renderer->context);
    //SAFE_RELEASE(renderer->d3d_annotation);

    d3d11.device_count--;

    ULONG ref_count = ID3D11Device1_Release(renderer->device);
#if !defined(NDEBUG)
    if (ref_count > 0)
    {
        //gpuLog(GPULogLevel_Error, "Direct3D11: There are %d unreleased references left on the device", ref_count);

        ID3D11Debug* d3d_debug = NULL;
        if (SUCCEEDED(ID3D11Device1_QueryInterface(renderer->device, &D3D11_IID_ID3D11Debug, (void**)&d3d_debug)))
        {
            ID3D11Debug_ReportLiveDeviceObjects(d3d_debug, D3D11_RLDO_SUMMARY | D3D11_RLDO_IGNORE_INTERNAL);
            ID3D11Debug_Release(d3d_debug);
        }
    }
#else
    (void)ref_count; // avoid warning
#endif

#ifdef _DEBUG
    if (d3d11.device_count == 0)
    {
        SAFE_RELEASE(renderer->factory);

        IDXGIDebug1* dxgiDebug;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (d3d11.dxgi.DXGIGetDebugInterface1 &&
            SUCCEEDED(vgpuDXGIGetDebugInterface1(0, &D3D_IID_IDXGIDebug1, (void**)&dxgiDebug)))
#else
        if (SUCCEEDED(vgpuDXGIGetDebugInterface1(0, &D3D_IID_IDXGIDebug1, (void**)&dxgiDebug)))
#endif
        {
            IDXGIDebug1_ReportLiveObjects(dxgiDebug, D3D_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL);
            IDXGIDebug1_Release(dxgiDebug);
        }
    }
#endif

    vgpu_free(renderer);
    vgpu_free(device);
}

static vgpu_context d3d11_create_context(vgpu_renderer driver_data, const vgpu_context_info* info) {
    d3d11_renderer* renderer = (d3d11_renderer*)driver_data;
    vgpu_context_d3d11* result = (vgpu_context_d3d11*)vgpu_malloc(sizeof(vgpu_context_d3d11));
    if (info->swapchain_info.handle) {
        result->swapchain = vgpu_d3d_create_swapchain(
            renderer->factory,
            (IUnknown*)renderer->device,
            renderer->factory_caps,
            info->swapchain_info.handle,
            info->width, info->height,
            info->max_inflight_frames
        );
    }

    return (vgpu_context)result;
}

static void d3d11_destroy_context(vgpu_renderer driver_data, vgpu_context context) {
    d3d11_renderer* renderer = (d3d11_renderer*)driver_data;
    vgpu_context_d3d11* d3d_context = (vgpu_context_d3d11*)context;
    if (d3d_context->swapchain) {
        IDXGISwapChain1_Release(d3d_context->swapchain);
    }

    vgpu_free(context);
}

static void d3d11_begin_frame(vgpu_renderer driver_data, vgpu_context context) {
}

static void d3d11_end_frame(vgpu_renderer driver_data, vgpu_context context) {
    vgpu_context_d3d11* d3d_context = (vgpu_context_d3d11*)context;
    if (d3d_context->swapchain) {
        IDXGISwapChain1_Present(d3d_context->swapchain, 1, 0);
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

static vgpu_device d3d11_create_device(bool validation) {
    vgpu_device_t* result;
    d3d11_renderer* renderer;

    /* Allocate and zero out the renderer */
    renderer = (d3d11_renderer*)vgpu_malloc(sizeof(d3d11_renderer));
    memset(renderer, 0, sizeof(d3d11_renderer));

    if (!d3d11_create_factory(renderer, validation)) {
        return NULL;
    }

    IDXGIAdapter1* adapter = d3d11_get_adapter(renderer->factory, false);

    /* Create d3d11 device */
    {
        UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
        if (validation && d3d11_sdk_layers_available())
        {
            // If the project is in a debug build, enable debugging via SDK Layers with this flag.
            creation_flags |= D3D11_CREATE_DEVICE_DEBUG;
        }
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
            }

            SAFE_RELEASE(d3d_info_queue);
            SAFE_RELEASE(d3d_debug);
        }
#endif

        VHR(ID3D11Device_QueryInterface(temp_d3d_device, &D3D11_IID_ID3D11Device1, (void**)&renderer->device));
        VHR(ID3D11DeviceContext_QueryInterface(temp_d3d_context, &D3D11_IID_ID3D11DeviceContext1, (void**)&renderer->context));
        //VHR(ID3D11DeviceContext_QueryInterface(temp_d3d_context, &D3D11_IID_ID3DUserDefinedAnnotation, (void**)&renderer->d3d_annotation));
        SAFE_RELEASE(temp_d3d_context);
        SAFE_RELEASE(temp_d3d_device);
    }

    IDXGIAdapter1_Release(adapter);

    d3d11.device_count++;

    /* Create and return the gpu_device */
    result = (vgpu_device_t*)vgpu_malloc(sizeof(vgpu_device_t));
    result->renderer = (vgpu_renderer)renderer;
    ASSIGN_DRIVER(d3d11);
    return result;
}

vgpu_driver d3d11_driver = {
    VGPU_BACKEND_TYPE_D3D11,
    d3d11_is_supported,
    d3d11_create_device
};

#endif /* defined(VGPU_DRIVER_VULKAN) */
