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
#include "D3D12CommandQueue.h"
#include "D3D12GraphicsDevice.h"
#include <algorithm>

namespace alimer
{
    D3D12GraphicsContext::D3D12GraphicsContext(D3D12GraphicsDevice& device_, QueueType type_)
        : device(device_)
        , type(type_)
    {
        const D3D12_COMMAND_LIST_TYPE& commandListType = D3D12GetCommandListType(type);

        currentAllocator = device_.GetQueue(type).RequestAllocator();
        ThrowIfFailed(device_.GetD3DDevice()->CreateCommandList(1, commandListType, currentAllocator, nullptr, IID_PPV_ARGS(&commandList)));
    }

    void D3D12GraphicsContext::Destroy()
    {
        if (commandList != nullptr)
        {
            commandList->Release();
            commandList = nullptr;
        }
    }

    void D3D12GraphicsContext::Reset()
    {
        // We only call Reset() on previously freed contexts.
        // The command list persists, but we must request a new allocator.
        ALIMER_ASSERT(commandList != nullptr && currentAllocator == nullptr);
        currentAllocator = device.GetQueue(type).RequestAllocator();
        commandList->Reset(currentAllocator, nullptr);

       /* m_CurGraphicsRootSignature = nullptr;
        m_CurPipelineState = nullptr;
        m_CurComputeRootSignature = nullptr;*/
        numBarriersToFlush = 0;

        //BindDescriptorHeaps();
    }

    void D3D12GraphicsContext::BeginMarker(const char* name)
    {
    }

    void D3D12GraphicsContext::EndMarker()
    {
    }

    void D3D12GraphicsContext::Flush(bool wait)
    {
        FlushResourceBarriers();

        ALIMER_ASSERT(currentAllocator != nullptr);

        commandList->Close();
        uint64_t fenceValue = device.GetQueue(type).ExecuteCommandList(commandList);

        if (wait)
        {
            device.WaitForFence(fenceValue);
        }

        // Reset the command list and restore previous state
        commandList->Reset(currentAllocator, nullptr);
    }

    void D3D12GraphicsContext::TransitionResource(d3d12::Resource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate)
    {
        D3D12_RESOURCE_STATES oldState = resource.state;

        if (type == QueueType::Compute)
        {
            ALIMER_ASSERT((oldState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == oldState);
            ALIMER_ASSERT((newState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == newState);
        }

        if (oldState != newState)
        {
            ALIMER_ASSERT_MSG(numBarriersToFlush < kMaxResourceBarriers, "Exceeded arbitrary limit on buffered barriers");
            D3D12_RESOURCE_BARRIER& barrierDesc = resourceBarriers[numBarriersToFlush++];

            barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrierDesc.Transition.pResource = resource.handle;
            barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrierDesc.Transition.StateBefore = oldState;
            barrierDesc.Transition.StateAfter = newState;

            // Check to see if we already started the transition
            if (newState == resource.transitioning_state)
            {
                barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
                resource.transitioning_state = (D3D12_RESOURCE_STATES)-1;
            }
            else
                barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

            resource.state = newState;
        }
        else if (newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        {
            //InsertUAVBarrier(Resource, FlushImmediate);
        }

        if (flushImmediate || numBarriersToFlush == kMaxResourceBarriers)
        {
            FlushResourceBarriers();
        }
    }

    void D3D12GraphicsContext::FlushResourceBarriers(void)
    {
        if (numBarriersToFlush > 0)
        {
            commandList->ResourceBarrier(numBarriersToFlush, resourceBarriers);
            numBarriersToFlush = 0;
        }
    }

    void D3D12GraphicsContext::BeginRenderPass(GPUTexture texture, const Color& clearColor)
    {
        auto d3dTexture = device.GetTexture(texture);
        TransitionResource(*d3dTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

        commandList->OMSetRenderTargets(1, &d3dTexture->rtv, FALSE, nullptr);
        commandList->ClearRenderTargetView(d3dTexture->rtv, clearColor.Data(), 0, nullptr);
        //commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        TransitionResource(*d3dTexture, D3D12_RESOURCE_STATE_COMMON, true);
    }

    void D3D12GraphicsContext::EndRenderPass()
    {

    }
}
