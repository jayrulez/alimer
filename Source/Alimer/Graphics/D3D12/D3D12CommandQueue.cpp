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
#include "D3D12GraphicsImpl.h"

namespace alimer
{
    D3D12CommandAllocatorPool::D3D12CommandAllocatorPool(D3D12GraphicsImpl* device_, D3D12_COMMAND_LIST_TYPE type_)
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
            wchar_t AllocatorName[32];
            swprintf(AllocatorName, 32, L"CommandAllocator %zu", allocatorPool.size());
            allocator->SetName(AllocatorName);
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

    D3D12CommandQueue::D3D12CommandQueue(D3D12GraphicsImpl* device_, CommandQueueType queueType, const std::string_view& name)
        : CommandQueue(queueType)
        , device(device_)
        , type(GetD3D12CommandListType(queueType))
        , nextFenceValue((uint64_t)type << 56 | 1)
        , lastCompletedFenceValue((uint64_t)type << 56)
        , allocatorPool(device_, type)
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = type;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.NodeMask = 1;
        ThrowIfFailed(device->GetD3DDevice()->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue)));

        // Create fence and event
        ThrowIfFailed(device->GetD3DDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
        fence->Signal((uint64_t)type << 56);

        fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
        ALIMER_ASSERT(fenceEvent != 0);

        if (name.empty())
        {
            switch (queueType)
            {
            case CommandQueueType::Graphics:
                commandQueue->SetName(L"Graphics Command Queue");
                fence->SetName(L"Graphics Command Queue Fence");
                break;
            case CommandQueueType::Compute:
                commandQueue->SetName(L"Compute Command Queue");
                fence->SetName(L"Compute Command Queue Fence");
                break;
            case CommandQueueType::Copy:
                commandQueue->SetName(L"Copy Command Queue");
                fence->SetName(L"Copy Command Queue Fence");
                break;
            }
        }
    }

    D3D12CommandQueue::~D3D12CommandQueue()
    {
        allocatorPool.Shutdown();
        CloseHandle(fenceEvent);
        SafeRelease(fence);
        SafeRelease(commandQueue);
    }

    uint64_t D3D12CommandQueue::IncrementFence(void)
    {
        std::lock_guard<std::mutex> LockGuard(fenceMutex);
        commandQueue->Signal(fence, nextFenceValue);
        return nextFenceValue++;
    }

    bool D3D12CommandQueue::IsFenceComplete(uint64_t fenceValue)
    {
        // Avoid querying the fence value by testing against the last one seen.
        // The max() is to protect against an unlikely race condition that could cause the last
        // completed fence value to regress.
        if (fenceValue > lastCompletedFenceValue)
        {
            lastCompletedFenceValue = Max(lastCompletedFenceValue, fence->GetCompletedValue());
        }

        return fenceValue <= lastCompletedFenceValue;
    }

    void D3D12CommandQueue::WaitForFence(uint64_t fenceValue)
    {
        if (IsFenceComplete(fenceValue))
            return;

        // TODO:  Think about how this might affect a multi-threaded situation.  Suppose thread A
        // wants to wait for fence 100, then thread B comes along and wants to wait for 99.  If
        // the fence can only have one event set on completion, then thread B has to wait for 
        // 100 before it knows 99 is ready.  Maybe insert sequential events?
        {
            std::lock_guard<std::mutex> LockGuard(eventMutex);

            fence->SetEventOnCompletion(fenceValue, fenceEvent);
            WaitForSingleObject(fenceEvent, INFINITE);
            lastCompletedFenceValue = fenceValue;
        }
    }

    uint64_t D3D12CommandQueue::ExecuteCommandList(ID3D12GraphicsCommandList* commandList)
    {
        std::lock_guard<std::mutex> LockGuard(fenceMutex);

        ThrowIfFailed(commandList->Close());

        // Kickoff the command list.
        ID3D12CommandList* commandLists[] = { commandList };
        commandQueue->ExecuteCommandLists(1, commandLists);

        // Signal the next fence value (with the GPU)
        commandQueue->Signal(fence, nextFenceValue);

        // And increment the fence value.  
        return nextFenceValue++;
    }

    ID3D12CommandAllocator* D3D12CommandQueue::RequestAllocator(void)
    {
        uint64_t completedFenceValue = fence->GetCompletedValue();
        return allocatorPool.RequestAllocator(completedFenceValue);
    }

    GraphicsDevice* D3D12CommandQueue::GetDevice() const
    {
        return device;
    }
}
