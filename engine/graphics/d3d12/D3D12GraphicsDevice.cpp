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
#include "D3D12GraphicsAdapter.h"
#include "D3D12GraphicsProvider.h"
#include "D3D12CommandContext.h"
#include "D3D12SwapChain.h"
#include "D3D12Texture.h"
#include "D3D12GraphicsBuffer.h"
#include "core/String.h"
#include "D3D12MemAlloc.h"

namespace alimer
{
    D3D12GraphicsDevice::D3D12GraphicsDevice(D3D12GraphicsAdapter* adapter_, GraphicsSurface* surface_)
        : GraphicsDevice(adapter_, surface_)
        , graphicsQueue(this, D3D12_COMMAND_LIST_TYPE_DIRECT)
        , computeQueue(this, D3D12_COMMAND_LIST_TYPE_COMPUTE)
        , copyQueue(this, D3D12_COMMAND_LIST_TYPE_COPY)
        , frameFence(this)
        , RTVDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false)
        , DSVDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false)
    {
        CreateDeviceResources();
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

    void D3D12GraphicsDevice::CreateDeviceResources()
    {
        auto dxgiAdapter = static_cast<D3D12GraphicsAdapter*>(adapter)->GetDXGIAdapter();

        // Create the DX12 API device object.
        ThrowIfFailed(D3D12CreateDevice(
            dxgiAdapter,
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&d3dDevice)
            ));

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
        D3D12MA::ALLOCATOR_DESC desc = {};
        desc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
        desc.pDevice = d3dDevice;
        desc.pAdapter = dxgiAdapter;

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

        // Create command queue's and default context.
        {
            graphicsQueue.Create();
            computeQueue.Create();
            copyQueue.Create();
        }

        // Create the main swap chain
        auto dxgiFactory = static_cast<D3D12GraphicsProvider*>(adapter->GetProvider())->GetDXGIFactory();
        swapChain.reset(new D3D12SwapChain(this, dxgiFactory, surface, renderLatency));

        // Create the main context.
        mainContext.reset(new D3D12GraphicsContext(this, D3D12_COMMAND_LIST_TYPE_DIRECT, renderLatency));

        // Create frame fence.
        frameFence.Init(0);

        // Init descriptor heaps
        RTVDescriptorHeap.Init(256, 0);
        DSVDescriptorHeap.Init(256, 0);
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
        return true;
    }

    void D3D12GraphicsDevice::PresentFrame()
    {
        // Present the frame.
        if (swapChain)
        {
            swapChain->Present();
        }

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
}
