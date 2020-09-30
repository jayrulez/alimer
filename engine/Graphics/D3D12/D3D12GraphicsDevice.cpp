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

#include "D3D12Texture.h"
#include "D3D12Buffer.h"
#include "D3D12CommandQueue.h"
#include "D3D12CommandContext.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12GraphicsDevice.h"

namespace Alimer
{
    namespace
    {
        constexpr D3D_FEATURE_LEVEL GetD3DFeatureLevel(FeatureLevel featureLevel)
        {
            switch (featureLevel)
            {
            case FeatureLevel::Level_11_1:
                return D3D_FEATURE_LEVEL_11_1;
            case FeatureLevel::Level_12_0:
                return D3D_FEATURE_LEVEL_12_0;
            case FeatureLevel::Level_12_1:
                return D3D_FEATURE_LEVEL_12_1;
            default:
                return D3D_FEATURE_LEVEL_11_0;
            }
        }

        constexpr FeatureLevel FromD3DFeatureLevel(D3D_FEATURE_LEVEL featureLevel)
        {
            switch (featureLevel)
            {
            case D3D_FEATURE_LEVEL_11_1:
                return FeatureLevel::Level_11_1;
            case D3D_FEATURE_LEVEL_12_0:
                return FeatureLevel::Level_12_0;
            case D3D_FEATURE_LEVEL_12_1:
                return FeatureLevel::Level_12_1;
            default:
                return FeatureLevel::Level_11_0;
            }
        }
    }

    GraphicsDeviceImpl::GraphicsDeviceImpl(FeatureLevel minFeatureLevel, bool enableDebugLayer)
        : featureLevel(minFeatureLevel)
        , d3dMinFeatureLevel(GetD3DFeatureLevel(minFeatureLevel))
    {
        // Enable the debug layer (requires the Graphics Tools "optional feature").
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.

        const bool GPUBasedValidation = false;

        if (enableDebugLayer)
        {
            ComPtr<ID3D12Debug> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
            {
                debugController->EnableDebugLayer();

                ComPtr<ID3D12Debug1> debugController1;
                if (SUCCEEDED(debugController.As(&debugController1)))
                {
                    debugController1->SetEnableGPUBasedValidation(GPUBasedValidation);
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

            ComPtr<IDXGIFactory5> dxgiFactory5;
            HRESULT hr = dxgiFactory.As(&dxgiFactory5);
            if (SUCCEEDED(hr))
            {
                hr = dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
            }

            if (FAILED(hr) || !allowTearing)
            {
#ifdef _DEBUG
                OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
            }
            else
            {
                isTearingSupported = true;
            }
        }

        // Get adapter and create device
        {
            ComPtr<IDXGIAdapter1> dxgiAdapter;
            GetAdapter(false, dxgiAdapter.GetAddressOf());

            // Create the DX12 API device object.
            ThrowIfFailed(D3D12CreateDevice(dxgiAdapter.Get(), d3dMinFeatureLevel, IID_PPV_ARGS(&d3dDevice)));

#ifndef NDEBUG
            // Configure debug device (if active).
            ComPtr<ID3D12InfoQueue> d3dInfoQueue;
            if (SUCCEEDED(d3dDevice.As(&d3dInfoQueue)))
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
            }
#endif

            // Create memory allocator.
            D3D12MA::ALLOCATOR_DESC desc = {};
            desc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
            desc.pDevice = d3dDevice.Get();
            desc.pAdapter = dxgiAdapter.Get();
            ThrowIfFailed(D3D12MA::CreateAllocator(&desc, &allocator));

            InitCapabilities(dxgiAdapter.Get());
        }
    }

    GraphicsDeviceImpl::~GraphicsDeviceImpl()
    {
        // Allocator.
        if (allocator != nullptr)
        {
            D3D12MA::Stats stats;
            allocator->CalculateStats(&stats);

            if (stats.Total.UsedBytes > 0) {
                LOGE("Total device memory leaked: {} bytes.", stats.Total.UsedBytes);
            }

            SafeRelease(allocator);
        }
    }

    void GraphicsDeviceImpl::GetAdapter(bool lowPower, IDXGIAdapter1** ppAdapter)
    {
        *ppAdapter = nullptr;

        ComPtr<IDXGIAdapter1> adapter = nullptr;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        ComPtr<IDXGIFactory6> dxgiFactory6;
        HRESULT hr = dxgiFactory.As(&dxgiFactory6);
        if (SUCCEEDED(hr))
        {
            const DXGI_GPU_PREFERENCE gpuPreference = lowPower ? DXGI_GPU_PREFERENCE_MINIMUM_POWER : DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;

            for (UINT adapterIndex = 0;
                SUCCEEDED(dxgiFactory6->EnumAdapterByGpuPreference(
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
                    adapter->Release();

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

        if (!adapter)
        {
            LOGE("No Direct3D 12 device found");
        }

        *ppAdapter = adapter.Detach();
    }

    void GraphicsDeviceImpl::InitCapabilities(IDXGIAdapter1* dxgiAdapter)
    {
        DXGI_ADAPTER_DESC1 desc;
        ThrowIfFailed(dxgiAdapter->GetDesc1(&desc));

        Caps.backendType = GPUBackendType::Direct3D12;
        Caps.deviceId = desc.DeviceId;
        Caps.vendorId = desc.VendorId;

        std::wstring deviceName(desc.Description);
        Caps.adapterName = Alimer::ToUtf8(deviceName);

        // Detect adapter type.
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            Caps.adapterType = GPUAdapterType::CPU;
        }
        else
        {
            D3D12_FEATURE_DATA_ARCHITECTURE arch = {};
            ThrowIfFailed(d3dDevice->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &arch, sizeof(arch)));

            Caps.adapterType = arch.UMA ? GPUAdapterType::IntegratedGPU : GPUAdapterType::DiscreteGPU;
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

        if (SUCCEEDED(d3dDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels))))
        {
            featureLevel = FromD3DFeatureLevel(featLevels.MaxSupportedFeatureLevel);
        }
        else
        {
            featureLevel = FromD3DFeatureLevel(d3dMinFeatureLevel);
        }

        // Features
        /*supportsRenderPass = false;
        if (d3d12options5.RenderPassesTier > D3D12_RENDER_PASS_TIER_0 &&
            caps.vendorId != KnownVendorId_Intel)
        {
            supportsRenderPass = true;
        }*/

        // Limits
        Caps.limits.maxVertexAttributes = kMaxVertexAttributes;
        Caps.limits.maxVertexBindings = kMaxVertexAttributes;
        Caps.limits.maxVertexAttributeOffset = kMaxVertexAttributeOffset;
        Caps.limits.maxVertexBindingStride = kMaxVertexBufferStride;

        //caps.limits.maxTextureDimension1D = D3D12_REQ_TEXTURE1D_U_DIMENSION;
        Caps.limits.maxTextureDimension2D = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
        Caps.limits.maxTextureDimension3D = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
        Caps.limits.maxTextureDimensionCube = D3D12_REQ_TEXTURECUBE_DIMENSION;
        Caps.limits.maxTextureArrayLayers = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
        Caps.limits.maxColorAttachments = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
        Caps.limits.maxUniformBufferSize = D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
        Caps.limits.minUniformBufferOffsetAlignment = 256u;
        Caps.limits.maxStorageBufferSize = UINT32_MAX;
        Caps.limits.minStorageBufferOffsetAlignment = 16;
        Caps.limits.maxSamplerAnisotropy = D3D12_MAX_MAXANISOTROPY;
        Caps.limits.maxViewports = D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        if (Caps.limits.maxViewports > kMaxViewportAndScissorRects) {
            Caps.limits.maxViewports = kMaxViewportAndScissorRects;
        }

        Caps.limits.maxViewportWidth = D3D12_VIEWPORT_BOUNDS_MAX;
        Caps.limits.maxViewportHeight = D3D12_VIEWPORT_BOUNDS_MAX;
        Caps.limits.maxTessellationPatchSize = D3D12_IA_PATCH_MAX_CONTROL_POINT_COUNT;
        Caps.limits.pointSizeRangeMin = 1.0f;
        Caps.limits.pointSizeRangeMax = 1.0f;
        Caps.limits.lineWidthRangeMin = 1.0f;
        Caps.limits.lineWidthRangeMax = 1.0f;
        Caps.limits.maxComputeSharedMemorySize = D3D12_CS_THREAD_LOCAL_TEMP_REGISTER_POOL;
        Caps.limits.maxComputeWorkGroupCountX = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        Caps.limits.maxComputeWorkGroupCountY = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        Caps.limits.maxComputeWorkGroupCountZ = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        Caps.limits.maxComputeWorkGroupInvocations = D3D12_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
        Caps.limits.maxComputeWorkGroupSizeX = D3D12_CS_THREAD_GROUP_MAX_X;
        Caps.limits.maxComputeWorkGroupSizeY = D3D12_CS_THREAD_GROUP_MAX_Y;
        Caps.limits.maxComputeWorkGroupSizeZ = D3D12_CS_THREAD_GROUP_MAX_Z;

        /* see: https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_format_support */
        UINT dxgi_fmt_caps = 0;
        for (uint32_t format = (static_cast<uint32_t>(PixelFormat::Invalid) + 1); format < static_cast<uint32_t>(PixelFormat::Count); format++)
        {
            D3D12_FEATURE_DATA_FORMAT_SUPPORT support;
            support.Format = ToDXGIFormat((PixelFormat)format);

            if (support.Format == DXGI_FORMAT_UNKNOWN)
                continue;

            ThrowIfFailed(d3dDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &support, sizeof(support)));
        }
    }


#if TODO
    D3D12GraphicsDevice::D3D12GraphicsDevice(GraphicsDebugFlags flags, PhysicalDevicePreference adapterPreference)
    {
        

        // Create Command queue's
        {
            graphicsQueue = new D3D12CommandQueue(this, D3D12_COMMAND_LIST_TYPE_DIRECT);
            computeQueue = new D3D12CommandQueue(this, D3D12_COMMAND_LIST_TYPE_COMPUTE);
            copyQueue = new D3D12CommandQueue(this, D3D12_COMMAND_LIST_TYPE_COPY);
        }

        // Heaps
        {
            rtvHeap = new D3D12DescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 256);
            dsvHeap = new D3D12DescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 256);
            cbvSrvUavCpuHeap = new D3D12DescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024);
        }

        // Create immediate context.
        immediateContext = new D3D12CommandContext(this, graphicsQueue);

        LOGI("Successfully create {} Graphics Device", ToString(caps.backendType));
    }

    D3D12GraphicsDevice::~D3D12GraphicsDevice()
    {
        WaitForGPU();
        Shutdown();
    }

    void D3D12GraphicsDevice::Shutdown()
    {
        shuttingDown = true;
        ExecuteDeferredReleases();
        SafeDelete(immediateContext);

        SafeDelete(graphicsQueue);
        SafeDelete(computeQueue);
        SafeDelete(copyQueue);

        SafeDelete(rtvHeap);
        SafeDelete(dsvHeap);
        SafeDelete(cbvSrvUavCpuHeap);

        

        ULONG refCount = d3dDevice->Release();
#if !defined(NDEBUG)
        if (refCount > 0)
        {
            LOGD("Direct3D12: There are {} unreleased references left on the device", refCount);

            ID3D12DebugDevice* debugDevice;
            if (SUCCEEDED(d3dDevice->QueryInterface<ID3D12DebugDevice>(&debugDevice)))
            {
                debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
                debugDevice->Release();
            }
        }
        else
        {
            LOGD("Direct3D12: No leaks detected");
        }
#else
        (void)refCount; // avoid warning
#endif
    }

    
    void D3D12GraphicsDevice::WaitForGPU()
    {
        immediateContext->Flush(true);
        graphicsQueue->WaitForIdle();
        computeQueue->WaitForIdle();
        copyQueue->WaitForIdle();
        ExecuteDeferredReleases();
    }

    void D3D12GraphicsDevice::WaitForFence(uint64_t fenceValue)
    {
        auto producer = GetQueue((D3D12_COMMAND_LIST_TYPE)(fenceValue >> 56));
        producer->WaitForFence(fenceValue);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE D3D12GraphicsDevice::AllocateCpuDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 count)
    {
        if (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
        {
            return rtvHeap->Allocate(count);
        }
        if (type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
        {
            return dsvHeap->Allocate(count);
        }
        if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
        {
            return cbvSrvUavCpuHeap->Allocate(count);
        }

        ALIMER_FATAL("Invalid heap type");
        return D3D12_CPU_DESCRIPTOR_HANDLE{ 0 };
    }

    void D3D12GraphicsDevice::ReleaseResource(IUnknown* resource)
    {
        if (resource == nullptr)
            return;

        if (shuttingDown || d3dDevice == nullptr) {
            resource->Release();
            return;
        }

        deferredReleases.push({ frameCount, resource });
    }

    void D3D12GraphicsDevice::ExecuteDeferredReleases()
    {
        uint64_t gpuValue = graphicsQueue->GetFenceCompletedValue();
        while (deferredReleases.size())
        {
            if (shuttingDown || deferredReleases.front().frameID <= gpuValue)
            {
                deferredReleases.front().resource->Release();
                deferredReleases.pop();
            }
        }
    }

    void D3D12GraphicsDevice::SetDeviceLost()
    {
        HRESULT result = d3dDevice->GetDeviceRemovedReason();

        const char* reason = "?";
        switch (result)
        {
        case DXGI_ERROR_DEVICE_HUNG:			reason = "HUNG"; break;
        case DXGI_ERROR_DEVICE_REMOVED:			reason = "REMOVED"; break;
        case DXGI_ERROR_DEVICE_RESET:			reason = "RESET"; break;
        case DXGI_ERROR_DRIVER_INTERNAL_ERROR:	reason = "INTERNAL_ERROR"; break;
        case DXGI_ERROR_INVALID_CALL:			reason = "INVALID_CALL"; break;
        }

        LOGE("The Direct3D 12 device has been removed (Error: {} '{}').  Please restart the application.", result, reason);
    }

    void D3D12GraphicsDevice::FinishFrame()
    {
        frameCount++;

        ExecuteDeferredReleases();

        if (!dxgiFactory->IsCurrent())
        {
            // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
            ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));
        }
    }
#endif // TODO

}
