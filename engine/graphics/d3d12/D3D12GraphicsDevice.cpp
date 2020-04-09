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
#include "D3D12CommandContext.h"
#include "D3D12SwapChain.h"
#include "D3D12Texture.h"
#include "D3D12GraphicsBuffer.h"
#include "core/String.h"
#include "D3D12MemAlloc.h"

namespace alimer
{
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

    D3D12GraphicsDevice::D3D12GraphicsDevice(GraphicsSurface* surface_, const Desc& desc_)
        : GraphicsDevice(surface_, desc_)
        , graphicsQueue(this, D3D12_COMMAND_LIST_TYPE_DIRECT)
        , computeQueue(this, D3D12_COMMAND_LIST_TYPE_COMPUTE)
        , copyQueue(this, D3D12_COMMAND_LIST_TYPE_COPY)
        , frameFence(this)
        , RTVDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false)
        , DSVDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false)
    {
    }

    D3D12GraphicsDevice::~D3D12GraphicsDevice()
    {
        WaitForIdle();
        Shutdown();
    }

    void D3D12GraphicsDevice::Shutdown()
    {
        ALIMER_ASSERT(currentCPUFrame == currentGPUFrame);
        shuttingDown = true;

        for (uint32_t i = 0; i < renderLatency; ++i)
        {
            ProcessDeferredReleases(i);
        }

        swapChain.reset();
        mainContext.reset();

        // Destroy descripto heaps.
        {
            RTVDescriptorHeap.Shutdown();
            DSVDescriptorHeap.Shutdown();
            //SRVDescriptorHeap.Shutdown();
            //UAVDescriptorHeap.Shutdown();
        }

        frameFence.Shutdown();

        copyQueue.Destroy();
        computeQueue.Destroy();
        graphicsQueue.Destroy();

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
        IDXGIDebug* dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
        {
            dxgiDebug->ReportLiveObjects(g_DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            dxgiDebug->Release();
        }
#endif
    }

    void D3D12GraphicsDevice::ProcessDeferredReleases(uint64_t frameIndex)
    {
        for (size_t i = 0, count = deferredReleases[frameIndex].size(); i < count; ++i)
        {
            deferredReleases[frameIndex][i]->Release();
        }

        deferredReleases[frameIndex].clear();
    }

    void D3D12GraphicsDevice::DeferredRelease_(IUnknown* resource, bool forceDeferred)
    {
        if (resource == nullptr)
            return;

        if ((currentCPUFrame == currentGPUFrame && forceDeferred == false) ||
            shuttingDown || d3dDevice == nullptr)
        {
            resource->Release();
            return;
        }

        deferredReleases[currentFrameIndex].push_back(resource);
    }

    bool D3D12GraphicsDevice::Init()
    {
#if defined(_DEBUG)
        // Enable the debug layer (requires the Graphics Tools "optional feature").
        //
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        if (any(desc.flags & GraphicsProviderFlags::Validation) ||
            any(desc.flags & GraphicsProviderFlags::GPUBasedValidation))
        {
            ComPtr<ID3D12Debug> d3d12debug;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(d3d12debug.GetAddressOf()))))
            {
                d3d12debug->EnableDebugLayer();

                ComPtr<ID3D12Debug1> d3d12debug1 = nullptr;
                if (SUCCEEDED(d3d12debug.As(&d3d12debug1)))
                {
                    if (any(desc.flags & GraphicsProviderFlags::GPUBasedValidation))
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

        // Check tearing support.
        {
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

        ComPtr<IDXGIAdapter1> adapter;
        if (!GetAdapter(adapter.GetAddressOf())) {
            return false;
        }

        // Create the DX12 API device object.
        ThrowIfFailed(D3D12CreateDevice(adapter.Get(), minFeatureLevel, IID_PPV_ARGS(&d3dDevice)));

        // Init caps
        InitCapabilities(adapter.Get());

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

        // Create memory allocator
        {
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

        // Create command queue's and default context.
        {
            graphicsQueue.Create();
            computeQueue.Create();
            copyQueue.Create();
        }

        // Create the main swap chain
        swapChain.reset(new D3D12SwapChain(this, dxgiFactory.Get(), surface, renderLatency));

        // Create the main context.
        mainContext.reset(new D3D12GraphicsContext(this, D3D12_COMMAND_LIST_TYPE_DIRECT, renderLatency));

        // Create frame fence.
        frameFence.Init(0);

        // Init descriptor heaps
        RTVDescriptorHeap.Init(256, 0);
        DSVDescriptorHeap.Init(256, 0);

        return true;
    }

    bool D3D12GraphicsDevice::GetAdapter(IDXGIAdapter1** ppAdapter)
    {
        *ppAdapter = nullptr;

        ComPtr<IDXGIAdapter1> adapter;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        ComPtr<IDXGIFactory6> factory6;
        HRESULT hr = dxgiFactory.As(&factory6);
        if (SUCCEEDED(hr))
        {
            DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
            if (desc.powerPreference == GPUPowerPreference::LowPower)
            {
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
                if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), minFeatureLevel, _uuidof(ID3D12Device), nullptr)))
                {
                    break;
                }
            }
        }
#endif
        if (!adapter)
        {
            for (UINT adapterIndex = 0;
                SUCCEEDED(dxgiFactory->EnumAdapters1(
                    adapterIndex,
                    adapter.ReleaseAndGetAddressOf()));
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
                if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), minFeatureLevel, _uuidof(ID3D12Device), nullptr)))
                {
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
                ALIMER_LOGERROR("WARP12 not available. Enable the 'Graphics Tools' optional feature");
                return false;
            }

            OutputDebugStringA("Direct3D Adapter - WARP12\n");
        }
#endif

        if (!adapter)
        {
            ALIMER_LOGERROR("No Direct3D 12 device found");
            return false;
        }

        *ppAdapter = adapter.Detach();
        return true;
    }

    void D3D12GraphicsDevice::InitCapabilities(IDXGIAdapter1* adapter)
    {
        HRESULT hr = S_OK;

        DXGI_ADAPTER_DESC1 desc;
        ThrowIfFailed(adapter->GetDesc1(&desc));

#ifdef _DEBUG
        wchar_t buff[256] = {};
        swprintf_s(buff, L"Direct3D Adapter VID:%04X, PID:%04X - %ls\n", desc.VendorId, desc.DeviceId, desc.Description);
        OutputDebugStringW(buff);
#endif

        caps.backendType = BackendType::Direct3D12;
        caps.vendorId = desc.VendorId;
        caps.deviceId = desc.DeviceId;

        std::wstring deviceName(desc.Description);
        caps.adapterName = alimer::ToUtf8(deviceName);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            caps.adapterType = GraphicsAdapterType::CPU;
        }
        else
        {
            D3D12_FEATURE_DATA_ARCHITECTURE arch = {};
            ThrowIfFailed(d3dDevice->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &arch, sizeof(arch)));

            caps.adapterType = arch.UMA ? GraphicsAdapterType::IntegratedGPU : GraphicsAdapterType::DiscreteGPU;
        }

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

        hr = d3dDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
        if (SUCCEEDED(hr))
        {
            featureLevel = featLevels.MaxSupportedFeatureLevel;
        }
        else
        {
            featureLevel = D3D_FEATURE_LEVEL_11_0;
        }
    }

    void D3D12GraphicsDevice::WaitForIdle()
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


    bool D3D12GraphicsDevice::BeginFrame()
    {
        /*mainContext->Begin();*/

        return true;
    }

    void D3D12GraphicsDevice::PresentFrame()
    {
        //ID3D12CommandList* commandLists[] = { mainContext->GetCommandList() };
        //graphicsQueue.GetHandle()->ExecuteCommandLists(_countof(commandLists), commandLists);

        // Present the frame.
        swapChain->Present();

        ++currentCPUFrame;

        // Signal the fence with the current frame number, so that we can check back on it
        frameFence.Signal(graphicsQueue.GetHandle(), currentCPUFrame);

        // Wait for the GPU to catch up before we stomp an executing command buffer
        const uint64_t gpuLag = currentCPUFrame - currentGPUFrame;
        ALIMER_ASSERT(gpuLag <= renderLatency);
        if (gpuLag >= renderLatency)
        {
            // Make sure that the previous frame is finished
            frameFence.Wait(currentGPUFrame + 1);
            ++currentGPUFrame;
        }

        currentFrameIndex = currentCPUFrame % renderLatency;

        // Release any pending deferred releases
        ProcessDeferredReleases(currentFrameIndex);
    }

    GraphicsContext* D3D12GraphicsDevice::RequestContext(bool compute)
    {
        return mainContext.get();
    }
}
