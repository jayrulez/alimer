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

#include "D3D12GraphicsImpl.h"
//#include "D3D12SwapChain.h"
#include "D3D12Texture.h"
//#include "D3D12Buffer.h"
#include "D3D12CommandQueue.h"
#include "D3D12CommandContext.h"
#include "Core/Window.h"
#include "Math/Matrix4x4.h"

namespace alimer
{
    bool D3D12GraphicsImpl::IsAvailable()
    {
        static bool available_initialized = false;
        static bool available = false;
        if (available_initialized) {
            return available;
        }

        available_initialized = true;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        static HMODULE dxgiLib = LoadLibraryA("dxgi.dll");
        CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(dxgiLib, "CreateDXGIFactory2");
        if (CreateDXGIFactory2 == nullptr) {
            return false;
        }
        DXGIGetDebugInterface1 = (PFN_GET_DXGI_DEBUG_INTERFACE1)GetProcAddress(dxgiLib, "DXGIGetDebugInterface1");

        static HMODULE d3d12Lib = LoadLibraryA("d3d12.dll");
        D3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(d3d12Lib, "D3D12CreateDevice");
        D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(d3d12Lib, "D3D12GetDebugInterface");
        if (D3D12CreateDevice == nullptr) {
            return false;
        }

        D3D12SerializeRootSignature = (PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)GetProcAddress(d3d12Lib, "D3D12SerializeRootSignature");
        D3D12CreateRootSignatureDeserializer = (PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(d3d12Lib, "D3D12CreateRootSignatureDeserializer");
        D3D12SerializeVersionedRootSignature = (PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE)GetProcAddress(d3d12Lib, "D3D12SerializeVersionedRootSignature");
        D3D12CreateVersionedRootSignatureDeserializer = (PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(d3d12Lib, "D3D12CreateVersionedRootSignatureDeserializer");
#endif

        available = true;
        return true;
    }

    D3D12GraphicsImpl::D3D12GraphicsImpl(Window* window, GPUFlags flags)
        : GraphicsDevice(window)
        , frameIndex(0)
        , frameActive(false)
    {
        ALIMER_VERIFY(IsAvailable());

        // Enable the debug layer (requires the Graphics Tools "optional feature").
        //
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        if (any(flags & GPUFlags::DebugRuntime | GPUFlags::GPUBaseValidation))
        {
            ComPtr<ID3D12Debug> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
            {
                debugController->EnableDebugLayer();

                if (any(flags & GPUFlags::GPUBaseValidation))
                {
                    ComPtr<ID3D12Debug1> d3d12Debug1;
                    if (SUCCEEDED(debugController.As(&d3d12Debug1)))
                    {
                        d3d12Debug1->SetEnableGPUBasedValidation(true);
                        //d3d12Debug1->SetEnableSynchronizedCommandQueueValidation(TRUE);
                    }
                }
            }
            else
            {
                OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
            }

#if defined(_DEBUG)
            IDXGIInfoQueue* dxgiInfoQueue;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
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
#endif
        }

        ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));

        // Determines whether tearing support is available for fullscreen borderless windows.
        {
            BOOL allowTearing = FALSE;

            ComPtr<IDXGIFactory5> dxgiFactory5 = nullptr;
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
                dxgiFactoryCaps |= DXGIFactoryCaps::Tearing;
            }
        }

        ComPtr<IDXGIAdapter1> adapter;
        GetAdapter(&adapter);

        // Create the DX12 API device object.
        ThrowIfFailed(D3D12CreateDevice(adapter.Get(), minFeatureLevel, IID_PPV_ARGS(&d3dDevice)));
        d3dDevice->SetName(L"Alimer Device");

        // Configure debug device (if active).
#ifndef NDEBUG
        {
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
                    D3D12_MESSAGE_ID_COPY_DESCRIPTORS_INVALID_RANGES,
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
                LOGD("ResourceHeapTier = D3D12_RESOURCE_HEAP_TIER_1");
                break;
            case D3D12_RESOURCE_HEAP_TIER_2:
                LOGD("ResourceHeapTier = D3D12_RESOURCE_HEAP_TIER_2");
                break;
            default:
                break;
            }
        }

        InitCapabilities(adapter.Get());

        // Create command queue's
        //graphicsQueue.reset(new D3D12CommandQueue(this, D3D12_COMMAND_LIST_TYPE_DIRECT));
        //computeQueue.reset(new D3D12CommandQueue(this, D3D12_COMMAND_LIST_TYPE_COMPUTE));
        //copyQueue.reset(new D3D12CommandQueue(this, D3D12_COMMAND_LIST_TYPE_COPY));

        // Create immediate default contexts
        //immediateContext.reset(new D3D12CommandContext(this));

        // Create main swapchain.
        {
            /*IDXGISwapChain1* tempSwapChain = DXGICreateSwapchain(
                dxgiFactory.Get(),
                dxgiFactoryCaps,
                graphicsQueue->GetCommandQueue(),
                window->GetNativeHandle(),
                backbufferWidth, backbufferHeight,
                PixelFormat::BGRA8Unorm,
                backbufferCount,
                window->IsFullscreen()
            );

            ThrowIfFailed(tempSwapChain->QueryInterface(IID_PPV_ARGS(&swapChain)));
            SafeRelease(tempSwapChain);

            backbufferIndex = swapChain->GetCurrentBackBufferIndex();*/
        }

        // Create a fence for tracking GPU execution progress.
        {
            ThrowIfFailed(d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frameFence)));
            frameFence->SetName(L"Frame Fence");

            frameFenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
            if (!frameFenceEvent)
            {
                LOGE("Direct3D12: CreateEventEx failed.");
            }
        }
    }

    D3D12GraphicsImpl::~D3D12GraphicsImpl()
    {
        // Allocator
        D3D12MA::Stats stats;
        allocator->CalculateStats(&stats);

        if (stats.Total.UsedBytes > 0) {
            LOGE("Total device memory leaked: %llu bytes.", stats.Total.UsedBytes);
        }

        SafeRelease(allocator);

        // Release Swapchain resources
        {
            SafeRelease(swapChain);
        }

        copyQueue.reset();
        computeQueue.reset();
        graphicsQueue.reset();

        {
            CloseHandle(frameFenceEvent);
            SafeRelease(frameFence);
        }

        immediateContext.reset();

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

        // Release factory at last.
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

    void D3D12GraphicsImpl::InitCapabilities(IDXGIAdapter1* dxgiAdapter)
    {
        DXGI_ADAPTER_DESC1 desc;
        ThrowIfFailed(dxgiAdapter->GetDesc1(&desc));

        caps.backendType = GPUBackendType::Direct3D12;
        caps.vendorId = desc.VendorId;
        caps.deviceId = desc.DeviceId;

        std::wstring deviceName(desc.Description);
        caps.adapterName = ToUtf8(deviceName);

        // Detect adapter type.
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            caps.adapterType = GPUAdapterType::CPU;
        }
        else {
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

        HRESULT hr = d3dDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
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
        caps.features.textureCompressionETC2 = false;
        caps.features.textureCompressionASTC_LDR = false;
        caps.features.textureCompressionBC = true;
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

        supportsRenderPass = false;
        if (d3d12options5.RenderPassesTier > D3D12_RENDER_PASS_TIER_0 &&
            static_cast<GPUKnownVendorId>(caps.vendorId) != GPUKnownVendorId::Intel)
        {
            supportsRenderPass = true;
        }

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

    void D3D12GraphicsImpl::GetAdapter(IDXGIAdapter1** ppAdapter, bool lowPower)
    {
        *ppAdapter = nullptr;

        ComPtr<IDXGIAdapter1> adapter;

#if defined(__dxgi1_6_h__) && defined(NTDDI_WIN10_RS4)
        ComPtr<IDXGIFactory6> dxgiFactory6;
        HRESULT hr = dxgiFactory.As(&dxgiFactory6);
        if (SUCCEEDED(hr))
        {
            DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
            if (lowPower) {
                gpuPreference = DXGI_GPU_PREFERENCE_MINIMUM_POWER;
            }

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
                if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), minFeatureLevel, _uuidof(ID3D12Device), nullptr)))
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
                if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), minFeatureLevel, _uuidof(ID3D12Device), nullptr)))
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

    void D3D12GraphicsImpl::CreateNewCommandList(D3D12_COMMAND_LIST_TYPE type, ID3D12GraphicsCommandList** commandList, ID3D12CommandAllocator** commandAllocator)
    {
        ALIMER_ASSERT_MSG(type != D3D12_COMMAND_LIST_TYPE_BUNDLE, "Bundles are not yet supported");
        switch (type)
        {
        case D3D12_COMMAND_LIST_TYPE_DIRECT: *commandAllocator = graphicsQueue->RequestAllocator(); break;
        case D3D12_COMMAND_LIST_TYPE_BUNDLE: break;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE: *commandAllocator = computeQueue->RequestAllocator(); break;
        case D3D12_COMMAND_LIST_TYPE_COPY: *commandAllocator = copyQueue->RequestAllocator(); break;
        }

        ThrowIfFailed(d3dDevice->CreateCommandList(1, type, *commandAllocator, nullptr, IID_PPV_ARGS(commandList)));
    }

    void D3D12GraphicsImpl::ExecuteCommandList(D3D12_COMMAND_LIST_TYPE type, ID3D12GraphicsCommandList* commandList, bool waitForCompletion)
    {
        ALIMER_ASSERT_MSG(type != D3D12_COMMAND_LIST_TYPE_BUNDLE, "Bundles are not yet supported");

        D3D12CommandQueue* commandQueue = nullptr;
        switch (type)
        {
        case D3D12_COMMAND_LIST_TYPE_DIRECT: commandQueue = graphicsQueue.get(); break;
        case D3D12_COMMAND_LIST_TYPE_BUNDLE: break;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE: commandQueue = computeQueue.get(); break;
        case D3D12_COMMAND_LIST_TYPE_COPY: commandQueue = copyQueue.get(); break;
        }

        uint64_t fenceValue = commandQueue->ExecuteCommandList(commandList);

        if (waitForCompletion)
            WaitForFence(fenceValue);
    }

    void D3D12GraphicsImpl::WaitForGPU()
    {
        graphicsQueue->WaitForIdle();
        computeQueue->WaitForIdle();
        copyQueue->WaitForIdle();
    }

    void D3D12GraphicsImpl::WaitForFence(uint64_t fenceValue)
    {
        //CommandQueue& Producer = Graphics::g_CommandManager.GetQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
        //Producer.WaitForFence(FenceValue);
    }

    bool D3D12GraphicsImpl::BeginFrame()
    {
        ALIMER_ASSERT_MSG(!frameActive, "Frame is still active, please call EndFrame first");

        //backbufferIndex = swapChain->GetCurrentBackBufferIndex();

        // Now the frame is active again.
        frameActive = true;

        return true;
    }

    void D3D12GraphicsImpl::EndFrame()
    {
        ALIMER_ASSERT_MSG(frameActive, "Frame is not active, please call BeginFrame first.");

        /*immediateContext->Flush(false);

        uint32_t syncInterval = 1;
        uint32_t presentFlags = 0;
        if (!vSync) {
            syncInterval = 0;
            if (any(dxgiFactoryCaps & DXGIFactoryCaps::Tearing)) {
                presentFlags = DXGI_PRESENT_ALLOW_TEARING;
            }
        }

        // Recommended to always use tearing if supported when using a sync interval of 0.
        // Note this will fail if in true 'fullscreen' mode.
        HRESULT hr = swapChain->Present(syncInterval, presentFlags);

        // If the device was reset we must completely reinitialize the renderer.
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
#ifdef _DEBUG
            char buff[64] = {};
            sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n",
                static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? d3dDevice->GetDeviceRemovedReason() : hr));
            OutputDebugStringA(buff);
#endif
            HandleDeviceLost();
        }
        else
        {
            ThrowIfFailed(hr);

            graphicsQueue->GetCommandQueue()->Signal(frameFence, ++frameCount);

            uint64_t GPUFrameCount = frameFence->GetCompletedValue();

            if ((frameCount - GPUFrameCount) >= backbufferCount)
            {
                frameFence->SetEventOnCompletion(GPUFrameCount + 1, frameFenceEvent);
                WaitForSingleObject(frameFenceEvent, INFINITE);
            }

            frameIndex = (frameIndex + 1) % backbufferCount;

            if (!dxgiFactory->IsCurrent())
            {
                // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
                ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));
            }
        }*/

        // Frame is not active anymore
        frameActive = false;
    }


    void D3D12GraphicsImpl::HandleDeviceLost()
    {

    }

    Texture* D3D12GraphicsImpl::GetBackbufferTexture() const
    {
        return backbufferTextures[backbufferIndex].Get();
    }

    std::shared_ptr<CommandQueue> D3D12GraphicsImpl::CreateCommandQueue(CommandQueueType queueType, const std::string_view& name)
    {
        return std::make_shared<D3D12CommandQueue>(this, queueType, name);
    }
}
