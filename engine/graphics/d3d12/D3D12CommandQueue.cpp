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

#include "D3D12Backend.h"
#include "graphics/CommandQueue.h"
#include "graphics/GPUDevice.h"
#include <algorithm>

namespace alimer
{
    static D3D12_COMMAND_LIST_TYPE GetD3D12CommandListType(CommandQueueType type) {
        static const D3D12_COMMAND_LIST_TYPE list_types[static_cast<uint32_t>(CommandQueueType::Count)] = {
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            D3D12_COMMAND_LIST_TYPE_COMPUTE,
            D3D12_COMMAND_LIST_TYPE_COPY,
        };

        return list_types[static_cast<uint32_t>(type)];
    }

    struct CommandQueueApiData {
        ID3D12CommandQueue* handle = nullptr;
        uint64_t nextFenceValue;
        uint64_t lastCompletedFenceValue;
        ID3D12Fence* fence = nullptr;
        HANDLE fenceEvent = INVALID_HANDLE_VALUE;
    };

    void CommandQueue::ApiInit()
    {
        CommandQueueApiData* pData = new CommandQueueApiData;
        apiData = pData;

        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = GetD3D12CommandListType(queueType);
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 0;

        pData->lastCompletedFenceValue = ((uint64_t)desc.Type << 56);
        pData->nextFenceValue = (uint64_t)desc.Type << 56 | 1;

        ThrowIfFailed(device.GetHandle()->CreateCommandQueue(&desc, IID_PPV_ARGS(&pData->handle)));
        ThrowIfFailed(device.GetHandle()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pData->fence)));
        pData->fence->Signal(pData->lastCompletedFenceValue);
        // Create an event handle to use for frame synchronization.
        pData->fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        if (pData->fenceEvent == INVALID_HANDLE_VALUE)
        {
            ALIMER_LOGERROR("CreateEventEx failed");
        }

        switch (queueType)
        {
        case CommandQueueType::Copy:
            pData->handle->SetName(L"COPY QUEUE");
            pData->fence->SetName(L"COPY QUEUE FENCE");
            break;
        case CommandQueueType::Compute:
            pData->handle->SetName(L"COMPUTE QUEUE");
            pData->fence->SetName(L"COMPUTE QUEUE FENCE");
            break;
        case CommandQueueType::Graphics:
            pData->handle->SetName(L"GRAPHICS QUEUE");
            pData->fence->SetName(L"GRAPHICS QUEUE FENCE");
            break;
        }
    }

    void CommandQueue::ApiDestroy()
    {
        if (apiData->handle == nullptr)
            return;

        CloseHandle(apiData->fenceEvent);
        SAFE_RELEASE(apiData->fence);
        SAFE_RELEASE(apiData->handle);

        SafeDelete(apiData);
    }

    CommandQueueHandle CommandQueue::GetHandle() const
    {
        return apiData->handle;
    }

#if TODO
    uint64_t D3D12CommandQueue::IncrementFence(void)
    {
        std::lock_guard<std::mutex> lockGuard(fenceMutex);
        handle->Signal(d3d12Fence, nextFenceValue);
        return nextFenceValue++;
    }

    bool D3D12CommandQueue::IsFenceComplete(uint64_t fenceValue)
    {
        // Avoid querying the fence value by testing against the last one seen.
        // The max() is to protect against an unlikely race condition that could cause the last
        // completed fence value to regress.
        if (fenceValue > lastCompletedFenceValue)
        {
            lastCompletedFenceValue = std::max(lastCompletedFenceValue, d3d12Fence->GetCompletedValue());
        }

        return fenceValue <= lastCompletedFenceValue;
    }

    void D3D12CommandQueue::WaitForFence(uint64_t fenceValue)
    {
        if (IsFenceComplete(fenceValue))
        {
            return;
        }

        // TODO:  Think about how this might affect a multi-threaded situation.  Suppose thread A
        // wants to wait for fence 100, then thread B comes along and wants to wait for 99.  If
        // the fence can only have one event set on completion, then thread B has to wait for 
        // 100 before it knows 99 is ready.  Maybe insert sequential events?
        {
            std::lock_guard<std::mutex> LockGuard(eventMutex);

            d3d12Fence->SetEventOnCompletion(fenceValue, fenceEvent);
            WaitForSingleObject(fenceEvent, INFINITE);
            lastCompletedFenceValue = fenceValue;
        }
    }

    ID3D12CommandAllocator* D3D12CommandQueue::RequestAllocator()
    {
        uint64_t completedFenceValue = d3d12Fence->GetCompletedValue();
        return allocatorPool.RequestAllocator(completedFenceValue);
    }

    uint64_t D3D12CommandQueue::ExecuteCommandList(ID3D12GraphicsCommandList* commandList)
    {
        std::lock_guard<std::mutex> LockGuard(fenceMutex);

        // Kickoff the command list
        ID3D12CommandList* commandLists[] = { commandList };
        handle->ExecuteCommandLists(1, commandLists);

        // Signal the next fence value (with the GPU)
        handle->Signal(d3d12Fence, nextFenceValue);

        // And increment the fence value.  
        return nextFenceValue++;
    }
#endif // TODO

}
