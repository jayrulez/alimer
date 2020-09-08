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

    D3D12CommandAllocatorPool::D3D12CommandAllocatorPool(D3D12GraphicsDevice* device_, D3D12_COMMAND_LIST_TYPE type_)
        : device(device_)
        , type(type_)
    {
    }

    D3D12CommandAllocatorPool::~D3D12CommandAllocatorPool()
    {
        Shutdown();
    }

    void D3D12CommandAllocatorPool::Shutdown()
    {
        for (size_t i = 0; i < allocatorPool.size(); ++i)
            allocatorPool[i]->Release();

        allocatorPool.clear();
    }

    ID3D12CommandAllocator* D3D12CommandAllocatorPool::RequestAllocator(uint64_t completedFenceValue)
    {
        std::lock_guard<std::mutex> LockGuard(allocatorMutex);

        ID3D12CommandAllocator* allocator = nullptr;

        if (!readyAllocators.empty())
        {
            std::pair<uint64_t, ID3D12CommandAllocator*>& allocatorPair = readyAllocators.front();

            if (allocatorPair.first <= completedFenceValue)
            {
                allocator = allocatorPair.second;
                ThrowIfFailed(allocator->Reset());
                readyAllocators.pop();
            }
        }

        // If no allocator's were ready to be reused, create a new one
        if (allocator == nullptr)
        {
            ThrowIfFailed(device->GetD3DDevice()->CreateCommandAllocator(type, IID_PPV_ARGS(&allocator)));
#ifdef _DEBUG
            wchar_t AllocatorName[32];
            swprintf(AllocatorName, 32, L"CommandAllocator %zu", allocatorPool.size());
            LOGD("Direct3D12: Created new CommandAllocator: {}", allocatorPool.size());
            allocator->SetName(AllocatorName);
#endif
            allocatorPool.push_back(allocator);
        }

        return allocator;
    }

    void D3D12CommandAllocatorPool::DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* allocator)
    {
        std::lock_guard<std::mutex> LockGuard(allocatorMutex);

        // That fence value indicates we are free to reset the allocator
        readyAllocators.push(std::make_pair(fenceValue, allocator));
    }

    D3D12CommandQueue::D3D12CommandQueue(D3D12GraphicsDevice* device, CommandQueueType queueType)
        : CommandQueue(queueType)
        , device{ device }
        , type(D3D12CommandListType(queueType))
        , fenceValue(0)
        , allocatorPool(device, type)
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
    }

    D3D12CommandQueue::~D3D12CommandQueue()
    {
        allocatorPool.Shutdown();
    }

    GraphicsDevice* D3D12CommandQueue::GetGraphicsDevice() const
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
            // Otherwise create a new command list.
            commandBuffer = MakeRefPtr<D3D12CommandBuffer>(this);
        }

        return commandBuffer;
    }

    uint64_t D3D12CommandQueue::Signal()
    {
        uint64_t nextFenceValue = ++fenceValue;
        handle->Signal(fence.Get(), nextFenceValue);
        return nextFenceValue;
    }

    bool D3D12CommandQueue::IsFenceComplete(uint64_t fenceValue)
    {
        return fence->GetCompletedValue() >= fenceValue;
    }

    void D3D12CommandQueue::WaitForFenceValue(uint64_t fenceValue)
    {
        if (IsFenceComplete(fenceValue))
            return;

        LOGD("Waiting on fence value");

        HANDLE event = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
        ALIMER_ASSERT_MSG(event, "Failed to create fence event handle.");

        // Is this function thread safe?
        fence->SetEventOnCompletion(fenceValue, event);
        WaitForSingleObjectEx(event, INFINITE, FALSE);
        CloseHandle(event);
    }

    void D3D12CommandQueue::WaitIdle()
    {
        WaitForFenceValue(fenceValue);
    }

    uint64_t D3D12CommandQueue::ExecuteCommandList(ID3D12GraphicsCommandList* commandList)
    {
        // Kickoff the command list.
        ID3D12CommandList* commandLists[] = { commandList };
        handle->ExecuteCommandLists(1, commandLists);
        uint64_t fenceValue = Signal();

        return fenceValue;
    }

    ID3D12CommandAllocator* D3D12CommandQueue::RequestAllocator()
    {
        uint64_t completedFenceValue = fence->GetCompletedValue();
        return allocatorPool.RequestAllocator(completedFenceValue);
    }

    void D3D12CommandQueue::DiscardAllocator(uint64_t fenceValueForReset, ID3D12CommandAllocator* commandAllocator)
    {
        allocatorPool.DiscardAllocator(fenceValueForReset, commandAllocator);
    }
}

