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

#include "D3D11GPUProvider.h"
#include "D3D11GPUAdapter.h"
#include "D3D11GPUDevice.h"

namespace alimer
{
    bool D3D11GPUProvider::IsAvailable()
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

        CreateDXGIFactory1 = (PFN_CREATE_DXGI_FACTORY)GetProcAddress(s_dxgiHandle, "CreateDXGIFactory1");
        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(s_dxgiHandle, "CreateDXGIFactory2");
        if (CreateDXGIFactory2 == nullptr)
        {
            if (CreateDXGIFactory1)
            {
                return false;
            }
        }

        DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(s_dxgiHandle, "DXGIGetDebugInterface1");
        if (DXGIGetDebugInterface1 == nullptr)
        {
            DXGIGetDebugInterface = (PFN_GET_DXGI_DEBUG_INTERFACE)GetProcAddress(s_dxgiHandle, "DXGIGetDebugInterface");
            if (DXGIGetDebugInterface == nullptr)
            {
                return false;
            }
        }

        static HMODULE s_d3d11Handle = LoadLibraryW(L"d3d11.dll");
        if (s_d3d11Handle == nullptr)
            return false;

        D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(s_d3d11Handle, "D3D11CreateDevice");
        if (D3D11CreateDevice == nullptr)
            return false;
#endif

        // Create temp factory and detect adapter support
        {
            ComPtr<IDXGIFactory1> dxgiFactory;
            if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(dxgiFactory.GetAddressOf()))))
            {
                return false;
            }
        }

        available = true;
        return available;
    }

    D3D11GPUProvider::D3D11GPUProvider(bool validation)
        : _validation(validation)
    {
        ALIMER_ASSERT(IsAvailable());

#if defined(_DEBUG) && (_WIN32_WINNT >= 0x0603 /*_WIN32_WINNT_WINBLUE*/)
        bool debugDXGI = false;
        {
            ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
            {
                debugDXGI = true;

                ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));

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
            }
        }

        _validation = debugDXGI;

        if (!debugDXGI)
#endif

        {
            ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));
        }

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

    D3D11GPUProvider::~D3D11GPUProvider()
    {
        dxgiFactory.Reset();

        ReportLiveObjects();
    }

    void D3D11GPUProvider::ReportLiveObjects()
    {
#ifdef _DEBUG
        ComPtr<IDXGIDebug> dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
        {
            dxgiDebug->ReportLiveObjects(g_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        }
#endif
    }

    RefPtr<GPUDevice> D3D11GPUProvider::CreateDevice(GPUPowerPreference powerPreference)
    {
        ComPtr<IDXGIAdapter1> dxgiAdapter;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        ComPtr<IDXGIFactory6> dxgiFactory6;
        HRESULT hr = dxgiFactory.As(&dxgiFactory6);
        if (SUCCEEDED(hr))
        {
            DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
            if (powerPreference == GPUPowerPreference::LowPower)
            {
                gpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
            }

            UINT adapterIndex = 0;
            while (dxgiFactory6->EnumAdapterByGpuPreference(adapterIndex++, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(dxgiAdapter.ReleaseAndGetAddressOf())) != DXGI_ERROR_NOT_FOUND)
            {
                DXGI_ADAPTER_DESC1 desc;
                ThrowIfFailed(dxgiAdapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    continue;
                }

#ifdef _DEBUG
                wchar_t buff[256] = {};
                swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
                OutputDebugStringW(buff);
#endif
                break;
            }
        }
#endif
        if (!dxgiAdapter)
        {
            UINT adapterIndex = 0;
            while (dxgiFactory6->EnumAdapters1(adapterIndex++, dxgiAdapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND)
            {
                DXGI_ADAPTER_DESC1 desc;
                ThrowIfFailed(dxgiAdapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    continue;
                }

#ifdef _DEBUG
                wchar_t buff[256] = {};
                swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
                OutputDebugStringW(buff);
#endif
                break;
            }
        }

        if (!dxgiAdapter)
        {
            throw std::exception("No Direct3D 12 device found");
        }

        auto adapter = new D3D11GPUAdapter(dxgiAdapter);
        return MakeRefPtr<D3D11GPUDevice>(this, adapter);
    }
}

