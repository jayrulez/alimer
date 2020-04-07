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
#include <algorithm>

namespace alimer
{
    D3D12CommandQueue::D3D12CommandQueue(D3D12GraphicsDevice* device_, D3D12_COMMAND_LIST_TYPE type_)
        : device(device_)
        , type(type_)
        , nextFenceValue((uint64_t)type_ << 56 | 1)
        , lastCompletedFenceValue((uint64_t)type_ << 56)
        , allocatorPool(type_)
    {
    }

    D3D12CommandQueue::~D3D12CommandQueue()
    {
        Destroy();
    }

    void D3D12CommandQueue::Create()
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = type;
        queueDesc.NodeMask = 0;

        ThrowIfFailed(device->GetD3DDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&handle)));
        ThrowIfFailed(device->GetD3DDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d3d12Fence)));
        d3d12Fence->Signal((uint64_t)type << 56);

        switch (type)
        {
        case D3D12_COMMAND_LIST_TYPE_COPY:
            handle->SetName(L"Copy Command Queue");
            d3d12Fence->SetName(L"Copy Command Queue Fence");
            break;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            handle->SetName(L"Compute Command Queue");
            d3d12Fence->SetName(L"Compute Command Queue Fence");
            break;
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
            handle->SetName(L"Graphics Command Queue");
            d3d12Fence->SetName(L"Graphics Command Queue Fence");
            break;
        }

        // Create an event handle to use for frame synchronization.
        fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        if (fenceEvent == nullptr)
        {
            ALIMER_LOGERROR("CreateEventEx failed");
        }
    }

    void D3D12CommandQueue::Destroy()
    {
        if (handle == nullptr)
            return;

        allocatorPool.Destroy();
        CloseHandle(fenceEvent);

        d3d12Fence->Release();
        d3d12Fence = nullptr;

        handle->Release();
        handle = nullptr;
    }

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
}

