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
#include "Graphics/CommandQueue.h"
#include "D3D12GraphicsDevice.h"

namespace Alimer
{
    namespace
    {
        constexpr D3D12_COMMAND_LIST_TYPE GetD3D12CommandListType(CommandQueueType type)
        {
            switch (type)
            {
            case CommandQueueType::Compute:
                return D3D12_COMMAND_LIST_TYPE_COMPUTE;
            case CommandQueueType::Copy:
                return D3D12_COMMAND_LIST_TYPE_COPY;
            default:
                return D3D12_COMMAND_LIST_TYPE_DIRECT;
            }
        }
    }

    struct CommandQueueApiData
    {
        ID3D12CommandQueue* handle;
        ID3D12Fence* fence;
        uint64 nextFenceValue;
        uint64 lastCompletedFenceValue;
        HANDLE fenceEvent;
        std::mutex fenceMutex;
        std::mutex eventMutex;
    };

    CommandQueue::CommandQueue(GraphicsDevice* device, CommandQueueType type)
        : device{ device }
        , type{ type }
    {
        auto d3dDevice = device->GetHandle();
        apiData = new CommandQueueApiData();

        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = GetD3D12CommandListType(type);
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.NodeMask = 0;
        ThrowIfFailed(d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&apiData->handle)));

        apiData->nextFenceValue = ((uint64)queueDesc.Type << 56 | 1);
        apiData->lastCompletedFenceValue = ((uint64)queueDesc.Type << 56);

        ThrowIfFailed(d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&apiData->fence)));
        apiData->fence->Signal((uint64_t)queueDesc.Type << 56);

        apiData->fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        ALIMER_ASSERT(apiData->fenceEvent != INVALID_HANDLE_VALUE);

        switch (type)
        {
        case CommandQueueType::Graphics:
            apiData->handle->SetName(L"Graphics Command Queue");
            apiData->fence->SetName(L"Graphics Command Queue Fence");
            break;
        case CommandQueueType::Compute:
            apiData->handle->SetName(L"Compute Command Queue");
            apiData->fence->SetName(L"Compute Command Queue pFence");
            break;
        case CommandQueueType::Copy:
            apiData->handle->SetName(L"Copy Command Queue");
            apiData->fence->SetName(L"Copy Command Queue  FFence");
            break;
        default:
            break;
        }
    }

    CommandQueue::~CommandQueue()
    {
        WaitIdle();
        CloseHandle(apiData->fenceEvent);
        SafeRelease(apiData->fence);
        SafeRelease(apiData->handle);
        SafeDelete(apiData);
    }

    void CommandQueue::WaitIdle()
    {
        WaitForFence(Signal());
    }

    CommandQueueHandle CommandQueue::GetHandle() const
    {
        return apiData->handle;
    }

    uint64 CommandQueue::Signal()
    {
        std::lock_guard<std::mutex> LockGuard(apiData->fenceMutex);
        ThrowIfFailed(apiData->handle->Signal(apiData->fence, apiData->nextFenceValue));
        return apiData->nextFenceValue++;
    }

    bool CommandQueue::IsFenceComplete(uint64 fenceValue)
    {
        // Avoid querying the fence value by testing against the last one seen.
        // The max() is to protect against an unlikely race condition that could cause the last
        // completed fence value to regress.
        if (fenceValue > apiData->lastCompletedFenceValue)
            apiData->lastCompletedFenceValue = Max(apiData->lastCompletedFenceValue, apiData->fence->GetCompletedValue());

        return fenceValue <= apiData->lastCompletedFenceValue;
    }

    void CommandQueue::WaitForFence(uint64 fenceValue)
    {
        if (IsFenceComplete(fenceValue))
            return;

        std::lock_guard<std::mutex> LockGuard(apiData->eventMutex);

        apiData->fence->SetEventOnCompletion(fenceValue, apiData->fenceEvent);
        WaitForSingleObject(apiData->fenceEvent, INFINITE);
        apiData->lastCompletedFenceValue = fenceValue;
    }

    void D3D12CommandQueue::StallForFence(uint64 fenceValue)
    {
        auto producer = device->GetQueue((D3D12_COMMAND_LIST_TYPE)(fenceValue >> 56));
        handle->Wait(producer->fence, fenceValue);
    }

    void D3D12CommandQueue::StallForProducer(D3D12CommandQueue& Producer)
    {
        ALIMER_ASSERT(Producer.nextFenceValue > 0);
        handle->Wait(Producer.fence, Producer.nextFenceValue - 1);
    }

    uint64 D3D12CommandQueue::ExecuteCommandList(ID3D12GraphicsCommandList* commandList)
    {
        std::lock_guard<std::mutex> LockGuard(fenceMutex);

        ThrowIfFailed(commandList->Close());

        // Kickoff the command list.
        ID3D12CommandList* commandLists[1] = { commandList };
        handle->ExecuteCommandLists(1, commandLists);

        // Signal the next fence value (with the GPU)
        ThrowIfFailed(handle->Signal(fence, nextFenceValue));

        // And increment the fence value.  
        return nextFenceValue++;
    }

    ID3D12CommandAllocator* D3D12CommandQueue::RequestAllocator()
    {
        uint64 completedFenceValue = fence->GetCompletedValue();
        return allocatorPool.RequestAllocator(completedFenceValue);
    }

    void D3D12CommandQueue::DiscardAllocator(uint64 fenceValue, ID3D12CommandAllocator* commandAllocator)
    {
        allocatorPool.DiscardAllocator(fenceValue, commandAllocator);
    }
}
#endif // TODO
