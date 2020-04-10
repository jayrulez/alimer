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
#include "D3D12GraphicsBuffer.h"
#include "core/String.h"
#include "D3D12MemAlloc.h"
#include "math/math.h"
#if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
#include <windows.ui.xaml.media.dxinterop.h>
#endif

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

    D3D12GraphicsDevice::D3D12GraphicsDevice(GraphicsProviderFlags flags, GPUPowerPreference powerPreference)
        : flags{ flags }
        , powerPreference{ powerPreference }
        , graphicsQueue(QueueType::Graphics)
        , computeQueue(QueueType::Compute)
        , copyQueue(QueueType::Copy)
        , RTVDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false)
        , DSVDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false)
    {
    }

    D3D12GraphicsDevice::~D3D12GraphicsDevice()
    {
        Shutdown();
    }

    void D3D12GraphicsDevice::Shutdown()
    {
        if (frameFence.cpuValue != frameFence.handle->GetCompletedValue())
        {
            WaitForIdle();
        }

        ALIMER_ASSERT(frameFence.cpuValue == frameFence.handle->GetCompletedValue());
        shuttingDown = true;

        ExecuteDeferredReleases();

        // Destroy descripton heaps.
        {
            RTVDescriptorHeap.Shutdown();
            DSVDescriptorHeap.Shutdown();
            //SRVDescriptorHeap.Shutdown();
            //UAVDescriptorHeap.Shutdown();
        }

        DestroyFence(&frameFence);

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

    void D3D12GraphicsDevice::ExecuteDeferredReleases()
    {
        uint64_t gpuValue = frameFence.handle->GetCompletedValue();
        while (deferredReleases.size() &&
            deferredReleases.front().frameIndex <= gpuValue)
        {
            deferredReleases.front().handle->Release();
            deferredReleases.pop();
        }
    }

    void D3D12GraphicsDevice::DeferredRelease_(IUnknown* resource, bool forceDeferred)
    {
        if (resource == nullptr)
            return;

        if (forceDeferred || shuttingDown || d3dDevice == nullptr)
        {
            resource->Release();
            return;
        }

        deferredReleases.push({ frameFence.cpuValue, resource });
    }

    bool D3D12GraphicsDevice::Init()
    {
#if defined(_DEBUG)
        // Enable the debug layer (requires the Graphics Tools "optional feature").
        //
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        if (any(flags & GraphicsProviderFlags::Validation) ||
            any(flags & GraphicsProviderFlags::GPUBasedValidation))
        {
            ComPtr<ID3D12Debug> d3d12debug;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(d3d12debug.GetAddressOf()))))
            {
                d3d12debug->EnableDebugLayer();

                ComPtr<ID3D12Debug1> d3d12debug1 = nullptr;
                if (SUCCEEDED(d3d12debug.As(&d3d12debug1)))
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
                D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
                D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
                D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE,
                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
                D3D12_MESSAGE_ID_COPY_DESCRIPTORS_INVALID_RANGES
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
            graphicsQueue.Create(d3dDevice);
            computeQueue.Create(d3dDevice);
            copyQueue.Create(d3dDevice);
        }

        // Create the main swap chain
        //swapChain.reset(new D3D12SwapChain(this, dxgiFactory.Get(), surface, renderLatency));

        // Create the main context.
        //mainContext.reset(new D3D12GraphicsContext(this, D3D12_COMMAND_LIST_TYPE_DIRECT, renderLatency));

        // Init descriptor heaps
        RTVDescriptorHeap.Init(256, 0);
        DSVDescriptorHeap.Init(256, 0);

        // Init pools
        {
            swapchains.init();
            textures.init();
            buffers.init();
        }

        // Create frame fence.
        CreateFence(&frameFence);

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
            if (powerPreference == GPUPowerPreference::LowPower)
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

        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

        // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if (FAILED(d3dDevice->CheckFeatureSupport(
            D3D12_FEATURE_ROOT_SIGNATURE,
            &featureData, sizeof(featureData))))
        {
            rootSignatureVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        // Features
        caps.features.independentBlend = true;
        caps.features.computeShader = true;
        caps.features.geometryShader = true;
        caps.features.tessellationShader = true;
        caps.features.logicOp = true;
        caps.features.multiViewport = true;
        caps.features.fullDrawIndexUint32 = true;
        caps.features.multiDrawIndirect = true;
        caps.features.fillModeNonSolid = true;
        caps.features.samplerAnisotropy = true;
        caps.features.textureCompressionBC = true;
        caps.features.textureCompressionPVRTC = false;
        caps.features.textureCompressionETC2 = false;
        caps.features.textureCompressionASTC = false;
        caps.features.texture1D = true;
        caps.features.texture3D = true;
        caps.features.texture2DArray = true;
        caps.features.textureCubeArray = true;

        D3D12_FEATURE_DATA_D3D12_OPTIONS5 d3d12options5 = {};
        if (SUCCEEDED(d3dDevice->CheckFeatureSupport(
            D3D12_FEATURE_D3D12_OPTIONS5,
            &d3d12options5, sizeof(d3d12options5)))
            && d3d12options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
        {
            caps.features.raytracing = true;
        }
        else
        {
            caps.features.raytracing = false;
        }
    }

    void D3D12GraphicsDevice::WaitForIdle()
    {
        SignalAndWait(&frameFence, graphicsQueue.GetHandle());

        // Wait for the GPU to fully catch up with the CPU
        //WaitForIdle(&graphicsQueue);
        //frameCount = GpuSignal(frameFence, graphicsQueue.handle);
        /*graphicsQueue.GetHandle()->Signal(frameFence, ++frameCount);
        frameFence->SetEventOnCompletion(frameCount, frameFenceEvent);
        WaitForSingleObject(frameFenceEvent, INFINITE);*/

        //GPUDescriptorHeaps[frameIndex].Size = 0;
        //GPUUploadMemoryHeaps[frameIndex].Size = 0;

        /*computeQueue->WaitForIdle();
        copyQueue->WaitForIdle();*/

        ExecuteDeferredReleases();
    }

    uint64_t D3D12GraphicsDevice::PresentFrame(uint32_t count, const GpuSwapchain* pSwapchains)
    {
        // Present swap chains.
        for (uint32_t i = 0; i < count; i++)
        {
            d3d12::Swapchain& swapchain = swapchains[pSwapchains[i].id];

            HRESULT hr = swapchain.handle->Present(swapchain.syncInterval, swapchain.flags);

            if (hr == DXGI_ERROR_DEVICE_REMOVED ||
                hr == DXGI_ERROR_DEVICE_RESET)
            {
                isLost = true;
                return static_cast<uint64_t>(-1);
            }

            ThrowIfFailed(hr);
        }

        // Signal the fence with the current frame number
        Signal(&frameFence, graphicsQueue.GetHandle());

        // Wait for the GPU to catch up before we stomp an executing command buffer
        if (frameFence.cpuValue >= kRenderLatency)
        {
            Wait(&frameFence, frameFence.cpuValue - kRenderLatency);
        }

        // Release any pending deferred releases
        ExecuteDeferredReleases();

        return ++frameCount;
    }

    void D3D12GraphicsDevice::CreateFence(d3d12::Fence* fence)
    {
        ThrowIfFailed(d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence->handle)));
        fence->cpuValue = 1;
        fence->fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        ALIMER_ASSERT(fence->fenceEvent != INVALID_HANDLE_VALUE);
    }

    void D3D12GraphicsDevice::DestroyFence(d3d12::Fence* fence)
    {
        CloseHandle(fence->fenceEvent);
        SafeRelease(fence->handle);
    }

    uint64_t D3D12GraphicsDevice::Signal(d3d12::Fence* fence, ID3D12CommandQueue* queue)
    {
        ThrowIfFailed(queue->Signal(fence->handle, fence->cpuValue));
        fence->cpuValue++;
        return fence->cpuValue - 1;
    }

    void D3D12GraphicsDevice::SignalAndWait(d3d12::Fence* fence, ID3D12CommandQueue* queue)
    {
        ThrowIfFailed(queue->Signal(fence->handle, ++fence->cpuValue));
        fence->handle->SetEventOnCompletion(fence->cpuValue, fence->fenceEvent);
        WaitForSingleObject(fence->fenceEvent, INFINITE);
    }

    void D3D12GraphicsDevice::Wait(d3d12::Fence* fence, uint64_t value)
    {
        const uint64_t gpuValue = fence->handle->GetCompletedValue();
        if (gpuValue < value)
        {
            ThrowIfFailed(fence->handle->SetEventOnCompletion(value, fence->fenceEvent));
            WaitForSingleObject(fence->fenceEvent, INFINITE);
        }
    }

    GpuSwapchain D3D12GraphicsDevice::CreateSwapChain(void* nativeHandle, uint32_t width, uint32_t height, PresentMode presentMode)
    {
        if (swapchains.isFull()) {
            ALIMER_LOGERROR("Direct3D12: reached maximum number of Swapchains allocated");
            return { kInvalidHandle };
        }

        // Determine the render target size in pixels.
        const uint32_t backBufferWidth = max<uint32_t>(width, 1u);
        const uint32_t backBufferHeight = max<uint32_t>(height, 1u);
        const DXGI_FORMAT backBufferFormat = ToDXGISwapChainFormat(PixelFormat::BGRA8UNorm);

        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = backBufferWidth;
        swapChainDesc.Height = backBufferHeight;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = kRenderLatency;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        if (presentMode == PresentMode::Immediate
            && isTearingSupported)
        {
            swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }

        // Create a swap chain for the window.
        IDXGISwapChain1* tempSwapChain;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = TRUE;

        HWND hwnd = reinterpret_cast<HWND>(nativeHandle);
        ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
            graphicsQueue.GetHandle(),
            hwnd,
            &swapChainDesc,
            &fsSwapChainDesc,
            nullptr,
            &tempSwapChain
        ));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        ThrowIfFailed(dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
#else
        IUnknown* window = reinterpret_cast<IUnknown*>(nativeHandle);

        ThrowIfFailed(dxgiFactory->CreateSwapChainForCoreWindow(
            GetD3D12GraphicsQueue(),
            window,
            &swapChainDesc,
            nullptr,
            &tempSwapChain
        ));

        /*IInspectable* swapChainPanel = reinterpret_cast<IInspectable*>(surface->GetHandle());
        ThrowIfFailed(factory->CreateSwapChainForComposition(
            device->GetD3D12GraphicsQueue(),
            &swapChainDesc,
            nullptr,
            &tempSwapChain
        ));

        ISwapChainPanelNative* panelNative;
        ThrowIfFailed(swapChainPanel->QueryInterface(__uuidof(ISwapChainPanelNative), (void**)panelNative));
        ThrowIfFailed(panelNative->SetSwapChain(tempSwapChain));*/
#endif

        const int id = swapchains.alloc();
        d3d12::Swapchain& handle = swapchains[id];
        ThrowIfFailed(tempSwapChain->QueryInterface(IID_PPV_ARGS(&handle.handle)));
        SafeRelease(tempSwapChain);

        handle.imageCount = swapChainDesc.BufferCount;
        handle.syncInterval = 1;
        handle.flags = 0;
        if (presentMode == PresentMode::Immediate
            && isTearingSupported)
        {
            handle.syncInterval = 0;
            handle.flags |= DXGI_PRESENT_ALLOW_TEARING;
        }

        for (uint32_t i = 0; i < handle.imageCount; ++i)
        {
            ID3D12Resource* resource;
            ThrowIfFailed(handle.handle->GetBuffer(i, IID_PPV_ARGS(&resource)));

            handle.textures[i] = CreateExternalTexture(resource);
        }

        return { (uint32_t)id };
    }

    void D3D12GraphicsDevice::DestroySwapChain(GpuSwapchain handle)
    {
        d3d12::Swapchain& swapchain = swapchains[handle.id];
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        swapchain.handle->SetFullscreenState(FALSE, nullptr);
#endif
        DeferredRelease(swapchain.handle);

        swapchains.dealloc(handle.id);
    }

    uint32_t D3D12GraphicsDevice::GetImageCount(GpuSwapchain handle)
    {
        d3d12::Swapchain& swapchain = swapchains[handle.id];
        return swapchain.imageCount;
    }

    GPUTexture D3D12GraphicsDevice::GetTexture(GpuSwapchain handle, uint32_t index)
    {
        d3d12::Swapchain& swapchain = swapchains[handle.id];
        return swapchain.textures[index];
    }

    uint32_t D3D12GraphicsDevice::GetNextTexture(GpuSwapchain handle)
    {
        d3d12::Swapchain& swapchain = swapchains[handle.id];
        return swapchain.handle->GetCurrentBackBufferIndex();
    }

    GPUTexture D3D12GraphicsDevice::CreateExternalTexture(ID3D12Resource* resource)
    {
        if (textures.isFull()) {
            ALIMER_LOGERROR("Direct3D12: reached maximum number of Textures allocated");
            return { kInvalidHandle };
        }

        const int id = textures.alloc();
        d3d12::Resource& texture = textures[id];
        texture.handle = resource;
        texture.state = D3D12_RESOURCE_STATE_COMMON;
        texture.transitioning_state = (D3D12_RESOURCE_STATES)-1;
        texture.gpu_virtual_address = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
        texture.rtv.ptr = 0;
        texture.rtv = RTVDescriptorHeap.AllocatePersistent().handles[0];
        d3dDevice->CreateRenderTargetView(resource, nullptr, texture.rtv);
        return { (uint32_t)id };
    }

    void D3D12GraphicsDevice::DestroyTexture(GPUTexture handle)
    {
        d3d12::Resource& texture = textures[handle.id];
        DeferredRelease(texture.handle);
        if (texture.rtv.ptr != 0)
        {
            RTVDescriptorHeap.FreePersistent(texture.rtv);
        }

        textures.dealloc(handle.id);
    }

    d3d12::Resource* D3D12GraphicsDevice::GetTexture(GPUTexture handle)
    {
        return &textures[handle.id];
    }

    GpuCommandBuffer* D3D12GraphicsDevice::CreateCommandBuffer(QueueType type)
    {
        return new D3D12GraphicsContext(*this, type);
    }

    void D3D12GraphicsDevice::DestroyCommandBuffer(GpuCommandBuffer* handle)
    {
        D3D12GraphicsContext* d3dContext = static_cast<D3D12GraphicsContext*>(handle);
        d3dContext->Destroy();
        delete d3dContext;
    }

    void D3D12GraphicsDevice::WaitForFence(uint64_t fenceValue)
    {
        D3D12CommandQueue& producer = GetQueue((D3D12_COMMAND_LIST_TYPE)(fenceValue >> 56));
        producer.WaitForFence(fenceValue);
    }
}
