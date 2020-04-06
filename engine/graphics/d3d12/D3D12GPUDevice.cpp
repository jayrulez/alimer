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

#include "D3D12GPUDevice.h"
#include "D3D12CommandQueue.h"
//#include "D3D12CommandContext.h"
#include "D3D12SwapChain.h"
#include "D3D12Texture.h"
#include "D3D12GPUBuffer.h"
#include "core/String.h"
#include "D3D12MemAlloc.h"

namespace alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
    // D3D12 functions.
    PFN_D3D12_CREATE_DEVICE D3D12CreateDevice;
    PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface;
    PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignature;
    PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER D3D12CreateRootSignatureDeserializer;
    PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE D3D12SerializeVersionedRootSignature;
    PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER D3D12CreateVersionedRootSignatureDeserializer;
#endif

    bool D3D12GPUDevice::IsAvailable()
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

            if (SUCCEEDED(D3D12CreateDevice(nullptr, d3dMinFeatureLevel, _uuidof(ID3D12Device), nullptr)))
            {
                available = true;
            }
        }

        available = true;
        return available;
    }

    D3D12GPUDevice::D3D12GPUDevice(Window* window_, const Desc& desc_)
        : GPUDevice(window_, desc_)
        , frameFence(this)
    {
    }

    D3D12GPUDevice::~D3D12GPUDevice()
    {
        WaitIdle();
        BackendShutdown();
    }

    bool D3D12GPUDevice::GetAdapter(IDXGIAdapter1** ppAdapter)
    {
        *ppAdapter = nullptr;

        IDXGIAdapter1* adapter = nullptr;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        IDXGIFactory6* dxgiFactory6;
        HRESULT hr = dxgiFactory->QueryInterface(IID_PPV_ARGS(&dxgiFactory6));
        if (SUCCEEDED(hr))
        {
            for (UINT adapterIndex = 0;
                SUCCEEDED(dxgiFactory6->EnumAdapterByGpuPreference(
                    adapterIndex,
                    DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                    IID_PPV_ARGS(&adapter)));
                adapterIndex++)
            {
                DXGI_ADAPTER_DESC1 desc;
                ThrowIfFailed(adapter->GetDesc1(&desc));

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    adapter->Release();

                    // Don't select the Basic Render Driver adapter.
                    continue;
                }

                // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
                if (SUCCEEDED(D3D12CreateDevice(adapter, d3dMinFeatureLevel, _uuidof(ID3D12Device), nullptr)))
                {
                    break;
                }
            }
        }
        SafeRelease(dxgiFactory6);
#endif

        if (!adapter)
        {
            for (UINT adapterIndex = 0;
                SUCCEEDED(dxgiFactory->EnumAdapters1(adapterIndex, &adapter));
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
                if (SUCCEEDED(D3D12CreateDevice(adapter, d3dMinFeatureLevel, _uuidof(ID3D12Device), nullptr)))
                {
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

        *ppAdapter = adapter;
        return true;
    }

    bool D3D12GPUDevice::BackendInit()
    {

#if defined(_DEBUG)
        // Enable the debug layer (requires the Graphics Tools "optional feature").
        //
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        if (any(desc.flags & GPUDeviceFlags::Validation) ||
            any(desc.flags & GPUDeviceFlags::GPUBasedValidation))
        {
            ID3D12Debug* d3d12debug = nullptr;
            if (SUCCEEDED(D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&d3d12debug)))
            {
                d3d12debug->EnableDebugLayer();

                ID3D12Debug1* d3d12debug1;
                if (SUCCEEDED(d3d12debug->QueryInterface(__uuidof(ID3D12Debug1), (void**)&d3d12debug1)))
                {
                    if (any(desc.flags & GPUDeviceFlags::GPUBasedValidation))
                    {
                        d3d12debug1->SetEnableGPUBasedValidation(true);
                    }
                    else
                    {
                        d3d12debug1->SetEnableGPUBasedValidation(false);
                    }
                    d3d12debug1->Release();
                }
                d3d12debug->Release();
            }
            else
            {
                OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
            }

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
        IDXGIFactory5* dxgiFactory5 = nullptr;
        HRESULT hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory5), (void**)&dxgiFactory5);
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

        // Get adapter, create device and allocator.
        {
            IDXGIAdapter1* adapter;
            GetAdapter(&adapter);

            // Create the DX12 API device object.
            ThrowIfFailed(D3D12CreateDevice(
                adapter,
                d3dMinFeatureLevel,
                IID_PPV_ARGS(&d3dDevice)
            ));

            d3dDevice->SetName(L"AlimerDevice");
            InitCapabilities(adapter);

            // Create memory allocator
            D3D12MA::ALLOCATOR_DESC desc = {};
            desc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
            desc.pDevice = d3dDevice;
            desc.pAdapter = adapter;

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

            adapter->Release();
        }

        // Create command queue's and default context.
        {
            D3D12_COMMAND_QUEUE_DESC queueDesc = {};
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
            queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            queueDesc.NodeMask = 0;
            ThrowIfFailed(d3dDevice->CreateCommandQueue(&queueDesc, __uuidof(ID3D12CommandQueue), (void**)&graphicsQueue));
            graphicsQueue->SetName(L"Graphics Command Queue");

            //computeQueue = new D3D12CommandQueue(this, CommandQueueType::Compute);
            //copyQueue = new D3D12CommandQueue(this, CommandQueueType::Copy);
            //mainContext.reset(new D3D12GraphicsContext(this, CommandQueueType::Graphics));
        }

        // Create frame fence.
        frameFence.Init(0);

        return true;
    }

    void D3D12GPUDevice::BackendShutdown()
    {
        //mainContext.reset();

        frameFence.Shutdown();

        SafeRelease(graphicsQueue);
        //SafeDelete(computeQueue);
        //SafeDelete(copyQueue);

        // Allocator
        D3D12MA::Stats stats;
        allocator->CalculateStats(&stats);

        if (stats.Total.UsedBytes > 0) {
            ALIMER_LOGE("Total device memory leaked: %llu bytes.", stats.Total.UsedBytes);
        }

        SafeRelease(allocator);

#if !defined(NDEBUG)
        ULONG refCount = d3dDevice->Release();
        if (refCount > 0)
        {
            ALIMER_LOGD("Direct3D12: There are %d unreleased references left on the device", refCount);

            SharedPtr<ID3D12DebugDevice> debugDevice;
            if (SUCCEEDED(d3dDevice->QueryInterface(IID_PPV_ARGS(debugDevice.GetAddressOf()))))
            {
                debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_SUMMARY | D3D12_RLDO_IGNORE_INTERNAL);
            }
        }
#else
        SafeRelease(d3dDevice);
#endif

        dxgiFactory.Reset();

#ifdef _DEBUG
        {
            SharedPtr<IDXGIDebug1> dxgiDebug;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiDebug.GetAddressOf()))))
            {
                dxgiDebug->ReportLiveObjects(g_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            }
        }
#endif
    }

    void D3D12GPUDevice::DeferredRelease_(IUnknown* resource, bool forceDeferred)
    {

    }

    void D3D12GPUDevice::InitCapabilities(IDXGIAdapter1* adapter)
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

        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

#ifdef _DEBUG
        wchar_t buff[256] = {};
        swprintf_s(buff, L"Direct3D Adapter: VID:%04X, PID:%04X - %ls\n", desc.VendorId, desc.DeviceId, desc.Description);
        OutputDebugStringW(buff);
#endif
        std::wstring deviceName(desc.Description);

        // Initialize caps and info.
        info.backend = GPUBackend::Direct3D12;
        info.backendName = "Direct3D12 - Level " + D3DFeatureLevelToVersion(d3dFeatureLevel);
        info.deviceName = alimer::ToUtf8(deviceName);
        info.vendorName = GetVendorByID(desc.VendorId);
        info.vendorId = desc.VendorId;
    }

    void D3D12GPUDevice::WaitIdle()
    {
        // Wait for the GPU to fully catch up with the CPU
        ALIMER_ASSERT(currentCPUFrame >= currentGPUFrame);
        if (currentCPUFrame > currentGPUFrame)
        {
            frameFence.Wait(currentCPUFrame);
            currentGPUFrame = currentCPUFrame;
        }

        /*computeQueue->WaitForIdle();
        copyQueue->WaitForIdle();*/
    }

    /*SharedPtr<SwapChain> D3D12GPUDevice::CreateSwapChain(const SwapChainDescriptor* descriptor)
    {
        ALIMER_ASSERT(descriptor);
        return new D3D12SwapChain(this, descriptor);
    }

    SharedPtr<Texture> D3D12GPUDevice::CreateTexture()
    {
        return new D3D12Texture(this);
    }

    GPUBuffer* D3D12GPUDevice::CreateBufferCore(const BufferDescriptor* descriptor, const void* initialData)
    {
        return new D3D12GPUBuffer(this, descriptor, initialData);
    }*/

    void D3D12GPUDevice::Commit()
    {
        /*graphicsQueue->GetHandle()->Signal(frameFence, ++numFrames);

        uint64_t GPUFrameCount = frameFence->GetCompletedValue();

        if ((numFrames - GPUFrameCount) >= 2)
        {
            frameFence->SetEventOnCompletion(GPUFrameCount + 1, frameFenceEvent);
            WaitForSingleObject(frameFenceEvent, INFINITE);
        }*/

        if (!dxgiFactory->IsCurrent())
        {
            // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
            ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));
        }
    }
}
