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
#include "core/Assert.h"

namespace alimer
{
    bool D3D12GraphicsProvider::IsAvailable()
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

        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(s_dxgiHandle, "CreateDXGIFactory2");
        if (CreateDXGIFactory2 == nullptr)
            return false;

        DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(s_dxgiHandle, "DXGIGetDebugInterface1");

        static HMODULE s_d3d12Handle = LoadLibraryW(L"d3d12.dll");
        if (s_d3d12Handle == nullptr)
            return false;

        D3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(s_d3d12Handle, "D3D12CreateDevice");
        if (D3D12CreateDevice == nullptr)
            return false;

        D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(s_d3d12Handle, "D3D12GetDebugInterface");
        D3D12SerializeRootSignature = (PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)GetProcAddress(s_d3d12Handle, "D3D12SerializeRootSignature");
        D3D12CreateRootSignatureDeserializer = (PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(s_d3d12Handle, "D3D12CreateRootSignatureDeserializer");
        D3D12SerializeVersionedRootSignature = (PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE)GetProcAddress(s_d3d12Handle, "D3D12SerializeVersionedRootSignature");
        D3D12CreateVersionedRootSignatureDeserializer = (PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(s_d3d12Handle, "D3D12CreateVersionedRootSignatureDeserializer");
#endif

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

        available = true;
        return available;
    }

    D3D12GraphicsProvider::D3D12GraphicsProvider(GraphicsProviderFlags flags)
    {
#if defined(_DEBUG)
        // Enable the debug layer (requires the Graphics Tools "optional feature").
        //
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        if (any(flags & GraphicsProviderFlags::Validation) ||
            any(flags & GraphicsProviderFlags::GPUBasedValidation))
        {
            ID3D12Debug* d3d12debug = nullptr;
            ID3D12Debug1* d3d12debug1 = nullptr;

            if (SUCCEEDED(D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&d3d12debug)))
            {
                d3d12debug->EnableDebugLayer();

                if (SUCCEEDED(d3d12debug->QueryInterface(__uuidof(ID3D12Debug1), (void**)&d3d12debug1)))
                {
                    if (any(flags & GraphicsProviderFlags::GPUBasedValidation))
                    {
                        d3d12debug1->SetEnableGPUBasedValidation(TRUE);
                        d3d12debug1->SetEnableSynchronizedCommandQueueValidation(TRUE);
                    }
                    else
                    {
                        d3d12debug1->SetEnableGPUBasedValidation(FALSE);
                    }
                    
                }
            }
            else
            {
                OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
            }

            SafeRelease(d3d12debug1);
            SafeRelease(d3d12debug);

            IDXGIInfoQueue* dxgiInfoQueue = nullptr;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, __uuidof(IDXGIInfoQueue), (void**)&dxgiInfoQueue)))
            {
                dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

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
#endif
        ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));

        BOOL allowTearing = FALSE;
        ComPtr<IDXGIFactory5> dxgiFactory5;
        HRESULT hr = dxgiFactory.As(&dxgiFactory5);
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
    }

    D3D12GraphicsProvider::~D3D12GraphicsProvider()
    {
        dxgiFactory.Reset();

#ifdef _DEBUG
        ComPtr<IDXGIDebug> dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
        {
            dxgiDebug->ReportLiveObjects(g_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        }
#endif
    }

    std::vector<std::unique_ptr<GraphicsAdapter>> D3D12GraphicsProvider::EnumerateGraphicsAdapters()
    {
        std::vector<std::unique_ptr<GraphicsAdapter>> adapters;

        UINT index = 0;

        ComPtr<ID3D12Device> tempDevice = nullptr;
        ComPtr<IDXGIAdapter1> dxgiAdapter = nullptr;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        ComPtr<IDXGIFactory6> dxgiFactory6;
        HRESULT hr = dxgiFactory.As(&dxgiFactory6);
        if (SUCCEEDED(hr))
        {
            while (dxgiFactory6->EnumAdapterByGpuPreference(index++, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(dxgiAdapter.ReleaseAndGetAddressOf())) != DXGI_ERROR_NOT_FOUND)
            {
                std::unique_ptr<D3D12GraphicsAdapter> adapter = std::make_unique<D3D12GraphicsAdapter>(this, std::move(dxgiAdapter));
                if (!adapter->Initialize()) {
                    continue;
                }

                adapters.push_back(std::move(adapter));
            }
        }
#endif

        if (adapters.empty())
        {
            index = 0;
            while (dxgiFactory->EnumAdapters1(index++, dxgiAdapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND)
            {
                std::unique_ptr<D3D12GraphicsAdapter> adapter = std::make_unique<D3D12GraphicsAdapter>(this, std::move(dxgiAdapter));
                if (!adapter->Initialize()) {
                    continue;
                }

                adapters.push_back(std::move(adapter));
            }
        }

#if !defined(NDEBUG)
        if (adapters.empty())
        {
            // Try WARP12 instead
            if (FAILED(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(dxgiAdapter.ReleaseAndGetAddressOf()))))
            {
                ALIMER_LOGERROR("WARP12 not available. Enable the 'Graphics Tools' optional feature");
            }
        }
#endif

        return adapters;
    }
}
