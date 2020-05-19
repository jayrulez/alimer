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

#if TODO_D3D12
#include "D3D12CommandQueue.h"
#include "D3D12GraphicsDevice.h"
#include <algorithm>

namespace alimer
{
    D3D12CommandQueue::D3D12CommandQueue(D3D12GraphicsDevice* device, D3D12_COMMAND_LIST_TYPE type)
        : commandListType(type)
        , allocatorPool(device, type)
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = type;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 0;

        lastCompletedFenceValue = ((uint64_t)desc.Type << 56);
        nextFenceValue = (uint64_t)desc.Type << 56 | 1;

        ThrowIfFailed(device->GetHandle()->CreateCommandQueue(&desc, IID_PPV_ARGS(&handle)));
        ThrowIfFailed(device->GetHandle()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
        fence->Signal(lastCompletedFenceValue);

        // Create an event handle to use for frame synchronization.
        fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        if (fenceEvent == INVALID_HANDLE_VALUE)
        {
            ALIMER_LOGERROR("CreateEventEx failed");
        }

        switch (type)
        {
        case D3D12_COMMAND_LIST_TYPE_COPY:
            handle->SetName(L"COPY QUEUE");
            fence->SetName(L"COPY QUEUE FENCE");
            break;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            handle->SetName(L"COMPUTE QUEUE");
            fence->SetName(L"COMPUTE QUEUE FENCE");
            break;
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
            handle->SetName(L"GRAPHICS QUEUE");
            fence->SetName(L"GRAPHICS QUEUE FENCE");
            break;
        }
    }

    D3D12CommandQueue::~D3D12CommandQueue()
    {
        Destroy();
    }

    void D3D12CommandQueue::Destroy()
    {
        if (handle == nullptr)
            return;

        allocatorPool.Destroy();
        CloseHandle(fenceEvent);
        fence->Release();
        fence = nullptr;

        handle->Release();
        handle = nullptr;
    }

    void D3D12CommandQueue::WaitForIdle()
    {
        WaitForFence(IncrementFence());
    }

    uint64_t D3D12CommandQueue::IncrementFence(void)
    {
        std::lock_guard<std::mutex> lockGuard(fenceMutex);
        handle->Signal(fence, nextFenceValue);
        return nextFenceValue++;
    }

    bool D3D12CommandQueue::IsFenceComplete(uint64_t fenceValue)
    {
        // Avoid querying the fence value by testing against the last one seen.
        // The max() is to protect against an unlikely race condition that could cause the last
        // completed fence value to regress.
        if (fenceValue > lastCompletedFenceValue)
        {
            lastCompletedFenceValue = std::max(lastCompletedFenceValue, fence->GetCompletedValue());
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

            fence->SetEventOnCompletion(fenceValue, fenceEvent);
            WaitForSingleObject(fenceEvent, INFINITE);
            lastCompletedFenceValue = fenceValue;
        }
    }

    ID3D12CommandAllocator* D3D12CommandQueue::RequestAllocator()
    {
        uint64_t completedFenceValue = fence->GetCompletedValue();
        return allocatorPool.RequestAllocator(completedFenceValue);
    }

    uint64_t D3D12CommandQueue::ExecuteCommandList(ID3D12GraphicsCommandList* commandList)
    {
        std::lock_guard<std::mutex> LockGuard(fenceMutex);

        // Kickoff the command list
        ID3D12CommandList* commandLists[] = { commandList };
        handle->ExecuteCommandLists(1, commandLists);

        // Signal the next fence value (with the GPU)
        handle->Signal(fence, nextFenceValue);

        // And increment the fence value.  
        return nextFenceValue++;
    }
}
#endif // TODO_D3D12

