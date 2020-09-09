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

#include "D3D12CommandQueue.h"
#include "D3D12CommandBuffer.h"
#include "D3D12GraphicsDevice.h"

namespace Alimer
{
    namespace
    {
        inline D3D12_COMMAND_LIST_TYPE D3D12CommandListType(CommandQueueType type)
        {
            switch (type)
            {
            case CommandQueueType::Graphics:
                return D3D12_COMMAND_LIST_TYPE_DIRECT;

            case CommandQueueType::Compute:
                return D3D12_COMMAND_LIST_TYPE_COMPUTE;

            case CommandQueueType::Copy:
                return D3D12_COMMAND_LIST_TYPE_COPY;

            default:
                ALIMER_UNREACHABLE();
                return D3D12_COMMAND_LIST_TYPE_DIRECT;
            }
        }
    }

    D3D12CommandQueue::D3D12CommandQueue(D3D12GraphicsDevice* device, CommandQueueType queueType)
        : CommandQueue(queueType)
        , device{ device }
        , type(D3D12CommandListType(queueType))
        , _fenceValue(0)
        , processInflights(true)
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = type;
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.NodeMask = 0;

        ThrowIfFailed(device->GetD3DDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&handle)));
        ThrowIfFailed(device->GetD3DDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

        switch (queueType)
        {
        case CommandQueueType::Graphics:
            handle->SetName(L"Graphics Command Queue");
            fence->SetName(L"Graphics Command Queue Fence");
            break;
        case CommandQueueType::Compute:
            handle->SetName(L"Compute Command Queue");
            fence->SetName(L"Compute Command Queue Fence");
            break;
        case CommandQueueType::Copy:
            handle->SetName(L"Copy Command Queue");
            fence->SetName(L"Copy Command Queue Fence");
            break;
        }

        processInFlightsThread = std::thread(&D3D12CommandQueue::ProccessInflightCommandBuffers, this);
    }

    D3D12CommandQueue::~D3D12CommandQueue()
    {
        processInflights = false;
        processInFlightsThread.join();
    }

    D3D12GraphicsDevice* D3D12CommandQueue::GetDevice() const
    {
        return device;
    }

    RefPtr<CommandBuffer> D3D12CommandQueue::GetCommandBuffer()
    {
        RefPtr<D3D12CommandBuffer> commandBuffer;

        // If there is a completed command buffer on the queue, return it.
        if (!availableCommandBuffers.Empty())
        {
            availableCommandBuffers.TryPop(commandBuffer);
        }
        else
        {
            // Otherwise create a new command buffer.
            commandBuffer = MakeRefPtr<D3D12CommandBuffer>(this);
        }

        return commandBuffer;
    }

    uint64_t D3D12CommandQueue::Signal()
    {
        uint64_t fenceValue = ++_fenceValue;
        handle->Signal(fence.Get(), fenceValue);
        return fenceValue;
    }

    bool D3D12CommandQueue::IsFenceComplete(uint64_t fenceValue)
    {
        return fence->GetCompletedValue() >= fenceValue;
    }

    void D3D12CommandQueue::WaitForFenceValue(uint64_t fenceValue)
    {
        if (IsFenceComplete(fenceValue))
            return;
        

        HANDLE event = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
        ALIMER_ASSERT_MSG(event, "Failed to create fence event handle.");

        // Is this function thread safe?
        fence->SetEventOnCompletion(fenceValue, event);
        WaitForSingleObjectEx(event, INFINITE, FALSE);
        CloseHandle(event);
    }

    void D3D12CommandQueue::WaitIdle()
    {
        std::unique_lock<std::mutex> lock(processInFlightCommandsThreadMutex);
        processInFlightCommandsThreadCV.wait(lock, [this] { return inflightCommandBuffers.Empty(); });

        WaitForFenceValue(_fenceValue);
    }

    void D3D12CommandQueue::ExecuteCommandBuffer(const RefPtr<CommandBuffer>& commandBuffer, bool waitForCompletion)
    {
        auto d3d12CommandBuffer = StaticCast<D3D12CommandBuffer>(commandBuffer);

        ThrowIfFailed( d3d12CommandBuffer->GetCommandList()->Close());
        ID3D12CommandList* commandList = d3d12CommandBuffer->GetCommandList();
        handle->ExecuteCommandLists(1, &commandList);
        uint64_t fenceValue = Signal();

        // Queue command lists for reuse.
        inflightCommandBuffers.Push({ fenceValue, d3d12CommandBuffer });

        if (waitForCompletion)
        {
            WaitForFenceValue(fenceValue);
        }
    }

    void D3D12CommandQueue::ExecuteCommandBuffers(const std::vector<RefPtr<CommandBuffer>> commandBuffers, bool waitForCompletion)
    {

    }

    void D3D12CommandQueue::ProccessInflightCommandBuffers()
    {
        std::unique_lock<std::mutex> lock(processInFlightCommandsThreadMutex, std::defer_lock);

        while (processInflights)
        {
            CommandBufferEntry entry;

            lock.lock();
            while (inflightCommandBuffers.TryPop(entry))
            {
                uint64_t fenceValue = std::get<0>(entry);
                auto commandBuffer = std::get<1>(entry);

                WaitForFenceValue(fenceValue);

                commandBuffer->Reset();

                availableCommandBuffers.Push(commandBuffer);
            }
            lock.unlock();
            processInFlightCommandsThreadCV.notify_one();

            std::this_thread::yield();
        }
    }
}

