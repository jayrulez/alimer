//
// Copyright (c) 2020 Amer Koleci.
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

#include <d3d11_1.h>
#include "agpu_d3d_common.h"

#define AGPU_VHR(hr) if (FAILED(hr)) { agpu_log_error("D3D11 failure, error code: %08X", hr); }
#define AGPU_SAFE_RELEASE(obj) if ((obj)) { (obj)->Release(); (obj) = nullptr; }

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

    bool validation;
    IDXGIFactory2* factory;
    uint32_t factory_caps;

    ID3D11Device1* device;
    ID3D11DeviceContext1* context;
    ID3DUserDefinedAnnotation* annotation;
    D3D_FEATURE_LEVEL feature_level;
} d3d11;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#   define _agpu_CreateDXGIFactory1 d3d11.dxgi.CreateDXGIFactory1
#   define _agpu_CreateDXGIFactory2 d3d11.dxgi.CreateDXGIFactory2
#   define _agpu_D3D11CreateDevice d3d11.D3D11CreateDevice
#else
#   define _agpu_CreateDXGIFactory1 CreateDXGIFactory1
#   define _agpu_CreateDXGIFactory2 CreateDXGIFactory2
#   define _agpu_D3D11CreateDevice D3D11CreateDevice
#endif

static bool _agpu_d3d11_sdk_layers_available() {
    HRESULT hr = _agpu_D3D11CreateDevice(
        NULL,
        D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
        NULL,
        D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
        NULL,                    // Any feature level will do.
        0,
        D3D11_SDK_VERSION,
        NULL,                    // No need to keep the D3D device reference.
        NULL,                    // No need to know the feature level.
        NULL                     // No need to keep the D3D device context reference.
    );

    return SUCCEEDED(hr);
}

struct agpu_buffer_d3d11 {
    ID3D11Buffer* handle;
};

/* Renderer functions */
static bool _agpu_d3d11_create_factory(bool debug) {
    if (d3d11.factory) {
        d3d11.factory->Release();
        d3d11.factory = nullptr;
    }

    HRESULT hr = S_OK;

#if defined(_DEBUG)
    bool debugDXGI = false;

    if (debug)
    {
        IDXGIInfoQueue* dxgi_info_queue;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (d3d11.dxgi.DXGIGetDebugInterface1 &&
            SUCCEEDED(d3d11.dxgi.DXGIGetDebugInterface1(0, __uuidof(IDXGIInfoQueue), (void**)&dxgi_info_queue)))
#else
        if (SUCCEEDED(DXGIGetDebugInterface1(0, __uuidof(IDXGIInfoQueue), (void**)&dxgi_info_queue)))
#endif
        {
            debugDXGI = true;
            hr = _agpu_CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, __uuidof(IDXGIFactory2), (void**)&d3d11.factory);
            if (FAILED(hr)) {
                return false;
            }

            AGPU_VHR(dxgi_info_queue->SetBreakOnSeverity(D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE));
            AGPU_VHR(dxgi_info_queue->SetBreakOnSeverity(D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE));
            AGPU_VHR(dxgi_info_queue->SetBreakOnSeverity(D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, FALSE));

            DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
            {
                80 // IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides.
            };

            DXGI_INFO_QUEUE_FILTER filter;
            memset(&filter, 0, sizeof(filter));
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            dxgi_info_queue->AddStorageFilterEntries(D3D_DXGI_DEBUG_DXGI, &filter);
            dxgi_info_queue->Release();
        }
    }

    if (!debugDXGI)
#endif
    {
        hr = _agpu_CreateDXGIFactory1(__uuidof(IDXGIFactory2), (void**)&d3d11.factory);
        if (FAILED(hr)) {
            return false;
        }
    }

    d3d11.factory_caps = 0;

    // Disable FLIP if not on a supporting OS
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    {
        IDXGIFactory4* factory4;
        HRESULT hr = d3d11.factory->QueryInterface(__uuidof(IDXGIFactory4), (void**)&factory4);
        if (SUCCEEDED(hr))
        {
            d3d11.factory_caps |= DXGIFACTORY_CAPS_FLIP_PRESENT;
        }
        AGPU_SAFE_RELEASE(factory4);
    }

    {
        IDXGIFactory5* factory5;
        HRESULT hr = d3d11.factory->QueryInterface(__uuidof(IDXGIFactory5), (void**)&factory5);
        if (SUCCEEDED(hr))
        {
            d3d11.factory_caps |= DXGIFACTORY_CAPS_HDR;
        }
        AGPU_SAFE_RELEASE(factory5);
    }
#else
    d3d11.factory_caps |= DXGIFACTORY_CAPS_FLIP_PRESENT;
    d3d11.factory_caps |= DXGIFACTORY_CAPS_HDR;
#endif

    // Check tearing support.
    {
        BOOL allowTearing = FALSE;
        IDXGIFactory5* factory5;
        HRESULT hr = d3d11.factory->QueryInterface(__uuidof(IDXGIFactory5), (void**)&factory5);
        if (SUCCEEDED(hr))
        {
            hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
        }

        AGPU_SAFE_RELEASE(factory5);

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

static IDXGIAdapter1* _agpu_d3d11_get_adapter(bool low_power) {
    /* Detect adapter now. */
    IDXGIAdapter1* adapter = NULL;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
    {
        IDXGIFactory6* factory6;
        HRESULT hr = d3d11.factory->QueryInterface(__uuidof(IDXGIFactory6), (void**)&factory6);
        if (SUCCEEDED(hr))
        {
            // By default prefer high performance
            DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
            if (low_power) {
                gpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
            }

            for (uint32_t i = 0;
                DXGI_ERROR_NOT_FOUND != factory6->EnumAdapterByGpuPreference(i, gpuPreference, __uuidof(IDXGIAdapter1), (void**)&adapter); i++)
            {
                DXGI_ADAPTER_DESC1 desc;
                AGPU_VHR(adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    adapter->Release();
                    continue;
                }

                break;
            }

            factory6->Release();
        }
    }
#endif

    if (!adapter)
    {
        for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != d3d11.factory->EnumAdapters1(i, &adapter); i++)
        {
            DXGI_ADAPTER_DESC1 desc;
            AGPU_VHR(adapter->GetDesc1(&desc));

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

static bool agpu_d3d11_init(const char* app_name, const agpu_config* config) {
    _AGPU_UNUSED(app_name);

    if (!_agpu_d3d11_create_factory(config->debug)) {
        return false;
    }

    IDXGIAdapter1* dxgi_adapter = _agpu_d3d11_get_adapter(false);

    /* Create d3d11 device */
    {
        UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

        if (config->debug)
        {
            if (_agpu_d3d11_sdk_layers_available())
            {
                // If the project is in a debug build, enable debugging via SDK Layers with this flag.
                creation_flags |= D3D11_CREATE_DEVICE_DEBUG;
            }
            else
            {
                OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
            }
        }

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
            hr = _agpu_D3D11CreateDevice(
                dxgi_adapter,
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
            __assume(false);
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
            //gpu_device_destroy(device);
            return false;
        }

        if (config->debug)
        {
            ID3D11Debug* d3dDebug;
            if (SUCCEEDED(temp_d3d_device->QueryInterface(__uuidof(ID3D11Debug), (void**)&d3dDebug)))
            {
                ID3D11InfoQueue* d3dInfoQueue;
                if (SUCCEEDED(d3dDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&d3dInfoQueue)))
                {
#ifdef _DEBUG
                    d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
                    d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
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
        }

        AGPU_VHR(temp_d3d_device->QueryInterface(__uuidof(ID3D11Device1), (void**)&d3d11.device));
        AGPU_VHR(temp_d3d_context->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&d3d11.context));
        AGPU_VHR(temp_d3d_context->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)&d3d11.annotation));
        AGPU_SAFE_RELEASE(temp_d3d_context);
        AGPU_SAFE_RELEASE(temp_d3d_device);
    }

    DXGI_ADAPTER_DESC1 adapter_desc;
    AGPU_VHR(dxgi_adapter->GetDesc1(&adapter_desc));
    agpu_log_info("agpu driver: Direct3D11");
    agpu_log_info("D3D11 Vendor: %S", adapter_desc.Description);

    dxgi_adapter->Release();
    dxgi_adapter = nullptr;

    return true;
}

static void agpu_d3d11_shutdown(void) {
    AGPU_SAFE_RELEASE(d3d11.context);
    AGPU_SAFE_RELEASE(d3d11.annotation);

    ULONG ref_count = d3d11.device->Release();
#if !defined(NDEBUG)
    if (ref_count > 0)
    {
        agpu_log_error("Direct3D11: There are %d unreleased references left on the device", ref_count);

        ID3D11Debug* d3d_debug = nullptr;
        if (SUCCEEDED(d3d11.device->QueryInterface(__uuidof(ID3D11Debug), (void**)&d3d_debug)))
        {
            d3d_debug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY | D3D11_RLDO_IGNORE_INTERNAL);
            d3d_debug->Release();
        }
    }
#else
    (void)ref_count; // avoid warning
#endif

    AGPU_SAFE_RELEASE(d3d11.factory);

#ifdef _DEBUG
    IDXGIDebug1* dxgi_debug1;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    if (d3d11.dxgi.DXGIGetDebugInterface1 &&
        SUCCEEDED(d3d11.dxgi.DXGIGetDebugInterface1(0, __uuidof(IDXGIDebug1), (void**)&dxgi_debug1)))
#else
    if (SUCCEEDED(DXGIGetDebugInterface1(0, __uuidof(IDXGIDebug1), (void**)&dxgi_debug1)))
#endif
    {
        dxgi_debug1->ReportLiveObjects(D3D_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        dxgi_debug1->Release();
}
#endif

    memset(&d3d11, 0, sizeof(d3d11));
}

static void agpu_d3d11_frame_begin(void) {
}

static void agpu_d3d11_frame_end(void) {

}

static agpu_buffer agpu_d3d11_create_buffer(const agpu_buffer_info* info) {
    agpu_buffer_d3d11* buffer = _AGPU_ALLOC_HANDLE(agpu_buffer_d3d11);
    return (agpu_buffer)buffer;
}

static void agpu_gl_destroy_buffer(agpu_buffer handle) {
    agpu_buffer_d3d11* buffer = (agpu_buffer_d3d11*)handle;
    buffer->handle->Release();
    _AGPU_FREE(buffer);
}

/* Driver functions */
static bool agpu_d3d11_is_supported(void) {
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
        _AGPU_COUNTOF(s_featureLevels),
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

static agpu_renderer* agpu_d3d11_init_renderer(void) {
    static agpu_renderer renderer = { 0 };
    renderer.init = agpu_d3d11_init;
    renderer.shutdown = agpu_d3d11_shutdown;
    renderer.frame_begin = agpu_d3d11_frame_begin;
    renderer.frame_end = agpu_d3d11_frame_end;
    return &renderer;
}

agpu_driver d3d11_driver = {
    AGPU_BACKEND_D3D11,
    agpu_d3d11_is_supported,
    agpu_d3d11_init_renderer
};

#endif /* defined(AGPU_DRIVER_OPENGL) */
