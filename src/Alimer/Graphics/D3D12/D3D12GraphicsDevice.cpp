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
#include "D3D12CommandQueue.h"
#include "D3D12SwapChain.h"

namespace Alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
    // DXGI functions
    PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2;
    PFN_GET_DXGI_DEBUG_INTERFACE1 DXGIGetDebugInterface1;
    // D3D12 functions.
    PFN_D3D12_CREATE_DEVICE D3D12CreateDevice;
    PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface;
    PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignature;
    PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER D3D12CreateRootSignatureDeserializer;
    PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE D3D12SerializeVersionedRootSignature;
    PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER D3D12CreateVersionedRootSignatureDeserializer;
#endif

#if defined(_DEBUG)
    // Declare debug guids to avoid linking with "dxguid.lib"
    static constexpr GUID g_DXGI_DEBUG_ALL = { 0xe48ae283, 0xda80, 0x490b, {0x87, 0xe6, 0x43, 0xe9, 0xa9, 0xcf, 0xda, 0x8} };
    static constexpr GUID g_DXGI_DEBUG_DXGI = { 0x25cddaa4, 0xb1c6, 0x47e1, {0xac, 0x3e, 0x98, 0x87, 0x5b, 0x5a, 0x2e, 0x2a} };
#endif

    bool D3D12GraphicsDevice::IsAvailable()
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
            ComPtr<IDXGIFactory4> tempFactory;
            HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(tempFactory.GetAddressOf()));
            if (FAILED(hr))
            {
                return false;
            }

            ComPtr<IDXGIAdapter1> adapter;
            available = GetAdapter(tempFactory, adapter.GetAddressOf());
        }

        available = true;
        return available;
    }

    D3D12GraphicsDevice::D3D12GraphicsDevice(const GraphicsDeviceDescriptor* descriptor)
        : GraphicsDevice(descriptor)
    {
        CreateDeviceResources();
    }

    D3D12GraphicsDevice::~D3D12GraphicsDevice()
    {
        WaitIdle();
        Destroy();
    }

    void D3D12GraphicsDevice::Destroy()
    {
        SafeDelete(graphicsQueue);
        SafeDelete(computeQueue);
        SafeDelete(copyQueue);

        // Allocator
        D3D12MA::Stats stats;
        allocator->CalculateStats(&stats);

        if (stats.Total.UsedBytes > 0) {
            ALIMER_LOGERRORF("Total device memory leaked: %llu bytes.", stats.Total.UsedBytes);
        }

        SafeRelease(allocator);

#if !defined(NDEBUG)
        ULONG refCount = d3dDevice->Release();
        if (refCount > 0)
        {
            ALIMER_LOGDEBUGF("Direct3D12: There are %d unreleased references left on the device", refCount);

            ID3D12DebugDevice* debugDevice;
            if (SUCCEEDED(d3dDevice->QueryInterface(IID_PPV_ARGS(&debugDevice))))
            {
                debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_SUMMARY | D3D12_RLDO_IGNORE_INTERNAL);
                debugDevice->Release();
            }
        }
#else
        SafeRelease(d3dDevice);
#endif

        dxgiFactory.Reset();

#ifdef _DEBUG
        {
            IDXGIDebug1* dxgiDebug;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
            {
                dxgiDebug->ReportLiveObjects(g_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
                dxgiDebug->Release();
            }
        }
#endif
    }

    bool D3D12GraphicsDevice::GetAdapter(ComPtr<IDXGIFactory4> factory4, IDXGIAdapter1** ppAdapter)
    {
        *ppAdapter = nullptr;

        ComPtr<IDXGIAdapter1> adapter;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        ComPtr<IDXGIFactory6> factory6;
        HRESULT hr = factory4.As(&factory6);
        if (SUCCEEDED(hr))
        {
            for (UINT adapterIndex = 0;
                SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                    adapterIndex,
                    DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
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
            for (UINT adapterIndex = 0;
                SUCCEEDED(factory4->EnumAdapters1(adapterIndex, adapter.ReleaseAndGetAddressOf()));
                ++adapterIndex)
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
            if (FAILED(factory4->EnumWarpAdapter(IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()))))
            {
                ALIMER_LOGERROR("WARP12 not available. Enable the 'Graphics Tools' optional feature");
            }
            else
            {
                OutputDebugStringA("Direct3D Adapter - WARP12\n");
            }
        }
#endif

        if (!adapter)
        {
            return false;
        }

        *ppAdapter = adapter.Detach();
        return true;
    }

    void D3D12GraphicsDevice::CreateDeviceResources()
    {
#if defined(_DEBUG)
        // Enable the debug layer (requires the Graphics Tools "optional feature").
        //
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        if (any(flags & GraphicsDeviceFlags::DebugRuntime)
            || any(flags & GraphicsDeviceFlags::GPUBasedValidation))
        {
            ComPtr<ID3D12Debug> d3d12debug;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(d3d12debug.GetAddressOf()))))
            {
                d3d12debug->EnableDebugLayer();

                ComPtr<ID3D12Debug1> d3d12debug1;
                if (SUCCEEDED(d3d12debug->QueryInterface(IID_PPV_ARGS(d3d12debug1.GetAddressOf()))))
                {
                    if (any(flags & GraphicsDeviceFlags::GPUBasedValidation))
                    {
                        d3d12debug1->SetEnableGPUBasedValidation(true);
                    }
                    else
                    {
                        d3d12debug1->SetEnableGPUBasedValidation(false);
                    }
                }
            }
            else
            {
                OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
            }

            ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
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
            }
        }
#endif

        ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));

        BOOL allowTearing = FALSE;
        ComPtr<IDXGIFactory5> factory5;
        HRESULT hr = dxgiFactory.As(&factory5);
        if (SUCCEEDED(hr))
        {
            hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
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

        // Get adapter, create device and allocator.
        {
            ComPtr<IDXGIAdapter1> adapter;
            GetAdapter(dxgiFactory, adapter.GetAddressOf());

            // Create the DX12 API device object.
            ThrowIfFailed(D3D12CreateDevice(
                adapter.Get(),
                d3dMinFeatureLevel,
                IID_PPV_ARGS(&d3dDevice)
            ));

            d3dDevice->SetName(L"AlimerDevice");
            InitCapabilities(adapter.Get());

            // Create memory allocator
            D3D12MA::ALLOCATOR_DESC desc = {};
            desc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
            desc.pDevice = d3dDevice;
            desc.pAdapter = adapter.Get();

            ThrowIfFailed(D3D12MA::CreateAllocator(&desc, &allocator));

            switch (allocator->GetD3D12Options().ResourceHeapTier)
            {
            case D3D12_RESOURCE_HEAP_TIER_1:
                ALIMER_LOGDEBUG("ResourceHeapTier = D3D12_RESOURCE_HEAP_TIER_1");
                break;
            case D3D12_RESOURCE_HEAP_TIER_2:
                ALIMER_LOGDEBUG("ResourceHeapTier = D3D12_RESOURCE_HEAP_TIER_2");
                break;
            default:
                break;
            }
        }

        // Create command queue's
        {
            graphicsQueue = new D3D12CommandQueue(this, CommandQueueType::Graphics);
            computeQueue = new D3D12CommandQueue(this, CommandQueueType::Compute);
            copyQueue = new D3D12CommandQueue(this, CommandQueueType::Copy);
        }
    }

    void D3D12GraphicsDevice::InitCapabilities(IDXGIAdapter1* adapter)
    {
        // Determine maximum supported feature level for this device
        static const D3D_FEATURE_LEVEL s_featureLevels[] =
        {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
        };

        D3D12_FEATURE_DATA_FEATURE_LEVELS featLevels =
        {
            _countof(s_featureLevels), s_featureLevels, D3D_FEATURE_LEVEL_11_0
        };

        HRESULT hr = d3dDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
        if (SUCCEEDED(hr))
        {
            d3dFeatureLevel = featLevels.MaxSupportedFeatureLevel;
        }
        else
        {
            d3dFeatureLevel = d3dMinFeatureLevel;
        }
    }

    void D3D12GraphicsDevice::WaitIdle()
    {
        graphicsQueue->WaitForIdle();
        computeQueue->WaitForIdle();
        copyQueue->WaitForIdle();
    }

    bool D3D12GraphicsDevice::BeginFrame()
    {
        return true;
    }

    void D3D12GraphicsDevice::EndFrame()
    {

    }


    SwapChain* D3D12GraphicsDevice::CreateSwapChainCore(void* nativeHandle, const SwapChainDescriptor* descriptor)
    {
        return new D3D12SwapChain(this, nativeHandle, descriptor);
    }

    D3D12CommandQueue* D3D12GraphicsDevice::GetQueue(CommandQueueType queueType) const
    {
        switch (queueType)
        {
        case CommandQueueType::Compute:
            return computeQueue;
        case CommandQueueType::Copy:
            return copyQueue;
        default:
            return graphicsQueue;
        }
    }

    ID3D12CommandQueue* D3D12GraphicsDevice::GetD3DCommandQueue(CommandQueueType queueType) const
    {
        switch (queueType)
        {
        case CommandQueueType::Compute:
            return computeQueue->GetHandle();
        case CommandQueueType::Copy:
            return copyQueue->GetHandle();
        default:
            return graphicsQueue->GetHandle();
        }
    }
}
