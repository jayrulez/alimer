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

#include "D3D12GraphicsDevice.h"
//#include "D3D12Texture.h"
#include "D3D12Buffer.h"
//#include "D3D12CommandContext.h"

namespace Alimer
{
    bool D3D12GraphicsDevice::IsAvailable()
    {
        static bool available = false;
        static bool available_initialized = false;

        if (available_initialized) {
            return available;
        }

        available_initialized = true;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
        static HMODULE dxgiDLL = LoadLibraryA("dxgi.dll");
        if (!dxgiDLL) {
            return false;
        }

        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(dxgiDLL, "CreateDXGIFactory2");
        if (!CreateDXGIFactory2)
        {
            return false;
        }
        DXGIGetDebugInterface1 = (PFN_DXGI_GET_DEBUG_INTERFACE1)GetProcAddress(dxgiDLL, "DXGIGetDebugInterface1");

        static HMODULE d3d12DLL = LoadLibraryA("d3d12.dll");
        if (!d3d12DLL) {
            return false;
        }

        D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(d3d12DLL, "D3D12GetDebugInterface");
        D3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(d3d12DLL, "D3D12CreateDevice");
        if (!D3D12CreateDevice) {
            return false;
        }
#endif

        available = true;
        return true;
    }

    D3D12GraphicsDevice::D3D12GraphicsDevice(const GraphicsDeviceDescription& desc)
        : dxgiFactoryFlags(0)
        , dxgiFactoryCaps(DXGIFactoryCaps::FlipPresent | DXGIFactoryCaps::HDR)
        , d3dMinFeatureLevel(D3D_FEATURE_LEVEL_11_0)
    {
        ALIMER_VERIFY(IsAvailable());

        // Enable the debug layer (requires the Graphics Tools "optional feature").
        //
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        if (any(desc.flags & GraphicsDeviceFlags::DebugRuntime) || any(desc.flags & GraphicsDeviceFlags::GPUBasedValidation))
        {
            ComPtr<ID3D12Debug> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
            {
                debugController->EnableDebugLayer();

                ComPtr<ID3D12Debug1> debugController1;
                HRESULT hr = debugController.As(&debugController1);
                if (SUCCEEDED(hr))
                {
                    debugController1->SetEnableGPUBasedValidation(any(desc.flags & GraphicsDeviceFlags::GPUBasedValidation));
                }
            }
            else
            {
                OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
            }


#if defined(_DEBUG)
            ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
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
            }
#endif
        }

        ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));

        // Determines whether tearing support is available for fullscreen borderless windows.
        {
            BOOL allowTearing = FALSE;

            ComPtr<IDXGIFactory5> factory5;
            HRESULT hr = dxgiFactory.As(&factory5);
            if (SUCCEEDED(hr))
            {
                hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
            }

            if (FAILED(hr) || !allowTearing)
            {
#ifdef _DEBUG
                OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
            }
            else
            {
                dxgiFactoryCaps |= DXGIFactoryCaps::Tearing;
            }
        }

        // Get adapter and create device
        {
            ComPtr<IDXGIAdapter1> adapter;
            GetAdapter(desc.adapterPreference, adapter.GetAddressOf());

            // Create the DX12 API device object.
            HRESULT hr = D3D12CreateDevice(adapter.Get(), d3dMinFeatureLevel, IID_PPV_ARGS(&d3dDevice));
            ThrowIfFailed(hr);
            d3dDevice->SetName(L"D3D12GraphicsDevice");

#ifndef NDEBUG
            // Configure debug device (if active).
            ID3D12InfoQueue* d3dInfoQueue;
            if (SUCCEEDED(d3dDevice->QueryInterface(&d3dInfoQueue)))
            {
#ifdef _DEBUG
                d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
                d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif
                D3D12_MESSAGE_ID hide[] =
                {
                    D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
                    D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
                    D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                    D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
                    D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE
                };
                D3D12_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs = _countof(hide);
                filter.DenyList.pIDList = hide;
                d3dInfoQueue->AddStorageFilterEntries(&filter);
                d3dInfoQueue->Release();
            }
#endif

            // Create memory allocator.
            D3D12MA::ALLOCATOR_DESC desc = {};
            desc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
            desc.pDevice = d3dDevice;
            desc.pAdapter = adapter.Get();

            ThrowIfFailed(D3D12MA::CreateAllocator(&desc, &allocator));
            switch (allocator->GetD3D12Options().ResourceHeapTier)
            {
            case D3D12_RESOURCE_HEAP_TIER_1:
                LOGD("ResourceHeapTier = D3D12_RESOURCE_HEAP_TIER_1");
                break;
            case D3D12_RESOURCE_HEAP_TIER_2:
                LOGD("ResourceHeapTier = D3D12_RESOURCE_HEAP_TIER_2");
                break;
            default:
                break;
            }

            InitCapabilities(adapter.Get());
        }
    }

    D3D12GraphicsDevice::~D3D12GraphicsDevice()
    {
        Shutdown();
    }

    void D3D12GraphicsDevice::Shutdown()
    {
        WaitForGPU();

        // Allocator
        if (allocator != nullptr)
        {
            D3D12MA::Stats stats;
            allocator->CalculateStats(&stats);

            if (stats.Total.UsedBytes > 0) {
                LOGE("Total device memory leaked: %llu bytes.", stats.Total.UsedBytes);
            }

            allocator->Release(); allocator = nullptr;
        }

        ULONG refCount = d3dDevice->Release();
#if !defined(NDEBUG)
        if (refCount > 0)
        {
            LOGD("Direct3D12: There are %d unreleased references left on the device", refCount);

            ID3D12DebugDevice* debugDevice;
            if (SUCCEEDED(d3dDevice->QueryInterface(&debugDevice)))
            {
                debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_SUMMARY | D3D12_RLDO_IGNORE_INTERNAL);
                debugDevice->Release();
            }
        }
#else
        (void)refCount; // avoid warning
#endif
    }

    void D3D12GraphicsDevice::GetAdapter(GraphicsAdapterPreference adapterPreference, IDXGIAdapter1** ppAdapter)
    {
        *ppAdapter = nullptr;

        ComPtr<IDXGIAdapter1> adapter;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        ComPtr<IDXGIFactory6> factory6;
        HRESULT hr = dxgiFactory.As(&factory6);
        if (SUCCEEDED(hr))
        {
            DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
            if (adapterPreference == GraphicsAdapterPreference::LowPower) {
                gpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
            }

            for (UINT adapterIndex = 0;
                SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                    adapterIndex,
                    gpuPreference,
                    IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf())));
                adapterIndex++)
            {
                DXGI_ADAPTER_DESC1 desc;
                ThrowIfFailed(adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    continue;
                }

                // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
                if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), d3dMinFeatureLevel, _uuidof(ID3D12Device), nullptr)))
                {
#ifdef _DEBUG
                    wchar_t buff[256] = {};
                    swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
                    OutputDebugStringW(buff);
#endif
                    break;
                }
            }
        }
#endif
        if (!adapter)
        {
            for (UINT adapterIndex = 0; SUCCEEDED(dxgiFactory->EnumAdapters1(adapterIndex, adapter.ReleaseAndGetAddressOf())); ++adapterIndex)
            {
                DXGI_ADAPTER_DESC1 desc;
                ThrowIfFailed(adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    continue;
                }

                // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
                if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), d3dMinFeatureLevel, _uuidof(ID3D12Device), nullptr)))
                {
#ifdef _DEBUG
                    wchar_t buff[256] = {};
                    swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
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
            if (FAILED(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()))))
            {
                LOGE("WARP12 not available. Enable the 'Graphics Tools' optional feature");
                ALIMER_FORCE_CRASH();
            }

            OutputDebugStringA("Direct3D Adapter - WARP12\n");
        }
#endif

        if (!adapter)
        {
            LOGE("No Direct3D 12 device found");
            ALIMER_FORCE_CRASH();
        }

        *ppAdapter = adapter.Detach();
    }

    void D3D12GraphicsDevice::HandleDeviceLost()
    {

    }


    void D3D12GraphicsDevice::InitCapabilities(IDXGIAdapter1* adapter)
    {

    }

    void D3D12GraphicsDevice::WaitForGPU()
    {

    }

    uint64 D3D12GraphicsDevice::Frame()
    {
        return frameCount;
    }
}
