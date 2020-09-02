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

#if defined(ALIMER_ENABLE_D3D12)

#include "platform/platform.h"
#include "platform/application.h"
#include <d3d12.h>
#include "gpu_driver_d3d_common.h"
#include <stdio.h>

namespace gpu
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface;
    PFN_D3D12_CREATE_DEVICE D3D12CreateDevice;
#endif


    struct D3D12_Texture
    {
        enum { MAX_COUNT = 8192 };

        ID3D12Resource* handle;
    };

    struct D3D12_Buffer
    {
        enum { MAX_COUNT = 8192 };

        ID3D12Resource* handle;
    };

    /* Global data */
    static struct {
        bool available_initialized;
        bool available;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        HMODULE dxgiDLL;
        HMODULE d3d12DLL;
#endif
        //agpu_device_caps caps;

        bool debug;
        bool vsync;

        IDXGIFactory4* dxgiFactory;
        WORD dxgiFactoryFlags;
        DXGIFactoryCaps dxgiFactoryCaps;

        ID3D12Device* device;
        D3D_FEATURE_LEVEL feature_level;
        bool is_lost;

        IDXGISwapChain1* swapChain;

        Pool<D3D12_Texture, D3D12_Texture::MAX_COUNT> textures;
        Pool<D3D12_Buffer, D3D12_Buffer::MAX_COUNT> buffers;
    } d3d12;

    /* Renderer functions */
    static IDXGIAdapter1* D3D12_GetAdapter(PowerPreference powerPreference)
    {
        /* Detect adapter now. */
        IDXGIAdapter1* adapter = NULL;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        IDXGIFactory6* factory6;
        HRESULT hr = d3d12.dxgiFactory->QueryInterface(IID_PPV_ARGS(&factory6));
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
            for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != d3d12.dxgiFactory->EnumAdapters1(i, &adapter); i++)
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

    static bool d3d12_init(const gpu_config* config)
    {
        d3d12.debug = config->debug;
        //d3d12.vsync = config->vsync;

        d3d12.dxgiFactoryFlags = 0;

#if defined(_DEBUG)
        // Enable the debug layer (requires the Graphics Tools "optional feature").
        //
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        {
            ID3D12Debug* debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            {
                debugController->EnableDebugLayer();
                debugController->Release();
            }
            else
            {
                OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
            }

            IDXGIInfoQueue* dxgiInfoQueue;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
            {
                d3d12.dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

                dxgiInfoQueue->SetBreakOnSeverity(D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
                dxgiInfoQueue->SetBreakOnSeverity(D3D_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

                DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
                {
                    80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
                };
                DXGI_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                dxgiInfoQueue->AddStorageFilterEntries(D3D_DXGI_DEBUG_DXGI, &filter);
                dxgiInfoQueue->Release();
            }
        }
#endif

        VHR(CreateDXGIFactory2(d3d12.dxgiFactoryFlags, IID_PPV_ARGS(&d3d12.dxgiFactory)));

        // Setup factory caps
        d3d12.dxgiFactoryCaps = DXGIFactoryCaps::FlipPresent | DXGIFactoryCaps::HDR;

        // Check tearing support.
        {
            BOOL allowTearing = FALSE;
            IDXGIFactory5* dxgiFactory5 = nullptr;
            HRESULT hr = d3d12.dxgiFactory->QueryInterface(IID_PPV_ARGS(&dxgiFactory5));
            if (SUCCEEDED(hr))
            {
                hr = dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
            }

            SAFE_RELEASE(dxgiFactory5);

            if (FAILED(hr) || !allowTearing)
            {
#ifdef _DEBUG
                OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
            }
            else
            {
                d3d12.dxgiFactoryCaps |= DXGIFactoryCaps::Tearing;
            }
        }

        IDXGIAdapter1* dxgiAdapter = D3D12_GetAdapter(config->device_preference);

        // Init pools.
        d3d12.textures.Init();
        d3d12.buffers.Init();

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

        VHR(d3d12.dxgiFactory->CreateSwapChainForHwnd(
            d3d12.device,
            window,
            &swapChainDesc,
            &fsSwapChainDesc,
            nullptr,
            &d3d12.swapChain
        ));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        VHR(d3d12.dxgiFactory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));
#else
        VHR(d3d12.dxgiFactory->CreateSwapChainForCoreWindow(
            d3d12.device,
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

    static void d3d12_shutdown(void)
    {
        ULONG refCount = d3d12.device->Release();
#if !defined(NDEBUG)
        if (refCount > 0)
        {
            //agpu_log_error("Direct3D11: There are %d unreleased references left on the device", refCount);

            ID3D12DebugDevice* debugDevice = nullptr;
            if (SUCCEEDED(d3d12.device->QueryInterface<ID3D12DebugDevice>(&debugDevice)))
            {
                debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
                debugDevice->Release();
        }
    }
#else
        (void)refCount; // avoid warning
#endif

        SAFE_RELEASE(d3d12.dxgiFactory);

#ifdef _DEBUG
        IDXGIDebug1* dxgiDebug1;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug1))))
        {
            dxgiDebug1->ReportLiveObjects(D3D_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            dxgiDebug1->Release();
        }
#endif
}

    static void d3d12_begin_frame(void)
    {
    }

    static void d3d12_end_frame(void)
    {
        HRESULT hr = E_FAIL;
        if (!d3d12.vsync)
        {
            // Recommended to always use tearing if supported when using a sync interval of 0.
            hr = d3d12.swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
        }
        else
        {
            // The first argument instructs DXGI to block until VSync, putting the application
            // to sleep until the next VSync. This ensures we don't waste any cycles rendering
            // frames that will never be displayed to the screen.
            hr = d3d12.swapChain->Present(1, 0);
        }

        // If the device was removed either by a disconnection or a driver upgrade, we
        // must recreate all device resources.
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
#ifdef _DEBUG
            char buff[64] = {};
            sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n",
                static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? d3d12.device->GetDeviceRemovedReason() : hr));
            OutputDebugStringA(buff);
#endif
            //HandleDeviceLost();
        }
        else
        {
            VHR(hr);

            if (!d3d12.dxgiFactory->IsCurrent())
            {
                SAFE_RELEASE(d3d12.dxgiFactory);

                // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
                VHR(CreateDXGIFactory2(d3d12.dxgiFactoryFlags, IID_PPV_ARGS(&d3d12.dxgiFactory)));
            }
        }
    }

    /* Driver functions */
    static bool d3d12_supported(void) {
        if (d3d12.available_initialized) {
            return d3d12.available;
        }

        d3d12.available_initialized = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
        d3d12.dxgiDLL = LoadLibraryA("dxgi.dll");
        if (!d3d12.dxgiDLL) {
            return false;
        }

        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(d3d12.dxgiDLL, "CreateDXGIFactory2");
        if (!CreateDXGIFactory2)
        {
            return false;
        }
        DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(d3d12.dxgiDLL, "DXGIGetDebugInterface1");

        d3d12.d3d12DLL = LoadLibraryA("d3d12.dll");
        if (!d3d12.d3d12DLL) {
            return false;
        }

        D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(d3d12.d3d12DLL, "D3D12GetDebugInterface");
        D3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(d3d12.d3d12DLL, "D3D12CreateDevice");
        if (!D3D12CreateDevice) {
            return false;
        }
#endif

        d3d12.available = true;
        return true;
    }

    static Renderer* d3d12_create_renderer(void)
    {
        static Renderer renderer = { nullptr };
        ASSIGN_DRIVER(d3d12);
        return &renderer;
    }

    Driver d3d12_driver = {
        BackendType::D3D12,
        d3d12_supported,
        d3d12_create_renderer
    };
}

#endif /* defined(ALIMER_ENABLE_D3D11) */
