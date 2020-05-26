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

#include "D3D12GraphicsProvider.h"
#include "D3D12GraphicsAdapter.h"
#include "D3D12GraphicsDevice.h"
#include "core/String.h"

namespace Alimer
{
    bool D3D12GraphicsProvider::IsAvailable()
    {
        static bool availableInitialized = false;
        static bool available = false;
        if (availableInitialized) {
            return available;
        }

        availableInitialized = true;

        // Create temp factory and detect adapter support
        {
            IDXGIFactory4* tempFactory;
            HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&tempFactory));
            if (FAILED(hr))
            {
                return false;
            }
            SafeRelease(tempFactory);

            if (SUCCEEDED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                available = true;
            }
        }

        return available;
    }

    D3D12GraphicsProvider::D3D12GraphicsProvider(bool validation_)
        : validation(validation_)
    {
        ALIMER_ASSERT(IsAvailable());

#if defined(_DEBUG)
        // Enable the debug layer (requires the Graphics Tools "optional feature").
        //
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        if (validation)
        {
            ID3D12Debug* debugController;

            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            {
                debugController->EnableDebugLayer();

                ID3D12Debug1* debugController1;
                if (SUCCEEDED(debugController->QueryInterface(&debugController1)))
                {
                    debugController1->SetEnableGPUBasedValidation(true);
                    //debugController1->SetEnableSynchronizedCommandQueueValidation(true);
                    debugController1->Release();
                }
            }
            else
            {
                OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
            }

            SafeRelease(debugController);

            IDXGIInfoQueue* dxgiInfoQueue;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
            {
                dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

                dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
                dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

                DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
                {
                    80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
                };
                DXGI_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
                dxgiInfoQueue->Release();
            }
        }
#endif
        VHR(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

        BOOL allowTearing = FALSE;
        IDXGIFactory5* dxgiFactory5 = nullptr;
        HRESULT hr = dxgiFactory->QueryInterface(&dxgiFactory5);
        if (SUCCEEDED(hr))
        {
            hr = dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
        }

        if (FAILED(hr) || !allowTearing)
        {
            isTearingSupported = false;
#ifdef _DEBUG
            OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
        }
        else
        {
            isTearingSupported = true;
        }
        SafeRelease(dxgiFactory5);
    }

    D3D12GraphicsProvider::~D3D12GraphicsProvider()
    {
        SafeRelease(dxgiFactory);

#ifdef _DEBUG
        IDXGIDebug* dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
        {
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            dxgiDebug->Release();
        }
#endif
    }

    Array<std::shared_ptr<GraphicsAdapter>> D3D12GraphicsProvider::EnumerateGraphicsAdapters()
    {
        IDXGIAdapter1* dxgiAdapter;
        Array<std::shared_ptr<GraphicsAdapter>> adapters;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        IDXGIFactory6* dxgiFactory6 = nullptr;
        HRESULT hr = dxgiFactory->QueryInterface(&dxgiFactory6);
        if (SUCCEEDED(hr))
        {
            UINT adapterIndex = 0;
            while (dxgiFactory6->EnumAdapterByGpuPreference(adapterIndex++, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&dxgiAdapter)) != DXGI_ERROR_NOT_FOUND)
            {
                DXGI_ADAPTER_DESC1 desc;
                VHR(dxgiAdapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    dxgiAdapter->Release();

                    continue;
                }

                // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
                if (SUCCEEDED(D3D12CreateDevice(dxgiAdapter, minFeatureLevel, _uuidof(ID3D12Device), nullptr)))
                {
                    adapters.Push(std::make_shared<D3D12GraphicsAdapter>(
                        dxgiAdapter,
                        Alimer::ToUtf8(desc.Description), desc.VendorId, desc.DeviceId)
                    );

                    break;
                }
            }
        }

        SafeRelease(dxgiFactory6);
#endif

        if (adapters.Empty())
        {
            UINT adapterIndex = 0;
            while (dxgiFactory6->EnumAdapters1(adapterIndex++, &dxgiAdapter) != DXGI_ERROR_NOT_FOUND)
            {
                DXGI_ADAPTER_DESC1 desc;
                VHR(dxgiAdapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    dxgiAdapter->Release();

                    continue;
                }

                // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
                if (SUCCEEDED(D3D12CreateDevice(dxgiAdapter, minFeatureLevel, _uuidof(ID3D12Device), nullptr)))
                {
                    adapters.Push(std::make_shared<D3D12GraphicsAdapter>(
                        dxgiAdapter,
                        Alimer::ToUtf8(desc.Description), desc.VendorId, desc.DeviceId)
                    );

                    break;
                }
            }
        }

#if !defined(NDEBUG)
        if (adapters.Empty())
        {
            // Try WARP12 instead
            if (FAILED(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter))))
            {
                LOG_ERROR("WARP12 not available. Enable the 'Graphics Tools' optional feature");
            }

            OutputDebugStringA("Direct3D Adapter - WARP12\n");
        }
#endif

        return adapters;
    }

    std::shared_ptr<GraphicsDevice> D3D12GraphicsProvider::CreateDevice(const std::shared_ptr<GraphicsAdapter>& adapter)
    {
        return std::make_shared<D3D12GraphicsDevice>(this, adapter);
    }

    /* D3D12GraphicsProviderFactory */
    std::unique_ptr<GraphicsProvider> D3D12GraphicsProviderFactory::CreateProvider(bool validation)
    {
        return std::make_unique<D3D12GraphicsProvider>(validation);
    }
}


