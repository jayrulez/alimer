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

#include "D3D12CommandBuffer.h"
#include "D3D12GraphicsDevice.h"
#include "D3D12Texture.h"

namespace Alimer
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

    D3D12CommandBuffer::D3D12CommandBuffer(D3D12CommandQueue* queue)
        : CommandBuffer(0)
        , queue{ queue }
    {
        /*auto d3dDevice = queue->GetDevice()->GetD3DDevice();
        ThrowIfFailed(d3dDevice->CreateCommandAllocator(queue->GetCommandListType(), IID_PPV_ARGS(&commandAllocator)));
        ThrowIfFailed(d3dDevice->CreateCommandList(0, queue->GetCommandListType(), commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
        commandList.As(&commandList4);*/

        LOGD("Direct3D12: Created Command Buffer");
    }

    D3D12CommandBuffer::~D3D12CommandBuffer()
    {
    }

    void D3D12CommandBuffer::Reset()
    {
        ThrowIfFailed(commandAllocator->Reset());
        ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));

        //currentGraphicsRootSignature = nullptr;
        //currentPipelineState = nullptr;
        //currentComputeRootSignature = nullptr;
        numBarriersToFlush = 0;

        //BindDescriptorHeaps();
    }

#if TODO

    

    void D3D12CommandContext::Commit(bool waitForCompletion)
    {
        FlushResourceBarriers();

        //if (m_ID.length() > 0)
        //    EngineProfiling::EndBlock(this);

        //device->CommitCommandBuffer(this, waitForCompletion);
    }

    void D3D12CommandContext::PushDebugGroup(const String& name)
    {
        auto wideName = ToUtf16(name);
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, wideName.c_str());
    }

    void D3D12CommandContext::PopDebugGroup()
    {

        commandList->EndEvent();
    }

    void D3D12CommandContext::InsertDebugMarker(const String& name)
    {
        auto wideName = ToUtf16(name);
        PIXSetMarker(commandList, PIX_COLOR_DEFAULT, wideName.c_str());
    }

    void D3D12CommandContext::BeginRenderPass(const RenderPassDescription& renderPass)
    {
    }

    void D3D12CommandContext::EndRenderPass()
    {

    }

    void D3D12CommandContext::SetScissorRect(uint32 x, uint32 y, uint32 width, uint32 height)
    {
        D3D12_RECT d3dScissorRect;
        d3dScissorRect.left = LONG(x);
        d3dScissorRect.top = LONG(y);
        d3dScissorRect.right = LONG(x + width);
        d3dScissorRect.bottom = LONG(y + height);
        commandList->RSSetScissorRects(1, &d3dScissorRect);
    }

    void D3D12CommandContext::SetScissorRects(const Rect* scissorRects, uint32_t count)
    {
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

    void D3D12CommandContext::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
    {
        D3D12_VIEWPORT viewport;
        viewport.TopLeftX = x;
        viewport.TopLeftY = y;
        viewport.Width = width;
        viewport.Height = height;
        viewport.MinDepth = minDepth;
        viewport.MaxDepth = maxDepth;
        commandList->RSSetViewports(1, &viewport);
    }

    void D3D12CommandContext::SetBlendColor(const Color& color)
    {
        commandList->OMSetBlendFactor(&color.r);
    }

    void D3D12CommandContext::BindBuffer(uint32_t slot, Buffer* buffer)
    {
    }

    void D3D12CommandContext::BindBufferData(uint32_t slot, const void* data, uint32_t size)
    {
    }

#ifdef TODO
    void D3D12CommandBuffer::BeginRenderPass(const RenderPassDescriptor* descriptor)
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

    void D3D12CommandBuffer::EndRenderPass()
    {
        if (useRenderPass)
        {
            commandList4->EndRenderPass();
        }
    }

    void D3D12CommandBuffer::SetBlendColor(const Color& color)
    {
        commandList->OMSetBlendFactor(&color.r);
}
#endif // TODO


    void D3D12CommandContext::TransitionResource(D3D12GpuResource* resource, D3D12_RESOURCE_STATES newState, bool flushImmediate)
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
#endif // TODO

}
