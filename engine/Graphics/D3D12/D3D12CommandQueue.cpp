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
#include "D3D12CommandQueue.h"
#include "D3D12CommandBuffer.h"
#include "D3D12GraphicsDevice.h"

namespace alimer
{
    D3D12CommandQueue::D3D12CommandQueue(D3D12GraphicsDevice* device, CommandQueueType queueType, const char* name)
        : CommandQueue(device, queueType)
        , fenceValue(0)
        , processCommandBuffers(true)
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = GetD3D12CommandListType(queueType);
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.NodeMask = 0;

        VHR(device->GetD3DDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&handle)));
        VHR(device->GetD3DDevice()->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

        if (!name)
        {
            switch (queueDesc.Type)
            {
            case D3D12_COMMAND_LIST_TYPE_COPY:
                handle->SetName(L"Copy Command Queue");
                break;
            case D3D12_COMMAND_LIST_TYPE_COMPUTE:
                handle->SetName(L"Compute Command Queue");
                break;
            case D3D12_COMMAND_LIST_TYPE_DIRECT:
                handle->SetName(L"Direct Command Queue");
                break;
            }
        }

        processCommandBuffersThread = std::thread(&D3D12CommandQueue::ProccessCommandBuffers, this);
    }

    D3D12CommandQueue::~D3D12CommandQueue()
    {
        processCommandBuffers = false;
        processCommandBuffersThread.join();

        SAFE_RELEASE(fence);
        SAFE_RELEASE(handle);
    }

    void D3D12CommandQueue::WaitIdle()
    {
        std::unique_lock<std::mutex> lock(processCommandBuffersThreadMutex);
        processCommandBuffersThreadCV.wait(lock, [this] { return inFlightCommandBuffers.empty(); });

        WaitForFenceValue(fenceValue);
    }

    uint64_t D3D12CommandQueue::Signal()
    {
        uint64_t signalFenceValue = ++fenceValue;
        handle->Signal(fence, signalFenceValue);
        return signalFenceValue;
    }

    bool D3D12CommandQueue::IsFenceComplete(uint64_t fenceValue)
    {
        return fence->GetCompletedValue() >= fenceValue;
    }

    void D3D12CommandQueue::WaitForFenceValue(uint64_t fenceValue)
    {
        if (IsFenceComplete(fenceValue))
            return;

        HANDLE event = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        ALIMER_ASSERT_MSG(event, "Failed to create fence event handle.");

        fence->SetEventOnCompletion(fenceValue, event);
        WaitForSingleObject(event, INFINITE);

        ::CloseHandle(event);
    }

    void D3D12CommandQueue::ProccessCommandBuffers()
    {
        std::unique_lock<std::mutex> lock(processCommandBuffersThreadMutex, std::defer_lock);

        while (processCommandBuffers)
        {
        }
    }

    RefPtr<CommandBuffer> D3D12CommandQueue::GetCommandBuffer()
    {
        RefPtr<D3D12CommandBuffer> commandBuffer;

        {
            std::lock_guard<std::mutex> lock(availableCommandBuffersMutex);

            // If there is a command list on the queue.
            if (!availableCommandBuffers.empty())
            {
                commandBuffer = availableCommandBuffers.front();
                availableCommandBuffers.pop();
            }
            else
            {
                // Otherwise create a new command buffer.
                commandBuffer = new D3D12CommandBuffer(this);
            }
        }

        return commandBuffer;
    }
}


#endif // TODO
