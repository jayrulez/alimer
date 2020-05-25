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

#include "D3D11GraphicsProvider.h"
#include "D3D11GraphicsAdapter.h"
#include "D3D11GraphicsDevice.h"
#include "core/String.h"

namespace Alimer
{
    bool D3D11GraphicsProvider::IsAvailable()
    {
        static bool availableInitialized = false;
        static bool available = false;
        if (availableInitialized) {
            return available;
        }

        availableInitialized = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
        static HMODULE s_dxgiHandle = LoadLibraryW(L"dxgi.dll");
        if (s_dxgiHandle == nullptr)
            return false;

        CreateDXGIFactory2Func = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(s_dxgiHandle, "CreateDXGIFactory2");
        DXGIGetDebugInterface1Func = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(s_dxgiHandle, "DXGIGetDebugInterface1");
#endif

        available = true;
        return available;
    }

    D3D11GraphicsProvider::D3D11GraphicsProvider(bool validation_)
        : validation(validation_)
    {
        ALIMER_ASSERT(IsAvailable());

#if defined(_DEBUG)
        bool debugDXGI = false;
        if (validation)
        {
            IDXGIInfoQueue* dxgiInfoQueue;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            if (DXGIGetDebugInterface1Func != nullptr &&
                SUCCEEDED(DXGIGetDebugInterface1Func(0, IID_PPV_ARGS(&dxgiInfoQueue))))
#else
            if (SUCCEEDED(DXGIGetDebugInterface1Func(0, IID_PPV_ARGS(&dxgiInfoQueue))))
#endif
            {
                debugDXGI = true;

                ThrowIfFailed(CreateDXGIFactory2Func(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory)));

                dxgiInfoQueue->SetBreakOnSeverity(g_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
                dxgiInfoQueue->SetBreakOnSeverity(g_DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

                DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
                {
                    80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
                };
                DXGI_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                dxgiInfoQueue->AddStorageFilterEntries(g_DXGI_DEBUG_DXGI, &filter);
                dxgiInfoQueue->Release();
            }
        }

        validation = debugDXGI;

        if (!debugDXGI)
#endif

        {
            ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));
        }

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

    D3D11GraphicsProvider::~D3D11GraphicsProvider()
    {
        SafeRelease(dxgiFactory);

#ifdef _DEBUG
        IDXGIDebug* dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
        {
            dxgiDebug->ReportLiveObjects(g_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            dxgiDebug->Release();
        }
#endif
    }

    std::vector<std::shared_ptr<GraphicsAdapter>> D3D11GraphicsProvider::EnumerateGraphicsAdapters()
    {
        IDXGIAdapter1* dxgiAdapter;
        std::vector<std::shared_ptr<GraphicsAdapter>> adapters;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        IDXGIFactory6* dxgiFactory6 = nullptr;
        HRESULT hr = dxgiFactory->QueryInterface(&dxgiFactory6);
        if (SUCCEEDED(hr))
        {
            UINT adapterIndex = 0;
            while (dxgiFactory6->EnumAdapterByGpuPreference(adapterIndex++, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&dxgiAdapter)) != DXGI_ERROR_NOT_FOUND)
            {
                DXGI_ADAPTER_DESC1 desc;
                ThrowIfFailed(dxgiAdapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    dxgiAdapter->Release();

                    continue;
                }

                adapters.push_back(std::make_shared<D3D11GraphicsAdapter>(
                    dxgiAdapter,
                    Alimer::ToUtf8(desc.Description), desc.VendorId, desc.DeviceId)
                );
            }
        }

        SafeRelease(dxgiFactory6);
#endif

        if (adapters.empty())
        {
            UINT adapterIndex = 0;
            while (dxgiFactory6->EnumAdapters1(adapterIndex++, &dxgiAdapter) != DXGI_ERROR_NOT_FOUND)
            {
                DXGI_ADAPTER_DESC1 desc;
                ThrowIfFailed(dxgiAdapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    dxgiAdapter->Release();

                    continue;
                }

                adapters.push_back(std::make_shared<D3D11GraphicsAdapter>(
                    dxgiAdapter,
                    Alimer::ToUtf8(desc.Description), desc.VendorId, desc.DeviceId)
                );
            }
        }

        return adapters;
    }

    std::shared_ptr<GraphicsDevice> D3D11GraphicsProvider::CreateDevice(const std::shared_ptr<GraphicsAdapter>& adapter)
    {
        return std::make_shared<D3D11GraphicsDevice>(this, adapter);
    }

    /* D3D11GraphicsProviderFactory */
    std::unique_ptr<GraphicsProvider> D3D11GraphicsProviderFactory::CreateProvider(bool validation)
    {
        return std::make_unique<D3D11GraphicsProvider>(validation);
    }
}


