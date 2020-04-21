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
#include "D3D12GPUProvider.h"
#include "D3D12GPUAdapter.h"
//#include "D3D12CommandContext.h"
//#include "D3D12GraphicsBuffer.h"
#include "core/String.h"
#include "D3D12MemAlloc.h"
#include "math/math.h"
#if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
#include <windows.ui.xaml.media.dxinterop.h>
#endif

namespace alimer
{
    D3D12GPUDevice::D3D12GPUDevice(D3D12GPUProvider* provider, D3D12GPUAdapter* adapter)
        : GPUDevice(provider, adapter)
        //, graphicsQueue(QueueType::Graphics)
        //, computeQueue(QueueType::Compute)
        //, copyQueue(QueueType::Copy)
        , RTVDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false)
        , DSVDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false)
    {
        // Create the DX12 API device object.
        ThrowIfFailed(D3D12CreateDevice(adapter->GetHandle(), minFeatureLevel, IID_PPV_ARGS(d3dDevice.ReleaseAndGetAddressOf())));

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
        }
#endif

        // Create memory allocator
        {
            D3D12MA::ALLOCATOR_DESC desc = {};
            desc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
            desc.pDevice = d3dDevice.Get();
            desc.pAdapter = adapter->GetHandle();

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
            //graphicsQueue.Create(d3dDevice);
            //computeQueue.Create(d3dDevice);
            //copyQueue.Create(d3dDevice);
        }

        // Create the main swap chain
        //swapChain.reset(new D3D12SwapChain(this, dxgiFactory.Get(), surface, renderLatency));

        // Create the main context.
        //mainContext.reset(new D3D12GraphicsContext(this, D3D12_COMMAND_LIST_TYPE_DIRECT, renderLatency));

        // Init descriptor heaps
        RTVDescriptorHeap.Init(256, 0);
        DSVDescriptorHeap.Init(256, 0);

        // Create frame fence.
        //CreateFence(&frameFence);

        // Init caps
        InitCapabilities();
    }

    D3D12GPUDevice::~D3D12GPUDevice()
    {
        Shutdown();
    }

    void D3D12GPUDevice::Shutdown()
    {
        /*if (frameFence.cpuValue != frameFence.handle->GetCompletedValue())
        {
            WaitForIdle();
        }

        ALIMER_ASSERT(frameFence.cpuValue == frameFence.handle->GetCompletedValue());*/
        shuttingDown = true;

        ReleaseTrackedResources();
        //ExecuteDeferredReleases();

        // Destroy descripton heaps.
        {
            RTVDescriptorHeap.Shutdown();
            DSVDescriptorHeap.Shutdown();
            //SRVDescriptorHeap.Shutdown();
            //UAVDescriptorHeap.Shutdown();
        }

        //DestroyFence(&frameFence);

        //copyQueue.Destroy();
        //computeQueue.Destroy();
        //graphicsQueue.Destroy();

        // Allocator
        D3D12MA::Stats stats;
        allocator->CalculateStats(&stats);

        if (stats.Total.UsedBytes > 0) {
            ALIMER_LOGE("Total device memory leaked: %llu bytes.", stats.Total.UsedBytes);
        }

        SafeRelease(allocator);

#if !defined(NDEBUG)
        ULONG refCount = d3dDevice.Reset();
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
        d3dDevice.Reset();
#endif

        _adapter.reset();
    }

    void D3D12GPUDevice::InitCapabilities()
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
        _features.independentBlend = true;
        _features.computeShader = true;
        _features.geometryShader = true;
        _features.tessellationShader = true;
        _features.logicOp = true;
        _features.multiViewport = true;
        _features.fullDrawIndexUint32 = true;
        _features.multiDrawIndirect = true;
        _features.fillModeNonSolid = true;
        _features.samplerAnisotropy = true;
        _features.textureCompressionETC2 = false;
        _features.textureCompressionASTC_LDR = false;
        _features.textureCompressionBC = true;
        _features.textureCubeArray = true;

        D3D12_FEATURE_DATA_D3D12_OPTIONS5 d3d12options5 = {};
        if (SUCCEEDED(d3dDevice->CheckFeatureSupport(
            D3D12_FEATURE_D3D12_OPTIONS5,
            &d3d12options5, sizeof(d3d12options5)))
            && d3d12options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
        {
            _features.raytracing = true;
        }
        else
        {
            _features.raytracing = false;
        }

        // Limits
        _limits.maxVertexAttributes = kMaxVertexAttributes;
        _limits.maxVertexBindings = kMaxVertexAttributes;
        _limits.maxVertexAttributeOffset = kMaxVertexAttributeOffset;
        _limits.maxVertexBindingStride = kMaxVertexBufferStride;

        //caps.limits.maxTextureDimension1D = D3D12_REQ_TEXTURE1D_U_DIMENSION;
        _limits.maxTextureDimension2D = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
        _limits.maxTextureDimension3D = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
        _limits.maxTextureDimensionCube = D3D12_REQ_TEXTURECUBE_DIMENSION;
        _limits.maxTextureArrayLayers = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
        _limits.maxColorAttachments = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
        _limits.maxUniformBufferSize = D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
        _limits.minUniformBufferOffsetAlignment = 256u;
        _limits.maxStorageBufferSize = UINT32_MAX;
        _limits.minStorageBufferOffsetAlignment = 16;
        _limits.maxSamplerAnisotropy = D3D12_MAX_MAXANISOTROPY;
        _limits.maxViewports = D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        _limits.maxViewportWidth = D3D12_VIEWPORT_BOUNDS_MAX;
        _limits.maxViewportHeight = D3D12_VIEWPORT_BOUNDS_MAX;
        _limits.maxTessellationPatchSize = D3D12_IA_PATCH_MAX_CONTROL_POINT_COUNT;
        _limits.pointSizeRangeMin = 1.0f;
        _limits.pointSizeRangeMax = 1.0f;
        _limits.lineWidthRangeMin = 1.0f;
        _limits.lineWidthRangeMax = 1.0f;
        _limits.maxComputeSharedMemorySize = D3D12_CS_THREAD_LOCAL_TEMP_REGISTER_POOL;
        _limits.maxComputeWorkGroupCountX = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        _limits.maxComputeWorkGroupCountY = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        _limits.maxComputeWorkGroupCountZ = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
        _limits.maxComputeWorkGroupInvocations = D3D12_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
        _limits.maxComputeWorkGroupSizeX = D3D12_CS_THREAD_GROUP_MAX_X;
        _limits.maxComputeWorkGroupSizeY = D3D12_CS_THREAD_GROUP_MAX_Y;
        _limits.maxComputeWorkGroupSizeZ = D3D12_CS_THREAD_GROUP_MAX_Z;

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

    void D3D12GPUDevice::WaitForIdle()
    {
    }

    SwapChain* D3D12GPUDevice::CreateSwapChainCore(const SwapChainDescriptor* descriptor)
    {
        return nullptr;
    }

#if TODO

    void D3D12GPUDevice::ExecuteDeferredReleases()
    {
        uint64_t gpuValue = frameFence.handle->GetCompletedValue();
        while (deferredReleases.size() &&
            deferredReleases.front().frameIndex <= gpuValue)
        {
            deferredReleases.front().handle->Release();
            deferredReleases.pop();
        }
    }

    void D3D12GPUDevice::DeferredRelease_(IUnknown* resource, bool forceDeferred)
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
#endif // TODO

}
