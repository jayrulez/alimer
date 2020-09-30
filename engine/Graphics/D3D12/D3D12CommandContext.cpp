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

#if TODO
#include "D3D12CommandContext.h"
#include "D3D12CommandQueue.h"
#include "D3D12GraphicsDevice.h"
#include "D3D12Texture.h"

namespace Alimer
{
    namespace
    {
        constexpr D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE D3D12BeginningAccessType(LoadAction action) {
            switch (action) {
            case LoadAction::Clear:
                return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
            case LoadAction::Load:
                return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
            default:
                ALIMER_UNREACHABLE();
            }
        }

        /*D3D12_RENDER_PASS_ENDING_ACCESS_TYPE D3D12EndingAccessType(StoreAction action) {
            switch (action) {
            case StoreAction::Store:
                return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
            case StoreAction::Clear:
                return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;

            default:
                ALIMER_UNREACHABLE();
            }
        }*/
    }

    D3D12CommandContext::D3D12CommandContext(D3D12GraphicsDevice* device, D3D12CommandQueue* queue)
        : device{ device }
        , queue{ queue }
    {
        currentAllocator = queue->RequestAllocator();
        ThrowIfFailed(device->GetD3DDevice()->CreateCommandList(0, queue->GetType(), currentAllocator, nullptr, IID_PPV_ARGS(&commandList)));
        useRenderPass = SUCCEEDED(commandList->QueryInterface(IID_PPV_ARGS(&commandList4)));
        useRenderPass = false;
    }

    D3D12CommandContext::~D3D12CommandContext()
    {
        SafeRelease(commandList);
        SafeRelease(commandList4);
    }

    void D3D12CommandContext::Reset()
    {
        ALIMER_ASSERT(commandList != nullptr && currentAllocator == nullptr);
        currentAllocator = queue->RequestAllocator();
        commandList->Reset(currentAllocator, nullptr);

        //currentGraphicsRootSignature = nullptr;
        //currentPipelineState = nullptr;
        //currentComputeRootSignature = nullptr;
        numBarriersToFlush = 0;

        //BindDescriptorHeaps();
    }

    void D3D12CommandContext::Flush(bool waitForCompletion)
    {
        FlushResourceBarriers();

        //if (m_ID.length() > 0)
        //    EngineProfiling::EndBlock(this);

        ALIMER_ASSERT(currentAllocator != nullptr);

        uint64_t fenceValue = queue->ExecuteCommandList(commandList);
        queue->DiscardAllocator(fenceValue, currentAllocator);
        currentAllocator = nullptr;

        if (waitForCompletion)
        {
            device->WaitForFence(fenceValue);
        }

        Reset();
    }

    void D3D12CommandContext::PushDebugGroup(const char* name)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, name);
    }

    void D3D12CommandContext::PopDebugGroup()
    {
        PIXEndEvent(commandList);
    }

    void D3D12CommandContext::InsertDebugMarker(const char* name)
    {
        PIXSetMarker(commandList, PIX_COLOR_DEFAULT, name);
    }

    void D3D12CommandContext::BeginRenderPass(const RenderPassDesc& renderPass)
    {
        if (useRenderPass)
        {
            /*uint32 colorRTVSCount = 0;
            for (uint32 i = 0; i < kMaxColorAttachments; i++)
            {
                const RenderPassColorAttachment& attachment = renderPass->colorAttachments[i];
                if (attachment.texture == nullptr)
                    continue;

                D3D12Texture* texture = static_cast<D3D12Texture*>(attachment.texture);
                texture->TransitionBarrier(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
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
            commandList4->BeginRenderPass(colorRTVSCount, colorRenderPassTargets, nullptr, renderPassFlags);*/
        }
        else
        {
            uint32 colorRTVSCount = 0;
            D3D12_CPU_DESCRIPTOR_HANDLE colorRTVS[kMaxColorAttachments] = {};

            for (uint32 i = 0; i < kMaxColorAttachments; i++)
            {
                const RenderPassColorAttachment& attachment = renderPass.colorAttachments[i];
                if (attachment.texture == nullptr)
                    continue;

                D3D12Texture* texture = static_cast<D3D12Texture*>(attachment.texture);
                TransitionResource(texture, TextureLayout::RenderTarget, true);
                colorRTVS[colorRTVSCount] = texture->GetRTV();

                switch (attachment.loadAction)
                {
                case LoadAction::DontCare:
                    commandList->DiscardResource(texture->GetResource(), nullptr);
                    break;

                case LoadAction::Clear:
                    commandList->ClearRenderTargetView(colorRTVS[colorRTVSCount], &attachment.clearColor.r, 0, nullptr);
                    break;

                default:
                    break;
                }

                colorRTVSCount++;
            }

            commandList->OMSetRenderTargets(colorRTVSCount, colorRTVS, FALSE, nullptr);
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

    void D3D12CommandContext::EndRenderPass()
    {
        if (useRenderPass)
        {
            commandList4->EndRenderPass();
        }
    }

    void D3D12CommandContext::SetScissorRect(const RectI& scissorRect)
    {
        D3D12_RECT d3dScissorRect;
        d3dScissorRect.left = LONG(scissorRect.x);
        d3dScissorRect.top = LONG(scissorRect.y);
        d3dScissorRect.right = LONG(scissorRect.x + scissorRect.width);
        d3dScissorRect.bottom = LONG(scissorRect.y + scissorRect.height);
        commandList->RSSetScissorRects(1, &d3dScissorRect);
    }

    void D3D12CommandContext::SetScissorRects(const RectI* scissorRects, uint32_t count)
    {
        count = Max<uint32_t>(count, D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);
        D3D12_RECT d3dScissorRects[kMaxViewportAndScissorRects];
        for (uint32_t i = 0; i < count; ++i)
        {
            d3dScissorRects[i].left = LONG(scissorRects[i].x);
            d3dScissorRects[i].top = LONG(scissorRects[i].y);
            d3dScissorRects[i].right = LONG(scissorRects[i].x + scissorRects[i].width);
            d3dScissorRects[i].bottom = LONG(scissorRects[i].y + scissorRects[i].height);
        }
        commandList->RSSetScissorRects(count, d3dScissorRects);
    }

    void D3D12CommandContext::SetViewport(const Viewport& viewport)
    {
        D3D12_VIEWPORT d3dViewport;
        d3dViewport.TopLeftX = viewport.x;
        d3dViewport.TopLeftY = viewport.y;
        d3dViewport.Width = viewport.width;
        d3dViewport.Height = viewport.height;
        d3dViewport.MinDepth = viewport.minDepth;
        d3dViewport.MaxDepth = viewport.maxDepth;
        commandList->RSSetViewports(1, &d3dViewport);
    }

    void D3D12CommandContext::SetViewports(const Viewport* viewports, uint32_t count)
    {
        count = Max<uint32_t>(count, D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);

        D3D12_VIEWPORT d3dViewports[D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        for (uint32_t i = 0; i < count; ++i)
        {
            d3dViewports[i].TopLeftX = viewports[i].x;
            d3dViewports[i].TopLeftY = viewports[i].y;
            d3dViewports[i].Width = viewports[i].width;
            d3dViewports[i].Height = viewports[i].height;
            d3dViewports[i].MinDepth = viewports[i].minDepth;
            d3dViewports[i].MaxDepth = viewports[i].maxDepth;
        }
        commandList->RSSetViewports(count, d3dViewports);
    }

    void D3D12CommandContext::SetBlendColor(const Color& color)
    {
        commandList->OMSetBlendFactor(&color.r);
    }

    void D3D12CommandContext::TransitionResource(D3D12Texture* resource, TextureLayout newLayout, bool flushImmediate)
    {
        TextureLayout currentLayout = resource->GetLayout();

        /*if (type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
        {
            ALIMER_ASSERT((currentState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == currentState);
            ALIMER_ASSERT((newState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == newState);
        }*/

        if (currentLayout != newLayout)
        {
            ALIMER_ASSERT_MSG(numBarriersToFlush < kMaxResourceBarriers, "Exceeded arbitrary limit on buffered barriers");
            D3D12_RESOURCE_BARRIER& barrierDesc = resourceBarriers[numBarriersToFlush++];

            barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrierDesc.Transition.pResource = resource->GetResource();
            barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrierDesc.Transition.StateBefore = D3D12ResourceState(currentLayout);
            barrierDesc.Transition.StateAfter = D3D12ResourceState(newLayout);

            resource->SetLayout(newLayout);
        }
        /*else if (newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        {
            InsertUAVBarrier(resource, flushImmediate);
        }*/

        if (flushImmediate || numBarriersToFlush == kMaxResourceBarriers)
            FlushResourceBarriers();
    }

    void D3D12CommandContext::InsertUAVBarrier(D3D12GpuResource* resource, bool flushImmediate)
    {
        ALIMER_ASSERT_MSG(numBarriersToFlush < kMaxResourceBarriers, "Exceeded arbitrary limit on buffered barriers");
        D3D12_RESOURCE_BARRIER& barrierDesc = resourceBarriers[numBarriersToFlush++];
        barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrierDesc.UAV.pResource = resource->GetResource();

        if (flushImmediate)
            FlushResourceBarriers();
    }

    void D3D12CommandContext::FlushResourceBarriers(void)
    {
        if (numBarriersToFlush > 0)
        {
            commandList->ResourceBarrier(numBarriersToFlush, resourceBarriers);
            numBarriersToFlush = 0;
        }
    }
}

#endif // TODO
