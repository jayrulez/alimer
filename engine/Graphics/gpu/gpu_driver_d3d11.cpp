//
// Copyright (c) 2019-2020 Amer Koleci and contributors.
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

#if defined(ALIMER_ENABLE_D3D11)

#include "platform/platform.h"
#include "platform/application.h"
#define D3D11_NO_HELPERS
#include <d3d11_3.h>
#include "gpu_driver_d3d_common.h"
#include <stdio.h>

namespace gpu
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    PFN_CREATE_DXGI_FACTORY1 CreateDXGIFactory1;
    PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2;
    PFN_GET_DXGI_DEBUG_INTERFACE1 DXGIGetDebugInterface1;
    PFN_D3D11_CREATE_DEVICE D3D11CreateDevice;
#endif

    struct D3D11_Texture
    {
        enum { MAX_COUNT = 8192 };

        ID3D11Resource* handle;
    };

    struct D3D11_Buffer
    {
        enum { MAX_COUNT = 8192 };

        ID3D11Buffer* handle;
    };

    /* Global data */
    static struct {
        bool available_initialized;
        bool available;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        HMODULE dxgiDLL;
        HMODULE d3d11DLL;
#endif
        //agpu_device_caps caps;

        bool debug;
        bool vsync;

        IDXGIFactory2* factory;
        uint32_t factory_caps;

        ID3D11Device1* device;
        ID3D11DeviceContext1* context;
        ID3DUserDefinedAnnotation* d3d_annotation;
        D3D_FEATURE_LEVEL feature_level;
        bool is_lost;

        IDXGISwapChain1* swapChain;

        Pool<D3D11_Texture, D3D11_Texture::MAX_COUNT> textures;
        Pool<D3D11_Buffer, D3D11_Buffer::MAX_COUNT> buffers;
    } d3d11;

    /* Renderer functions */
    static bool D3D11_SdkLayersAvailable() {
        HRESULT hr = D3D11CreateDevice(
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


    static bool D3D11_CreateFactory(void)
    {
        SAFE_RELEASE(d3d11.factory);

        HRESULT hr = S_OK;

#if defined(_DEBUG)
        bool debugDXGI = false;

        if (d3d11.debug)
        {
            IDXGIInfoQueue* dxgiInfoQueue;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            if (DXGIGetDebugInterface1 && SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
#else
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
#endif
            {
                debugDXGI = true;
                hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&d3d11.factory));
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
            hr = CreateDXGIFactory1(IID_PPV_ARGS(&d3d11.factory));
            if (FAILED(hr)) {
                return false;
            }
        }

        d3d11.factory_caps = 0;

        // Disable FLIP if not on a supporting OS
        d3d11.factory_caps |= (uint32_t)DXGIFactoryCaps::FlipPresent;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        {
            IDXGIFactory4* factory4;
            HRESULT hr = d3d11.factory->QueryInterface(IID_PPV_ARGS(&factory4));
            if (FAILED(hr))
            {
                d3d11.factory_caps &= (uint32_t)DXGIFactoryCaps::FlipPresent;
            }
            SAFE_RELEASE(factory4);
        }
#endif

        // Check tearing support.
        {
            BOOL allowTearing = FALSE;
            IDXGIFactory5* factory5;
            HRESULT hr = d3d11.factory->QueryInterface(IID_PPV_ARGS(&factory5));
            if (SUCCEEDED(hr))
            {
                hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
                SAFE_RELEASE(factory5);
            }

            if (FAILED(hr) || !allowTearing)
            {
#ifdef _DEBUG
                OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
            }
            else
            {
                d3d11.factory_caps |= (uint32_t)DXGIFactoryCaps::Tearing;
            }
        }

        return true;
}

    static IDXGIAdapter1* D3D11_GetAdapter(PowerPreference powerPreference)
    {
        /* Detect adapter now. */
        IDXGIAdapter1* adapter = NULL;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        IDXGIFactory6* factory6;
        HRESULT hr = d3d11.factory->QueryInterface(IID_PPV_ARGS(&factory6));
        if (SUCCEEDED(hr))
        {
            // By default prefer high performance
            DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
            if (powerPreference == PowerPreference::LowPower) {
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

    static bool d3d11_init(const gpu_config* config)
    {
        d3d11.debug = config->debug;
        d3d11.vsync = true; // config->vsync;

        if (!D3D11_CreateFactory())
            return false;

        IDXGIAdapter1* dxgiAdapter = D3D11_GetAdapter(config->device_preference);

        /* Create d3d11 device */
        {
            UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

            if (d3d11.debug && D3D11_SdkLayersAvailable())
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
            ID3D11Device* temp_d3d_device = nullptr;
            ID3D11DeviceContext* temp_d3d_context = nullptr;

            HRESULT hr = E_FAIL;
            if (dxgiAdapter)
            {
                hr = D3D11CreateDevice(
                    dxgiAdapter,
                    D3D_DRIVER_TYPE_UNKNOWN,
                    nullptr,
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
                //agpu_log_error("No Direct3D hardware device found");
                AGPU_UNREACHABLE();
            }
#else
            if (FAILED(hr))
            {
                // If the initialization fails, fall back to the WARP device.
                // For more information on WARP, see:
                // http://go.microsoft.com/fwlink/?LinkId=286690
                hr = D3D11CreateDevice(
                    nullptr,
                    D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
                    nullptr,
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

                    D3D11_INFO_QUEUE_FILTER filter = {};
                    filter.DenyList.NumIDs = _countof(hide);
                    filter.DenyList.pIDList = hide;
                    d3d_info_queue->AddStorageFilterEntries(&filter);
                    d3d_info_queue->Release();
                }

                d3d_debug->Release();
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

            VHR(temp_d3d_device->QueryInterface(IID_PPV_ARGS(&d3d11.device)));
            VHR(temp_d3d_context->QueryInterface(IID_PPV_ARGS(&d3d11.context)));
            VHR(temp_d3d_context->QueryInterface(IID_PPV_ARGS(&d3d11.d3d_annotation)));
            temp_d3d_context->Release();
            temp_d3d_device->Release();
        }

        // Init pools.
        d3d11.textures.Init();
        d3d11.buffers.Init();

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        //swapChainDesc.Width = config->width;
        //swapChainDesc.Height = config->height;
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2u;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
#else
        swapChainDesc.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
#endif
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        // Create a swap chain for the window.
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        HWND window = nullptr; // Platform::get_native_handle()
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = TRUE;

        VHR(d3d11.factory->CreateSwapChainForHwnd(
            d3d11.device,
            window,
            &swapChainDesc,
            &fsSwapChainDesc,
            nullptr,
            &d3d11.swapChain
        ));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        VHR(d3d11.factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));
#else
        VHR(d3d11.factory->CreateSwapChainForCoreWindow(
            d3d11.device,
            Platform::get_native_handle(),
            &swapChainDesc,
            nullptr,
            &d3d11.swapChain
        ));
#endif
        //VHR(swapChain.As(&m_swapChain));

        // TODO: Init caps
        SAFE_RELEASE(dxgiAdapter);

        return true;
    }

    static void d3d11_shutdown(void)
    {
        SAFE_RELEASE(d3d11.context);
        SAFE_RELEASE(d3d11.d3d_annotation);

        ULONG refCount = d3d11.device->Release();
#if !defined(NDEBUG)
        if (refCount > 0)
        {
            //agpu_log_error("Direct3D11: There are %d unreleased references left on the device", refCount);

            ID3D11Debug* d3d11Debug = nullptr;
            if (SUCCEEDED(d3d11.device->QueryInterface(IID_PPV_ARGS(&d3d11Debug))))
            {
                d3d11Debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
                d3d11Debug->Release();
            }
        }
#else
        (void)refCount; // avoid warning
#endif

        SAFE_RELEASE(d3d11.factory);

#ifdef _DEBUG
        IDXGIDebug1* dxgiDebug1;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        if (DXGIGetDebugInterface1 && SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug1))))
#else
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug1))))
#endif
        {
            dxgiDebug1->ReportLiveObjects(D3D_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            dxgiDebug1->Release();
    }
#endif
        }

    static void d3d11_begin_frame(void)
    {
    }

    static void d3d11_end_frame(void)
    {
        HRESULT hr = E_FAIL;
        if (!d3d11.vsync)
        {
            // Recommended to always use tearing if supported when using a sync interval of 0.
            hr = d3d11.swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
        }
        else
        {
            // The first argument instructs DXGI to block until VSync, putting the application
            // to sleep until the next VSync. This ensures we don't waste any cycles rendering
            // frames that will never be displayed to the screen.
            hr = d3d11.swapChain->Present(1, 0);
        }

        // If the device was removed either by a disconnection or a driver upgrade, we
        // must recreate all device resources.
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
#ifdef _DEBUG
            char buff[64] = {};
            sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n",
                static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? d3d11.device->GetDeviceRemovedReason() : hr));
            OutputDebugStringA(buff);
#endif
            //HandleDeviceLost();
        }
        else
        {
            VHR(hr);

            if (!d3d11.factory->IsCurrent())
            {
                // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
                D3D11_CreateFactory();
            }
        }
    }

    /* Driver functions */
    static bool d3d11_supported(void) {
        if (d3d11.available_initialized) {
            return d3d11.available;
        }

        d3d11.available_initialized = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
        d3d11.dxgiDLL = LoadLibraryA("dxgi.dll");
        if (!d3d11.dxgiDLL) {
            return false;
        }

        CreateDXGIFactory1 = (PFN_CREATE_DXGI_FACTORY1)GetProcAddress(d3d11.dxgiDLL, "CreateDXGIFactory1");
        if (!CreateDXGIFactory1)
        {
            return false;
        }

        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(d3d11.dxgiDLL, "CreateDXGIFactory2");
        DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(d3d11.dxgiDLL, "DXGIGetDebugInterface1");

        d3d11.d3d11DLL = LoadLibraryA("d3d11.dll");
        if (!d3d11.d3d11DLL) {
            return false;
        }

        D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11.d3d11DLL, "D3D11CreateDevice");
        if (!D3D11CreateDevice) {
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

        HRESULT hr = D3D11CreateDevice(
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
    }

    static Renderer* d3d11_create_renderer(void)
    {
        static Renderer renderer = { nullptr };
        ASSIGN_DRIVER(d3d11);
        return &renderer;
    }

    Driver d3d11_driver = {
        BackendType::D3D11,
        d3d11_supported,
        d3d11_create_renderer
    };
}

#endif /* defined(ALIMER_ENABLE_D3D11) */
