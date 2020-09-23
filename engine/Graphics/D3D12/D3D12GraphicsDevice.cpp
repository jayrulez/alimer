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
#include "D3D12SwapChain.h"
#include "D3D12GraphicsDevice.h"

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

        static HMODULE dxilDLL = LoadLibraryA("dxil.dll");
        if (dxilDLL) {
            FreeLibrary(dxilDLL);
        }

        static HMODULE dxcompilerDLL = LoadLibraryA("dxcompiler.dll");
        if (!dxcompilerDLL) {
            return false;
        }

        DxcCreateInstance = (PFN_DXC_CREATE_INSTANCE)GetProcAddress(dxcompilerDLL, "DxcCreateInstance");
        //FreeLibrary(dxcompilerDLL);
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

#if defined(_DEBUG)
        // Enable the debug layer (requires the Graphics Tools "optional feature").
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.

        // TODO: Parse command line.
        if (desc.enableDebugLayer)
        {
            ID3D12Debug* debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            {
                debugController->EnableDebugLayer();

                const bool enableGPUBasedValidation = false;
                if (enableGPUBasedValidation)
                {
                    ID3D12Debug1* debugController1 = nullptr;
                    HRESULT hr = debugController->QueryInterface(&debugController1);
                    if (SUCCEEDED(hr))
                    {
                        debugController1->SetEnableGPUBasedValidation(TRUE);
                    }

                    SafeRelease(debugController1);
                }

                debugController->Release();
            }
            else
            {
                OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
            }

            IDXGIInfoQueue* dxgiInfoQueue;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
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
        }
#endif

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

        // Create Command queue's
        {
            D3D12_COMMAND_QUEUE_DESC queueDesc = {};
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
            queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            queueDesc.NodeMask = 0;

            ThrowIfFailed(d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&graphicsQueue)));
            graphicsQueue->SetName(L"Graphics Command Queue");

            // Compute
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
            ThrowIfFailed(d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&computeQueue)));
            computeQueue->SetName(L"Compute Command Queue");

            // Copy
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
            ThrowIfFailed(d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&copyQueue)));
            copyQueue->SetName(L"Copy Command Queue");
        }

        // Create descriptor heaps (TODO: Improve this)
        // Render target descriptor heap (RTV).
        {
            RTVHeap.Capacity = 1024;

            D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
            HeapDesc.NumDescriptors = RTVHeap.Capacity;
            HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&RTVHeap.Heap)));
            RTVHeap.DescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            RTVHeap.CPUStart = RTVHeap.Heap->GetCPUDescriptorHandleForHeapStart();
        }

        // Depth-stencil descriptor heap (DSV).
        {
            DSVHeap.Capacity = 1024;

            D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
            HeapDesc.NumDescriptors = DSVHeap.Capacity;
            HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&DSVHeap.Heap)));
            DSVHeap.DescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            DSVHeap.CPUStart = DSVHeap.Heap->GetCPUDescriptorHandleForHeapStart();
        }

        // Create primary SwapChain.
        //primarySwapChain.Reset(new D3D12SwapChain(this, desc.primarySwapChain));

        // Frame fence
        frameFence.Init(this, 0);
        frameFence.d3dFence->SetName(L"Frame Fence");
    }

    D3D12GraphicsDevice::~D3D12GraphicsDevice()
    {
        Shutdown();
    }

    void D3D12GraphicsDevice::Shutdown()
    {
        WaitForGPU();

        ALIMER_ASSERT(frameCount == GPUFrameCount);
        shuttingDown = true;

        for (uint64 i = 0; i < ALIMER_STATIC_ARRAY_SIZE(deferredReleases); ++i)
        {
            ProcessDeferredReleases(i);
        }

        frameFence.Shutdown();

        primarySwapChain.Reset();

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

        // Descriptor heaps
        {
            SafeRelease(RTVHeap.Heap);
            SafeRelease(DSVHeap.Heap);
        }

        SafeRelease(graphicsQueue);
        SafeRelease(computeQueue);
        SafeRelease(copyQueue);

        ULONG refCount = d3dDevice->Release();
#if !defined(NDEBUG)
        if (refCount > 0)
        {
            LOGD("Direct3D12: There are {} unreleased references left on the device", refCount);

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
            }

            OutputDebugStringA("Direct3D Adapter - WARP12\n");
        }
#endif

        if (!adapter)
        {
            LOGE("No Direct3D 12 device found");
        }

        *ppAdapter = adapter.Detach();
    }

    void D3D12GraphicsDevice::HandleDeviceLost()
    {
        HRESULT hr = d3dDevice->GetDeviceRemovedReason();

        const char* reason = "?";
        switch (hr)
        {
        case DXGI_ERROR_DEVICE_HUNG:			reason = "HUNG"; break;
        case DXGI_ERROR_DEVICE_REMOVED:			reason = "REMOVED"; break;
        case DXGI_ERROR_DEVICE_RESET:			reason = "RESET"; break;
        case DXGI_ERROR_DRIVER_INTERNAL_ERROR:	reason = "INTERNAL_ERROR"; break;
        case DXGI_ERROR_INVALID_CALL:			reason = "INVALID_CALL"; break;
        }

        LOGE("The Direct3D12 device Lost on Present: (Error:  0x%08X '%s')", hr, reason);
        isLost = true;
    }


    void D3D12GraphicsDevice::InitCapabilities(IDXGIAdapter1* adapter)
    {
        DXGI_ADAPTER_DESC1 desc;
        ThrowIfFailed(adapter->GetDesc1(&desc));

        caps.backendType = GPUBackendType::Direct3D12;
        caps.deviceId = desc.DeviceId;
        caps.vendorId = desc.VendorId;

        std::wstring deviceName(desc.Description);
        caps.adapterName = Alimer::ToUtf8(deviceName);

        // Detect adapter type.
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            caps.adapterType = GPUAdapterType::CPU;
        }
        else
        {
            D3D12_FEATURE_DATA_ARCHITECTURE arch = {};
            ThrowIfFailed(d3dDevice->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &arch, sizeof(arch)));

            caps.adapterType = arch.UMA ? GPUAdapterType::IntegratedGPU : GPUAdapterType::DiscreteGPU;
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
            d3dFeatureLevel = featLevels.MaxSupportedFeatureLevel;
        }
        else
        {
            d3dFeatureLevel = d3dMinFeatureLevel;
        }

        // Features
        /*supportsRenderPass = false;
        if (d3d12options5.RenderPassesTier > D3D12_RENDER_PASS_TIER_0 &&
            caps.vendorId != KnownVendorId_Intel)
        {
            supportsRenderPass = true;
        }*/

        // Limits
        caps.limits.maxVertexAttributes = kMaxVertexAttributes;
        caps.limits.maxVertexBindings = kMaxVertexAttributes;
        caps.limits.maxVertexAttributeOffset = kMaxVertexAttributeOffset;
        caps.limits.maxVertexBindingStride = kMaxVertexBufferStride;

        //caps.limits.maxTextureDimension1D = D3D12_REQ_TEXTURE1D_U_DIMENSION;
        caps.limits.maxTextureDimension2D = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
        caps.limits.maxTextureDimension3D = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
        caps.limits.maxTextureDimensionCube = D3D12_REQ_TEXTURECUBE_DIMENSION;
        caps.limits.maxTextureArrayLayers = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
        caps.limits.maxColorAttachments = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
        caps.limits.maxUniformBufferSize = D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
        caps.limits.minUniformBufferOffsetAlignment = 256u;
        caps.limits.maxStorageBufferSize = UINT32_MAX;
        caps.limits.minStorageBufferOffsetAlignment = 16;
        caps.limits.maxSamplerAnisotropy = D3D12_MAX_MAXANISOTROPY;
        caps.limits.maxViewports = D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        if (caps.limits.maxViewports > kMaxViewportAndScissorRects) {
            caps.limits.maxViewports = kMaxViewportAndScissorRects;
        }

        caps.limits.maxViewportWidth = D3D12_VIEWPORT_BOUNDS_MAX;
        caps.limits.maxViewportHeight = D3D12_VIEWPORT_BOUNDS_MAX;
        caps.limits.maxTessellationPatchSize = D3D12_IA_PATCH_MAX_CONTROL_POINT_COUNT;
        caps.limits.pointSizeRangeMin = 1.0f;
        caps.limits.pointSizeRangeMax = 1.0f;
        caps.limits.lineWidthRangeMin = 1.0f;
        caps.limits.lineWidthRangeMax = 1.0f;
        caps.limits.maxComputeSharedMemorySize = D3D12_CS_THREAD_LOCAL_TEMP_REGISTER_POOL;
        caps.limits.maxComputeWorkGroupCountX = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        caps.limits.maxComputeWorkGroupCountY = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        caps.limits.maxComputeWorkGroupCountZ = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        caps.limits.maxComputeWorkGroupInvocations = D3D12_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
        caps.limits.maxComputeWorkGroupSizeX = D3D12_CS_THREAD_GROUP_MAX_X;
        caps.limits.maxComputeWorkGroupSizeY = D3D12_CS_THREAD_GROUP_MAX_Y;
        caps.limits.maxComputeWorkGroupSizeZ = D3D12_CS_THREAD_GROUP_MAX_Z;

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

    D3D12_CPU_DESCRIPTOR_HANDLE D3D12GraphicsDevice::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count)
    {
        D3D12DescriptorHeap* descriptorHeap = nullptr;

        if (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
        {
            descriptorHeap = &RTVHeap;
        }
        else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
        {
            descriptorHeap = &DSVHeap;
        }

        ALIMER_ASSERT(descriptorHeap);
        ALIMER_ASSERT((descriptorHeap->Size + count) < descriptorHeap->Capacity);

        D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
        CPUHandle.ptr = descriptorHeap->CPUStart.ptr + (size_t)descriptorHeap->Size * descriptorHeap->DescriptorSize;
        descriptorHeap->Size += count;

        return CPUHandle;
    }

    /*bool D3D12GraphicsDevice::IsFeatureSupported(Feature feature) const
    {
        switch (feature)
        {
        case Feature::Instancing:
            return true;
        case Feature::IndependentBlend:
            return true;
        case Feature::ComputeShader:
            return true;
        case Feature::LogicOp:
            return true;
        case Feature::MultiViewport:
            return true;
        case Feature::IndexUInt32:
            return true;
        case Feature::MultiDrawIndirect:
            return true;
        case Feature::FillModeNonSolid:
            return true;
        case Feature::SamplerAnisotropy:
            return true;
        case Feature::TextureCompressionETC2:
            return false;
        case Feature::TextureCompressionASTC_LDR:
            return false;
        case Feature::TextureCompressionBC:
            return true;
        case Feature::TextureMultisample:
            return true;
        case Feature::TextureCubeArray:
            return true;
        case Feature::Raytracing:
        {
            D3D12_FEATURE_DATA_D3D12_OPTIONS5 d3d12options5 = {};
            if (SUCCEEDED(d3dDevice->CheckFeatureSupport(
                D3D12_FEATURE_D3D12_OPTIONS5,
                &d3d12options5, sizeof(d3d12options5)))
                && d3d12options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
            {
                return true;
            }

            return false;
        }

        default:
            ALIMER_UNREACHABLE();
            return false;
        }
    }*/

    void D3D12GraphicsDevice::WaitForGPU()
    {
        // Wait for the GPU to fully catch up with the CPU.
        ALIMER_ASSERT(frameCount >= GPUFrameCount);
        if (frameCount > GPUFrameCount)
        {
            frameFence.Wait(frameCount);
            GPUFrameCount = frameCount;
        }

        // Clean up what we can now
        for (uint64 i = 1; i < kRenderLatency; ++i)
        {
            uint64 index = (i + frameIndex) % kRenderLatency;
            ProcessDeferredReleases(index);
        }
    }

    FrameOpResult D3D12GraphicsDevice::BeginFrame()
    {
       /* ALIMER_UNUSED(flags);

        D3D12SwapChain* d3d11SwapChain = static_cast<D3D12SwapChain*>(swapChain);
        d3d11SwapChain->BeginFrame();*/
        return FrameOpResult::Success;
    }

    FrameOpResult D3D12GraphicsDevice::EndFrame(EndFrameFlags flags)
    {
        // TODO: Manage upload

        // Execute deferred command lists.
        /*D3D12SwapChain* d3d11SwapChain = static_cast<D3D12SwapChain*>(swapChain);
        FrameOpResult result = d3d11SwapChain->EndFrame(graphicsQueue, flags);
        if (result != FrameOpResult::Success)
            return result;
        */

        // Increase frame count.
        frameCount++;

        // Signal the fence with the current frame number, so that we can check back on it.
        frameFence.Signal(graphicsQueue, frameCount);

        // Wait for the GPU to catch up before we stomp an executing command buffer
        const uint64 gpuLag = frameCount - GPUFrameCount;
        ALIMER_ASSERT(gpuLag <= kRenderLatency);
        if (gpuLag >= kRenderLatency)
        {
            // Make sure that the previous frame is finished
            frameFence.Wait(GPUFrameCount + 1);
            ++GPUFrameCount;
        }

        frameIndex = frameCount % kRenderLatency;

        // Perform deferred release for current frame index.
        ProcessDeferredReleases(frameIndex);

        // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
        if (!dxgiFactory->IsCurrent())
        {
            ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));
        }

        return FrameOpResult::Success;
    }

    void D3D12GraphicsDevice::DeferredRelease_(IUnknown* resource, bool forceDeferred)
    {
        if (resource == nullptr)
            return;

        if ((frameCount == GPUFrameCount && forceDeferred == false) || shuttingDown || d3dDevice == nullptr)
        {
            // Safe to release.
            resource->Release();
            return;
        }

        deferredReleases[frameIndex].push_back(resource);
    }

    void D3D12GraphicsDevice::ProcessDeferredReleases(uint64 index)
    {
        for (uint64 i = 0, count = deferredReleases[index].size(); i < count; ++i)
        {
            deferredReleases[index][i]->Release();
        }

        deferredReleases[index].clear();
    }

    /*RefPtr<GPUBuffer> D3D12GraphicsDevice::CreateBufferCore(const BufferDescription& desc, const void* initialData)
    {
        return MakeRefPtr<D3D12Buffer>(this, desc, initialData);
    }*/

#if !defined(ALIMER_DISABLE_SHADER_COMPILER)
    IDxcLibrary* D3D12GraphicsDevice::GetOrCreateDxcLibrary()
    {
        if (dxcLibrary == nullptr)
        {
            HRESULT hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&dxcLibrary));
            if (FAILED(hr))
            {
                LOGE("DXC: Failed to create library");
            }

            ALIMER_ASSERT(dxcLibrary != nullptr);
        }

        return dxcLibrary.Get();
    }

    IDxcCompiler* D3D12GraphicsDevice::GetOrCreateDxcCompiler()
    {
        if (dxcCompiler == nullptr)
        {
            HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
            if (FAILED(hr))
            {
                LOGE("DXC: Failed to create compiler");
            }

            ALIMER_ASSERT(dxcCompiler != nullptr);
        }

        return dxcCompiler.Get();
    }
#endif
}
