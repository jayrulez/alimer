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
#include "D3D12GraphicsContext.h"
#include "D3D12Texture.h"

namespace alimer
{
    namespace
    {
        D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE D3D12BeginningAccessType(LoadAction action) {
            switch (action) {
            case LoadAction::Clear:
                return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
            case LoadAction::Load:
                return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
            default:
                ALIMER_UNREACHABLE();
            }
        }

        D3D12_RENDER_PASS_ENDING_ACCESS_TYPE D3D12EndingAccessType(StoreAction action) {
            switch (action) {
            case StoreAction::Store:
                return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
            case StoreAction::Clear:
                return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;

            default:
                ALIMER_UNREACHABLE();
            }
        }
    }

    D3D12GraphicsContext::D3D12GraphicsContext(D3D12GraphicsDevice* device, const GraphicsContextDescription& desc)
        : GraphicsContext(*device, desc)
        , device(device)
        , useRenderPass(device->SupportsRenderPass())
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        VHR(device->GetD3DDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));
        if (desc.label)
        {
            //commandQueue->SetName(L"Main Gfx Queue");
        }

        // Create command allocator.
        for (u64 i = 0; i < kRenderLatency; ++i)
        {
            VHR(device->GetD3DDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[i])));
        }

        // Create command list.
        VHR(
            device->GetD3DDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[0], nullptr, IID_PPV_ARGS(&commandList))
        );
        commandList->Close();
        if (FAILED(commandList->QueryInterface(&commandList4))) {
            useRenderPass = false;
        }
        //useRenderPass = true;

        VHR(device->GetD3DDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
        // Create an event handle to use for frame synchronization.
        fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        if (fenceEvent == INVALID_HANDLE_VALUE)
        {
            LOG_ERROR("CreateEventEx failed");
        }

        if (desc.handle)
        {
            // Flip mode doesn't support SRGB formats
            dxgiColorFormat = ToDXGIFormat(srgbToLinearFormat(desc.colorFormat));

            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
            swapChainDesc.Width = desc.width;
            swapChainDesc.Height = desc.height;
            swapChainDesc.Format = dxgiColorFormat;
            swapChainDesc.Stereo = FALSE;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = kNumBackBuffers;
            swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
            if (device->GetProvider()->IsTearingSupported())
            {
                swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
            }

            DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
            fsSwapChainDesc.Windowed = !desc.isFullscreen;

            IDXGISwapChain1* tempSwapChain;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            HWND hwnd = (HWND)desc.handle;
            if (!IsWindow(hwnd)) {
                LOG_ERROR("Invalid HWND handle");
                return;
            }


            VHR(device->GetProvider()->GetDXGIFactory()->CreateSwapChainForHwnd(
                commandQueue,
                hwnd,
                &swapChainDesc,
                &fsSwapChainDesc,
                NULL,
                &tempSwapChain
            ));

            // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
            VHR(device->GetProvider()->GetDXGIFactory()->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
#else
            ThrowIfFailed(device->GetProvider()->GetDXGIFactory()->CreateSwapChainForCoreWindow(
                commandQueue,
                (IUnknown*)window,
                &swapChainDesc,
                nullptr,
                &tempSwapChain
            ));
#endif

            VHR(tempSwapChain->QueryInterface(IID_PPV_ARGS(&swapChain)));
            tempSwapChain->Release();

            CreateRenderTargets();
        }
        else
        {
            dxgiColorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        }
    }

    D3D12GraphicsContext::~D3D12GraphicsContext()
    {
        Destroy();
    }

    void D3D12GraphicsContext::Destroy()
    {
        if (swapChain == nullptr) {
            return;
        }

        // Wait for GPU to catch up.
        WaitForGPU();
        const u64 currentGPUFrame = fence->GetCompletedValue();
        ALIMER_ASSERT(currentCPUFrame == currentGPUFrame);

        CloseHandle(fenceEvent);
        SAFE_RELEASE(fence);
        for (u64 i = 0; i < kRenderLatency; ++i)
        {
            SAFE_RELEASE(commandAllocators[i]);
        }

        SAFE_RELEASE(commandList4);
        SAFE_RELEASE(commandList);
        SAFE_RELEASE(commandQueue);

        for (uint32_t i = 0; i < kNumBackBuffers; ++i)
        {
            SafeDelete(colorTextures[i]);
        }

        SAFE_RELEASE(swapChain);
    }

    void D3D12GraphicsContext::WaitForGPU()
    {
        commandQueue->Signal(fence, ++currentCPUFrame);
        fence->SetEventOnCompletion(currentCPUFrame, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);

        //GPUDescriptorHeaps[frameIndex].Size = 0;
        //GPUUploadMemoryHeaps[frameIndex].Size = 0;
    }

    void D3D12GraphicsContext::Begin(const char* name, bool profile)
    {
        /*if (base.State == CommandBufferState.Recording)
        {
            context.ValidationLayer ? .Notify("DX12", "Begin cannot be called again until End has been successfully called");
        }*/

        ALIMER_ASSERT_MSG(!frameActive, "Frame is still active, please call EndFrame");

        commandAllocators[frameIndex]->Reset();
        commandList->Reset(commandAllocators[frameIndex], nullptr);
        //commandList->SetDescriptorHeaps(1, &GPUDescriptorHeaps[frameIndex].Heap);

        // Set frame as active.
        frameActive = true;

        //return true;
    }

    void D3D12GraphicsContext::End()
    {

    }

    void D3D12GraphicsContext::Flush(bool wait)
    {
        ALIMER_ASSERT_MSG(frameActive, "Frame is not active, please call BeginFrame first.");

        //if (swapchainTexture != nullptr)
        //{
        //    TransitionResource(swapchainTexture, D3D12_RESOURCE_STATE_PRESENT, false);
        //}
        FlushResourceBarriers();

        VHR(commandList->Close());

        /* TODO: Add compute and copy upload support .*/
        //CompleteUpload();

        ID3D12CommandList* commandLists[] = { commandList };
        commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

        if (swapChain != nullptr)
        {
            HRESULT hr = swapChain->Present(syncInterval, presentFlags);

            // If the device was reset we must completely reinitialize the renderer.
            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
                device->HandleDeviceLost();
            }

            VHR(hr);
            backbufferIndex = swapChain->GetCurrentBackBufferIndex();
        }

        if (wait)
        {
            WaitForGPU();
        }
        else
        {
            // Signal the fence with the current frame number, so that we can check back on it
            commandQueue->Signal(fence, ++currentCPUFrame);

            uint64_t GPUFrameCount = fence->GetCompletedValue();

            if ((currentCPUFrame - GPUFrameCount) >= kRenderLatency)
            {
                fence->SetEventOnCompletion(GPUFrameCount + 1, fenceEvent);
                WaitForSingleObject(fenceEvent, INFINITE);
            }
        }

        frameIndex = currentCPUFrame % kRenderLatency;

        // See if we have any deferred releases to process
        //ProcessDeferredReleases(CurrFrameIdx);

        //GPUDescriptorHeaps[frameIndex].Size = 0;
        //GPUUploadMemoryHeaps[frameIndex].Size = 0;

        // Frame is not active anymore
        frameActive = false;
    }

    Texture* D3D12GraphicsContext::GetCurrentColorTexture() const
    {
        return colorTextures[backbufferIndex];
    }

    void D3D12GraphicsContext::BeginRenderPass(const RenderPassDescriptor* descriptor)
    {
        if (useRenderPass)
        {
            u32 colorRTVSCount = 0;
            for (u32 i = 0; i < kMaxColorAttachments; i++)
            {
                const RenderPassColorAttachmentDescriptor& attachment = descriptor->colorAttachments[i];
                if (attachment.texture == nullptr)
                    continue;

                D3D12Texture* texture = static_cast<D3D12Texture*>(attachment.texture);
                TransitionResource(texture, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
                colorRenderPassTargets[colorRTVSCount].cpuDescriptor = texture->GetRenderTargetView(attachment.mipLevel, attachment.slice);
                colorRenderPassTargets[colorRTVSCount].BeginningAccess.Type = D3D12BeginningAccessType(attachment.loadAction);
                if (attachment.loadAction == LoadAction::Clear) {
                    colorRenderPassTargets[colorRTVSCount].BeginningAccess.Clear.ClearValue.Color[0] = attachment.clearColor.r;
                    colorRenderPassTargets[colorRTVSCount].BeginningAccess.Clear.ClearValue.Color[1] = attachment.clearColor.g;
                    colorRenderPassTargets[colorRTVSCount].BeginningAccess.Clear.ClearValue.Color[2] = attachment.clearColor.b;
                    colorRenderPassTargets[colorRTVSCount].BeginningAccess.Clear.ClearValue.Color[3] = attachment.clearColor.a;
                    colorRenderPassTargets[colorRTVSCount].BeginningAccess.Clear.ClearValue.Format = texture->GetDXGIFormat();
                }

                colorRenderPassTargets[colorRTVSCount].EndingAccess.Type = D3D12EndingAccessType(attachment.storeOp);
                colorRTVSCount++;
            }

            D3D12_RENDER_PASS_FLAGS renderPassFlags = D3D12_RENDER_PASS_FLAG_NONE;
            commandList4->BeginRenderPass(colorRTVSCount, colorRenderPassTargets, nullptr, renderPassFlags);
        }
        else
        {
            u32 colorRTVSCount = 0;
            for (u32 i = 0; i < kMaxColorAttachments; i++)
            {
                const RenderPassColorAttachmentDescriptor& attachment = descriptor->colorAttachments[i];
                if (attachment.texture == nullptr)
                    continue;

                D3D12Texture* texture = static_cast<D3D12Texture*>(attachment.texture);
                TransitionResource(texture, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
                colorRTVS[colorRTVSCount] = texture->GetRenderTargetView(attachment.mipLevel, attachment.slice);
                if (attachment.loadAction == LoadAction::Clear)
                {
                    commandList->ClearRenderTargetView(colorRTVS[colorRTVSCount], &attachment.clearColor.r, 0, nullptr);
                }

                //swapchainTexture = texture;
                colorRTVSCount++;
            }

            commandList->OMSetRenderTargets(colorRTVSCount, colorRTVS, FALSE, NULL);
        }

        // Set up default dynamic state
        {
            /*uint32_t width = renderPass->width;
            uint32_t height = renderPass->height;
            D3D12_VIEWPORT viewport = {
                0.f, 0.f, static_cast<float>(width), static_cast<float>(height), 0.f, 1.f };
            D3D12_RECT scissorRect = { 0, 0, static_cast<long>(width), static_cast<long>(height) };
            commandList->RSSetViewports(1, &viewport);
            commandList->RSSetScissorRects(1, &scissorRect);*/

            Color defaultBlendColor = { 0, 0, 0, 0 };
            SetBlendColor(defaultBlendColor);
        }
    }

    void D3D12GraphicsContext::EndRenderPass()
    {
        if (useRenderPass)
        {
            commandList4->EndRenderPass();
        }
    }

    void D3D12GraphicsContext::SetBlendColor(const Color& color)
    {
        commandList->OMSetBlendFactor(&color.r);
    }

    void D3D12GraphicsContext::TransitionResource(D3D12GpuResource* resource, D3D12_RESOURCE_STATES newState, bool flushImmediate)
    {
        D3D12_RESOURCE_STATES currentState = resource->GetState();

        /*if (type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
        {
            ALIMER_ASSERT((currentState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == currentState);
            ALIMER_ASSERT((newState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == newState);
        }*/

        if (currentState != newState)
        {
            ALIMER_ASSERT_MSG(numBarriersToFlush < kMaxResourceBarriers, "Exceeded arbitrary limit on buffered barriers");
            D3D12_RESOURCE_BARRIER& barrierDesc = resourceBarriers[numBarriersToFlush++];

            barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrierDesc.Transition.pResource = resource->GetResource();
            barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrierDesc.Transition.StateBefore = currentState;
            barrierDesc.Transition.StateAfter = newState;

            // Check to see if we already started the transition
            if (newState == resource->GetTransitioningState())
            {
                barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
                resource->SetTransitioningState((D3D12_RESOURCE_STATES)-1);
            }
            else
            {
                barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            }

            resource->SetState(newState);
        }
        else if (newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        {
            InsertUAVBarrier(resource, flushImmediate);
        }

        if (flushImmediate || numBarriersToFlush == kMaxResourceBarriers)
            FlushResourceBarriers();
    }

    void D3D12GraphicsContext::InsertUAVBarrier(D3D12GpuResource* resource, bool flushImmediate)
    {
        ALIMER_ASSERT_MSG(numBarriersToFlush < kMaxResourceBarriers, "Exceeded arbitrary limit on buffered barriers");
        D3D12_RESOURCE_BARRIER& barrierDesc = resourceBarriers[numBarriersToFlush++];
        barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrierDesc.UAV.pResource = resource->GetResource();

        if (flushImmediate)
            FlushResourceBarriers();
    }

    void D3D12GraphicsContext::FlushResourceBarriers(void)
    {
        if (numBarriersToFlush > 0)
        {
            commandList->ResourceBarrier(numBarriersToFlush, resourceBarriers);
            numBarriersToFlush = 0;
        }
    }

    void D3D12GraphicsContext::CreateRenderTargets()
    {
        for (uint32_t index = 0; index < kNumBackBuffers; ++index)
        {
            ID3D12Resource* backbuffer;
            VHR(swapChain->GetBuffer(index, IID_PPV_ARGS(&backbuffer)));
            colorTextures[index] = D3D12Texture::CreateFromExternal(device, backbuffer, colorFormat, D3D12_RESOURCE_STATE_PRESENT);
        }

        backbufferIndex = swapChain->GetCurrentBackBufferIndex();
    }
}

