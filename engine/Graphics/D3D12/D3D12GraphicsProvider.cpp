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
#include "D3D12GraphicsDevice.h"

namespace alimer
{
    D3D12GraphicsProvider::D3D12GraphicsProvider(bool validation)
        : GraphicsProvider(BackendType::Direct3D12, validation)
        , dxgiFactoryFlags(0)
        , functions(new D3D12PlatformFunctions())
    {

        // Enable the debug layer (requires the Graphics Tools "optional feature").
        //
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        if (validation)
        {
            ID3D12Debug* d3d12Debug;
            if (SUCCEEDED(functions->d3d12GetDebugInterface(IID_PPV_ARGS(&d3d12Debug))))
            {
                d3d12Debug->EnableDebugLayer();

                ID3D12Debug1* d3d12Debug1;
                if (SUCCEEDED(d3d12Debug->QueryInterface(&d3d12Debug1)))
                {
                    d3d12Debug1->SetEnableGPUBasedValidation(TRUE);
                    //d3d12Debug1->SetEnableSynchronizedCommandQueueValidation(TRUE);
                    d3d12Debug1->Release();
                }

                d3d12Debug->Release();
            }
            else
            {
                OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
            }

#if defined(_DEBUG)
            IDXGIInfoQueue* dxgiInfoQueue;
            if (SUCCEEDED(functions->dxgiGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
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
#endif
        }

        VHR(functions->createDxgiFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

        // Check tearing support.
        {
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
            SAFE_RELEASE(dxgiFactory5);
        }
    }

    D3D12GraphicsProvider::~D3D12GraphicsProvider()
    {
        SAFE_RELEASE(dxgiFactory);

#ifdef _DEBUG
        IDXGIDebug1* dxgiDebug;
        if (SUCCEEDED(functions->dxgiGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
        {
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            dxgiDebug->Release();
        }
#endif

        SafeDelete(functions);
    }

    GraphicsDevice* D3D12GraphicsProvider::CreateDevice(const GraphicsDeviceDescriptor* descriptor)
    {
        IDXGIAdapter1* adapter = nullptr;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        if (descriptor->powerPreference != GPUPowerPreference::Default)
        {
            IDXGIFactory6* dxgiFactory6 = nullptr;
            HRESULT hr = dxgiFactory->QueryInterface(&dxgiFactory6);
            if (SUCCEEDED(hr))
            {
                DXGI_GPU_PREFERENCE dxgiGpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
                if (descriptor->powerPreference == GPUPowerPreference::LowPower) {
                    dxgiGpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
                }

                for (UINT index = 0; SUCCEEDED(dxgiFactory6->EnumAdapterByGpuPreference(index, dxgiGpuPreference, IID_PPV_ARGS(&adapter))); index++)
                {
                    DXGI_ADAPTER_DESC1 desc;
                    VHR(adapter->GetDesc1(&desc));

                    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                    {
                        // Don't select the Basic Render Driver adapter.
                        adapter->Release();

                        continue;
                    }

                    // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
                    if (SUCCEEDED(functions->d3d12CreateDevice(adapter, minFeatureLevel, _uuidof(ID3D12Device), nullptr)))
                    {
#ifdef _DEBUG
                        wchar_t buff[256] = {};
                        swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", index, desc.VendorId, desc.DeviceId, desc.Description);
                        OutputDebugStringW(buff);
#endif
                        break;
                    }
                }
            }

            SAFE_RELEASE(dxgiFactory6);
        }
#endif

        if (!adapter)
        {
            for (UINT index = 0;
                SUCCEEDED(dxgiFactory->EnumAdapters1(index, &adapter));
                ++index)
            {
                DXGI_ADAPTER_DESC1 desc;
                VHR(adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    adapter->Release();

                    continue;
                }

                // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
                if (SUCCEEDED(functions->d3d12CreateDevice(adapter, minFeatureLevel, _uuidof(ID3D12Device), nullptr)))
                {
#ifdef _DEBUG
                    wchar_t buff[256] = {};
                    swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", index, desc.VendorId, desc.DeviceId, desc.Description);
                    OutputDebugStringW(buff);
#endif
                    break;
                }
            }
        }

#if !defined(NDEBUG)
        if (!adapter)
        {
            // Try WARP12 instead
            if (FAILED(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter))))
            {
                LOG_ERROR("WARP12 not available. Enable the 'Graphics Tools' optional feature");
                ALIMER_FORCE_CRASH();
            }

            OutputDebugStringA("Direct3D Adapter - WARP12\n");
        }
#endif

        if (!adapter)
        {
            LOG_ERROR("No Direct3D 12 device found");
            ALIMER_FORCE_CRASH();
        }

        return new D3D12GraphicsDevice(this, adapter);
    }
}

