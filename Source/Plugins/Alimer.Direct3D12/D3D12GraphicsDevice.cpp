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
#include "D3D12GraphicsProvider.h"
#include "D3D12GraphicsAdapter.h"
//#include "D3D12CommandQueue.h"
#include "D3D12MemAlloc.h"
#include "core/String.h"

namespace Alimer
{
    D3D12GraphicsDevice::D3D12GraphicsDevice(D3D12GraphicsProvider* provider, const std::shared_ptr<GraphicsAdapter>& adapter)
        : GraphicsDevice(adapter)
        , frameFence(this)
        , validation(provider->IsValidationEnabled())
        , dxgiFactory(provider->GetDXGIFactory())
        , isTearingSupported(provider->IsTearingSupported())
    {
        IDXGIAdapter1* dxgiAdapter = std::static_pointer_cast<D3D12GraphicsAdapter>(adapter)->GetDXGIAdapter();

        // Create the DX12 API device object.
        VHR(D3D12CreateDevice(dxgiAdapter, provider->GetMinFeatureLevel(), IID_PPV_ARGS(&d3dDevice)));

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
                //D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
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
            desc.pAdapter = dxgiAdapter;

            VHR(D3D12MA::CreateAllocator(&desc, &memoryAllocator));
            switch (memoryAllocator->GetD3D12Options().ResourceHeapTier)
            {
            case D3D12_RESOURCE_HEAP_TIER_1:
                LOG_DEBUG("ResourceHeapTier = D3D12_RESOURCE_HEAP_TIER_1");
                break;
            case D3D12_RESOURCE_HEAP_TIER_2:
                LOG_DEBUG("ResourceHeapTier = D3D12_RESOURCE_HEAP_TIER_2");
                break;
            default:
                break;
            }
        }

        // Init caps and features.
        InitCapabilities();
    }

    D3D12GraphicsDevice::~D3D12GraphicsDevice()
    {
        WaitForIdle();

        //ReleaseTrackedResources();
        //ExecuteDeferredReleases();

        Shutdown();
    }

    void D3D12GraphicsDevice::InitCapabilities()
    {
        // Init capabilities
        {
            IDXGIAdapter1* dxgiAdapter = std::static_pointer_cast<D3D12GraphicsAdapter>(adapter)->GetDXGIAdapter();

            DXGI_ADAPTER_DESC1 desc;
            VHR(dxgiAdapter->GetDesc1(&desc));

            caps.vendorId = desc.VendorId;
            caps.deviceId = desc.DeviceId;

            std::wstring deviceName(desc.Description);
            caps.adapterName = Alimer::ToUtf8(deviceName);

            // Detect adapter type.
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                caps.adapterType = GPUAdapterType::CPU;
            }
            else {
                D3D12_FEATURE_DATA_ARCHITECTURE arch = {};
                VHR(d3dDevice->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &arch, sizeof(arch)));

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
            /*UINT dxgi_fmt_caps = 0;
            for (int fmt = (VGPUTextureFormat_Undefined + 1); fmt < VGPUTextureFormat_Count; fmt++)
            {
                DXGI_FORMAT dxgi_fmt = _vgpu_d3d_get_format((VGPUTextureFormat)fmt);
                HRESULT hr = ID3D11Device1_CheckFormatSupport(renderer->d3d_device, dxgi_fmt, &dxgi_fmt_caps);
                VGPU_ASSERT(SUCCEEDED(hr));
                sg_pixelformat_info* info = &_sg.formats[fmt];
                info->sample = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_TEXTURE2D);
                info->filter = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE);
                info->render = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_RENDER_TARGET);
                info->blend = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_BLENDABLE);
                info->msaa = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET);
                info->depth = 0 != (dxgi_fmt_caps & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL);
                if (info->depth) {
                    info->render = true;
                }
            }*/
        }

        // Init command queue's
        /*{
            graphicsQueue = std::make_unique<D3D12CommandQueue>(this, D3D12_COMMAND_LIST_TYPE_DIRECT);
            computeQueue = std::make_unique<D3D12CommandQueue>(this, D3D12_COMMAND_LIST_TYPE_COMPUTE);
            copyQueue = std::make_unique<D3D12CommandQueue>(this, D3D12_COMMAND_LIST_TYPE_COPY);
        }

        shuttingDown = false;
        currentCPUFrame = 0;
        currentGPUFrame = 0;
        currentFrameIndex = 0;
        frameFence.Init(0);

        if (window) {
            CreateSwapChain(&swapChain, window, info.colorFormat);
        }*/
    }

    void D3D12GraphicsDevice::Shutdown()
    {
        ALIMER_ASSERT(currentCPUFrame == currentGPUFrame);
        shuttingDown = true;

        /*for (uint64_t i = 0; i < _countof(deferredReleases); ++i)
        {
            ProcessDeferredReleases(i);
        }*/

        frameFence.Shutdown();

        //copyQueue.reset();
        //computeQueue.reset();
        //graphicsQueue.reset();

        // Allocator
        D3D12MA::Stats stats;
        memoryAllocator->CalculateStats(&stats);

        if (stats.Total.UsedBytes > 0) {
            LOG_ERROR("Total device memory leaked: %llu bytes.", stats.Total.UsedBytes);
        }

        memoryAllocator->Release();
        memoryAllocator = nullptr;

        ULONG refCount = d3dDevice->Release();
#if !defined(NDEBUG)
        if (refCount > 0)
        {
            LOG_DEBUG("Direct3D12: There are %d unreleased references left on the device", refCount);

            ID3D12DebugDevice* debugDevice;
            if (SUCCEEDED(d3dDevice->QueryInterface(IID_PPV_ARGS(&debugDevice))))
            {
                debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_SUMMARY | D3D12_RLDO_IGNORE_INTERNAL);
                debugDevice->Release();
            }
        }
#else
        (void)refCount; // avoid warning
#endif
    }

    void D3D12GraphicsDevice::ProcessDeferredReleases(uint64_t frameIndex)
    {
        /*for (size_t i = 0; i < deferredReleases[frameIndex].size(); ++i)
        {
            deferredReleases[frameIndex][i]->Release();
        }

        deferredReleases[frameIndex].clear();*/
    }

    void D3D12GraphicsDevice::DeferredRelease_(IUnknown* resource, bool forceDeferred)
    {
        if (resource == nullptr)
            return;

        if ((currentCPUFrame == currentGPUFrame && forceDeferred == false) || shuttingDown || d3dDevice == nullptr)
        {
            // Free-for-all!
            resource->Release();
            return;
        }

       // deferredReleases[currentFrameIndex].push_back(resource);
    }

    /*D3D12CommandQueue* D3D12GraphicsDevice::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const
    {
        switch (type)
        {
        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            return computeQueue.get();
        case D3D12_COMMAND_LIST_TYPE_COPY:
            return copyQueue.get();
        default:
            return graphicsQueue.get();
        }
    }*/

    void D3D12GraphicsDevice::WaitForIdle()
    {
        // Wait for the GPU to fully catch up with the CPU
        ALIMER_ASSERT(currentCPUFrame >= currentGPUFrame);
        if (currentCPUFrame > currentGPUFrame)
        {
            frameFence.Wait(currentCPUFrame);
            currentGPUFrame = currentCPUFrame;
        }

        // Clean up what we can now
        for (uint64_t i = 1; i < kMaxFrameLatency; ++i)
        {
            uint64_t frameIndex = (i + currentFrameIndex) % kMaxFrameLatency;
            ProcessDeferredReleases(frameIndex);
        }

        //graphicsQueue->WaitForIdle();
        //computeQueue->WaitForIdle();
        //copyQueue->WaitForIdle();
    }

#if TODO_D3D12
    void D3D12GraphicsDevice::BeginFrame()
    {
    }

    void D3D12GraphicsDevice::PresentFrame()
    {
        HRESULT hr = S_OK;

        /*for (size_t i = 0, count = swapChains.size(); i < count; i++)
        {
            hr = swapChains[i]->GetHandle()->Present(syncInterval, presentFlags);
            isLost = d3dIsLost(hr);

            if (isLost)
            {
                break;
            }
        }*/

        if (!isLost)
        {
            if (SUCCEEDED(hr)
                && swapChain.handle != nullptr)
            {
                hr = swapChain.handle->Present(syncInterval, presentFlags);
                swapChain.currentBackBufferIndex = swapChain.handle->GetCurrentBackBufferIndex();
            }
        }

        isLost = d3dIsLost(hr);
        if (isLost)
        {
#ifdef _DEBUG
            char buff[64] = {};
            sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n",
                static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? d3dDevice->GetDeviceRemovedReason() : hr));
            OutputDebugStringA(buff);
#endif

            HandleDeviceLost();
            return;
        }

        ThrowIfFailed(hr);

        ++currentCPUFrame;

        // Signal the fence with the current frame number, so that we can check back on it
        frameFence.Signal(graphicsQueue->GetHandle(), currentCPUFrame);

        // Wait for the GPU to catch up before we stomp an executing command buffer
        const uint64_t gpuLag = currentCPUFrame - currentGPUFrame;
        ALIMER_ASSERT(gpuLag <= kMaxFrameLatency);
        if (gpuLag >= kMaxFrameLatency)
        {
            // Make sure that the previous frame is finished
            frameFence.Wait(currentGPUFrame + 1);
            ++currentGPUFrame;
        }

        currentFrameIndex = currentCPUFrame % kMaxFrameLatency;

        // Process any deferred releases
        ProcessDeferredReleases(currentFrameIndex);
    }

    void D3D12GraphicsDevice::HandleDeviceLost()
    {

    }

    void D3D12GraphicsDevice::CreateSwapChain(SwapChain* swapChain, window_t* window, PixelFormat colorFormat)
    {
        UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        if (syncInterval
            && d3d12.isTearingSupported)
        {
            //presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
            swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }

        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = window_width(window);
        swapChainDesc.Height = window_height(window);
        swapChainDesc.Format = ToDXGISwapChainFormat(colorFormat);
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT/* | DXGI_USAGE_SHADER_INPUT*/;
        swapChainDesc.BufferCount = kMaxFrameLatency;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapChainDesc.Flags = swapChainFlags;

        ComPtr<IDXGISwapChain1> tempSwapChain;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        HWND hwnd = reinterpret_cast<HWND>(window_handle(window));

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = TRUE;

        // Create a swap chain for the window.
        ThrowIfFailed(d3d12.dxgiFactory->CreateSwapChainForHwnd(
            graphicsQueue->GetHandle(),
            hwnd,
            &swapChainDesc,
            &fsSwapChainDesc,
            nullptr,
            tempSwapChain.GetAddressOf()
        ));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        ThrowIfFailed(d3d12.dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
#else
        IUnknown* window = reinterpret_cast<IUnknown*>(info.handle);
        ThrowIfFailed(dxgiFactory->CreateSwapChainForCoreWindow(
            d3dCommandQueue,
            window,
            &swapChainDesc,
            nullptr,
            &tempSwapChain
        ));
#endif
        ThrowIfFailed(tempSwapChain.As(&swapChain->handle));
        swapChain->currentBackBufferIndex = swapChain->handle->GetCurrentBackBufferIndex();
    }
#endif // TODO_D3D12

}
