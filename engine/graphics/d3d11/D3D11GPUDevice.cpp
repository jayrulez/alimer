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

#include "D3D11GPUDevice.h"
//#include "D3D11Framebuffer.h"

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

namespace alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    // DXGI functions
    static PFN_CREATE_DXGI_FACTORY CreateDXGIFactory1 = nullptr;
    static PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2 = nullptr;
    static PFN_GET_DXGI_DEBUG_INTERFACE DXGIGetDebugInterface = nullptr;
    static PFN_GET_DXGI_DEBUG_INTERFACE1 DXGIGetDebugInterface1 = nullptr;

    // D3D12 functions.
    static PFN_D3D11_CREATE_DEVICE D3D11CreateDevice = nullptr;
#endif

#if defined(_DEBUG)
    // Check for SDK Layer support.
    inline bool SdkLayersAvailable()
    {
        HRESULT hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
            nullptr,
            D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
            nullptr,                    // Any feature level will do.
            0,
            D3D11_SDK_VERSION,
            nullptr,                    // No need to keep the D3D device reference.
            nullptr,                    // No need to know the feature level.
            nullptr                     // No need to keep the D3D device context reference.
        );

        return SUCCEEDED(hr);
    }
#endif

    bool D3D11GPUDevice::IsAvailable()
    {
        static bool availableInitialized = false;
        static bool available = false;
        if (availableInitialized) {
            return available;
        }

        availableInitialized = true;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        static HMODULE s_dxgiModule = LoadLibraryW(L"dxgi.dll");
        if (s_dxgiModule == nullptr)
            return false;

        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(s_dxgiModule, "CreateDXGIFactory2");
        CreateDXGIFactory1 = (PFN_CREATE_DXGI_FACTORY)GetProcAddress(s_dxgiModule, "CreateDXGIFactory1");
        if (CreateDXGIFactory1 == nullptr)
            CreateDXGIFactory1 = (PFN_CREATE_DXGI_FACTORY)GetProcAddress(s_dxgiModule, "CreateDXGIFactory");

        DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(s_dxgiModule, "DXGIGetDebugInterface1");
        DXGIGetDebugInterface = (PFN_GET_DXGI_DEBUG_INTERFACE)GetProcAddress(s_dxgiModule, "DXGIGetDebugInterface");

        static HMODULE s_d3d11Module = LoadLibraryW(L"d3d11.dll");
        if (s_d3d11Module == nullptr) {
            return false;
        }

        D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(s_d3d11Module, "D3D11CreateDevice");
        if (D3D11CreateDevice == nullptr)
            return false;
#endif

        available = true;
        return available;
    }

    D3D11GPUDevice::D3D11GPUDevice(Window* window_, const Desc& desc_)
        : GPUDevice(window_, desc_)
    {
    }

    D3D11GPUDevice::~D3D11GPUDevice()
    {
        WaitIdle();
        BackendShutdown();
    }

    bool D3D11GPUDevice::BackendInit()
    {
        CreateFactory();

        // Detect the driver to use.
        ComPtr<IDXGIAdapter1> adapter = nullptr;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        IDXGIFactory6* dxgiFactory6;
        HRESULT hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory6), (void**)&dxgiFactory6);
        if (SUCCEEDED(hr))
        {
            // By default prefer high performance
            DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
            if (desc.preferredAdapterType == GPUAdapterType::IntegratedGPU) {
                gpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
            } else if (desc.preferredAdapterType == GPUAdapterType::Unknown) {
                gpuPreference = DXGI_GPU_PREFERENCE_UNSPECIFIED;
            }

            for (UINT i = 0;
                SUCCEEDED(dxgiFactory6->EnumAdapterByGpuPreference(i, gpuPreference, IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf())));
                i++)
            {
                DXGI_ADAPTER_DESC1 desc;
                ThrowIfFailed(adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    continue;
                }

                break;
            }
        }

        SafeRelease(dxgiFactory6);
#endif

        if (!adapter)
        {
            for (UINT i = 0; SUCCEEDED(dxgiFactory->EnumAdapters1(i, adapter.ReleaseAndGetAddressOf())); ++i)
            {
                DXGI_ADAPTER_DESC1 desc;
                ThrowIfFailed(adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    continue;
                }

                break;
            }
        }

        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
        if (any(desc.flags & GPUDeviceFlags::Validation))
        {
            if (SdkLayersAvailable())
            {
                // If the project is in a debug build, enable debugging via SDK Layers with this flag.
                creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
            }
            else
            {
                OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
            }
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
        ID3D11Device* tempDevice;
        ID3D11DeviceContext* tempContext;

        hr = D3D11CreateDevice(
            adapter.Get(),
            D3D_DRIVER_TYPE_UNKNOWN,
            nullptr,
            creationFlags,
            s_featureLevels,
            _countof(s_featureLevels),
            D3D11_SDK_VERSION,
            &tempDevice,
            &d3dFeatureLevel,
            &tempContext
        );

#if !defined(NDEBUG)
        if (FAILED(hr))
        {
            // If the initialization fails, fall back to the WARP device.
            // For more information on WARP, see:
            // http://go.microsoft.com/fwlink/?LinkId=286690
            hr = D3D11CreateDevice(
                nullptr,
                D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
                nullptr,
                creationFlags,
                s_featureLevels,
                _countof(s_featureLevels),
                D3D11_SDK_VERSION,
                &tempDevice,
                &d3dFeatureLevel,
                &tempContext
            );

            if (SUCCEEDED(hr))
            {
                OutputDebugStringA("Direct3D Adapter - WARP\n");
            }
        }
#endif

#ifndef NDEBUG
        if (any(desc.flags & GPUDeviceFlags::Validation))
        {
            ID3D11Debug* d3dDebug;
            if (SUCCEEDED(tempDevice->QueryInterface(__uuidof(ID3D11Debug), (void**)&d3dDebug)))
            {
                ID3D11InfoQueue* d3dInfoQueue;
                if (SUCCEEDED(d3dDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&d3dInfoQueue)))
                {
#ifdef _DEBUG
                    d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
                    d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
                    d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, FALSE);
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
#endif

        ThrowIfFailed(tempDevice->QueryInterface(__uuidof(ID3D11Device1), (void**)&d3dDevice));
        ThrowIfFailed(tempContext->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&d3dContext));

        SafeRelease(tempDevice);
        SafeRelease(tempContext);

        // Init caps and features.
        InitCapabilities(adapter.Get());

        /*if (desc.native_window_handle != nullptr)
        {
            SwapChainDescriptor swapChainDesc = {};
            swapChainDesc.nativeWindowHandle = desc.native_window_handle;
            createFramebuffer(&swapChainDesc);
        }*/

        return true;
    }

    void D3D11GPUDevice::InitCapabilities(IDXGIAdapter1* adapter)
    {
    }

    void D3D11GPUDevice::BackendShutdown()
    {
        SafeRelease(d3dContext);

        
#if !defined(NDEBUG)
        ULONG refCount = d3dDevice->Release();
        if (refCount > 0)
        {
            ALIMER_LOGD("Direct3D11: There are %u unreleased references left on the device", refCount);

            ID3D11Debug* debugDevice;
            if (SUCCEEDED(d3dDevice->QueryInterface(__uuidof(ID3D11Debug), (void**)&debugDevice)))
            {
                debugDevice->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_SUMMARY | D3D11_RLDO_IGNORE_INTERNAL);
                debugDevice->Release();
            }
        }
#else
        d3dDevice->Release();
#endif
    }

    void D3D11GPUDevice::CreateFactory()
    {
        SafeRelease(dxgiFactory);

#if defined(_DEBUG)
        bool debugDXGI = false;
        {
            ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
            {
                debugDXGI = true;
                ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory)));

                dxgiInfoQueue->SetBreakOnSeverity(g_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
                dxgiInfoQueue->SetBreakOnSeverity(g_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

                DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
                {
                    80, // IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides.
                };
                DXGI_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                dxgiInfoQueue->AddStorageFilterEntries(g_DXGI_DEBUG_DXGI, &filter);
            }
        }

        if (!debugDXGI)
#endif
        {
            ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));
        }

        // Determines whether tearing support is available for fullscreen borderless windows.
        IDXGIFactory5* dxgiFactory5;
        HRESULT hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory5), (void**)&dxgiFactory5);
        if (SUCCEEDED(hr))
        {
            BOOL allowTearing = FALSE;
            HRESULT tearingSupportHR = dxgiFactory5->CheckFeatureSupport(
                DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing)
            );

            if (SUCCEEDED(tearingSupportHR) && allowTearing)
            {
                isTearingSupported = true;
            }
        }
        SafeRelease(dxgiFactory5);
    }

    void D3D11GPUDevice::WaitIdle()
    {
        d3dContext->Flush();
    }

    void D3D11GPUDevice::Commit()
    {
        /* HRESULT hr = S_OK;
         UINT syncInterval = vsync ? 1 : 0;
         UINT presentFlags = (isTearingSupported && !vsync) ? DXGI_PRESENT_ALLOW_TEARING : 0;

         for (auto swap_chain : swap_chains)
         {
             hr = swap_chain->present(syncInterval, presentFlags);
             if (hr == DXGI_ERROR_DEVICE_REMOVED
                 || hr == DXGI_ERROR_DEVICE_HUNG
                 || hr == DXGI_ERROR_DEVICE_RESET
                 || hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR
                 || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
             {
             }
         }*/
    }

    /*std::shared_ptr<Framebuffer> D3D11GPUDevice::createFramebufferCore(const SwapChainDescriptor* descriptor)
    {
        swap_chains.push_back(std::make_shared<D3D11Framebuffer>(this, descriptor));
        return swap_chains.back();
    }*/
}
