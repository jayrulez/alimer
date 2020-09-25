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
#include "D3D12GraphicsDevice.h"
#include "D3D12Texture.h"

namespace Alimer
{
    D3D12CommandQueue::D3D12CommandQueue(D3D12GraphicsDevice* device, D3D12_COMMAND_LIST_TYPE type)
        : device{ device }
        , type{ type }
        , nextFenceValue((uint64_t)type << 56 | 1)
        , lastCompletedFenceValue((uint64_t)type << 56)
        , allocatorPool(device->GetD3DDevice(), type)
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = type;
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.NodeMask = 0;
        ThrowIfFailed(device->GetD3DDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&handle)));

        // Create fence
        ThrowIfFailed(device->GetD3DDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
        fence->Signal((uint64_t)type << 56);
        fenceEvent = CreateEventExW(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
        ALIMER_ASSERT(fenceEvent != 0);

        switch (type)
        {
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
            handle->SetName(L"Graphics Command Queue");
            fence->SetName(L"Graphics Command Queue Fence");
            break;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            handle->SetName(L"Compute Command Queue");
            fence->SetName(L"Compute Command Queue pFence");
            break;
        case D3D12_COMMAND_LIST_TYPE_COPY:
            handle->SetName(L"Copy Command Queue");
            fence->SetName(L"Copy Command Queue  FFence");
            break;
        default:
            break;
        }
    }

    D3D12CommandQueue::~D3D12CommandQueue()
    {
        allocatorPool.Shutdown();

        CloseHandle(fenceEvent);
        fence->Release(); fence = nullptr;
        handle->Release(); handle = nullptr;
    }

    uint64 D3D12CommandQueue::IncrementFence(void)
    {
        std::lock_guard<std::mutex> LockGuard(fenceMutex);
        ThrowIfFailed(handle->Signal(fence, nextFenceValue));
        return nextFenceValue++;
    }

    bool D3D12CommandQueue::IsFenceComplete(uint64 fenceValue)
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

    void D3D12CommandQueue::WaitForFence(uint64 fenceValue)
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
