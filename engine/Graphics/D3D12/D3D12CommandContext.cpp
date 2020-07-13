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

#include "D3D12CommandContext.h"
#include "D3D12GraphicsImpl.h"
//#include "D3D12CommandQueue.h"
//#include "D3D12Texture.h"

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

    D3D12CommandContext::D3D12CommandContext(D3D12GraphicsImpl* device_)
        : device(device_)
    {
        /*const D3D12_COMMAND_LIST_TYPE commandListType = GetD3D12CommandListType(commandQueue->GetQueueType());

        VHR(device->GetD3DDevice()->CreateCommandAllocator(commandListType, IID_PPV_ARGS(&commandAllocator)));
        VHR(device->GetD3DDevice()->CreateCommandList(0, commandListType, commandAllocator, nullptr, IID_PPV_ARGS(&commandList)));

        useRenderPass = device->SupportsRenderPass();
        if (FAILED(commandList->QueryInterface(&commandList4))) {
            useRenderPass = false;
        }
        //useRenderPass = true;*/
    }

    D3D12CommandContext::~D3D12CommandContext()
    {
        //SAFE_RELEASE(commandAllocator);
        //SAFE_RELEASE(commandList4);
        //SAFE_RELEASE(commandList);
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
}
