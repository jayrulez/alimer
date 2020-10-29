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
#include "D3D12CommandContext.h"
#include "D3D12GraphicsDevice.h"

namespace Alimer
{
    D3D12CommandAllocatorPool::D3D12CommandAllocatorPool(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
        : device{ device }
        , type{type}
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

        ID3D12CommandAllocator* commandAllocator = nullptr;

        if (!readyAllocators.empty())
        {
            auto& allocatorPair = readyAllocators.front();

            if (allocatorPair.first <= completedFenceValue)
            {
                commandAllocator = allocatorPair.second;
                ThrowIfFailed(commandAllocator->Reset());
                readyAllocators.pop();
            }
        }

        // If no allocator's were ready to be reused, create a new one
        if (commandAllocator == nullptr)
        {
            ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));
#ifdef _DEBUG
            wchar_t allocatorName[32];
            swprintf(allocatorName, 32, L"CommandAllocator %zu", allocatorPool.size());
            commandAllocator->SetName(allocatorName);
#endif

            allocatorPool.push_back(commandAllocator);
        }

        return commandAllocator;
    }

    void D3D12CommandAllocatorPool::DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator)
    {
        std::lock_guard<std::mutex> LockGuard(allocatorMutex);

        // That fence value indicates we are free to reset the allocator
        readyAllocators.push(std::make_pair(FenceValue, Allocator));
    }

    D3D12CommandQueue::D3D12CommandQueue(D3D12GraphicsDevice& device, D3D12_COMMAND_LIST_TYPE type)
        : device{ device }
        , type{ type }
        , allocatorPool(device.GetD3DDevice(), type)
        , nextFenceValue((uint64_t)type << 56 | 1)
        , lastCompletedFenceValue((uint64_t)type << 56)
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = type;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 0;

        ThrowIfFailed(device.GetD3DDevice()->CreateCommandQueue(&desc, IID_PPV_ARGS(&handle)));
        ThrowIfFailed(device.GetD3DDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
        fence->Signal(lastCompletedFenceValue);
        fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
        ALIMER_ASSERT(fenceEvent != INVALID_HANDLE_VALUE);

        switch (type)
        {
        case D3D12_COMMAND_LIST_TYPE_COPY:
            handle->SetName(L"Copy Command Queue");
            fence->SetName(L"Copy Command Queue Fence");
            break;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            handle->SetName(L"Compute Command Queue");
            fence->SetName(L"Compute Command Queue Fence");
            break;
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
            handle->SetName(L"Direct Command Queue");
            fence->SetName(L"Direct Command Queue Fence");
            break;
        }
    }

    D3D12CommandQueue::~D3D12CommandQueue()
    {
        allocatorPool.Shutdown();
        CloseHandle(fenceEvent);
    }

    uint64_t D3D12CommandQueue::Signal()
    {
        ThrowIfFailed(handle->Signal(fence.Get(), nextFenceValue));
        return nextFenceValue++;
    }

    bool D3D12CommandQueue::IsFenceComplete(uint64_t fenceValue)
    {
        // Avoid querying the fence value by testing against the last one seen.
        // The max() is to protect against an unlikely race condition that could cause the last
        // completed fence value to regress.
        if (fenceValue > lastCompletedFenceValue) {
            lastCompletedFenceValue = Max(lastCompletedFenceValue, fence->GetCompletedValue());
        }

        return fenceValue <= lastCompletedFenceValue;
    }

    void D3D12CommandQueue::WaitForFenceValue(uint64_t fenceValue)
    {
        if (IsFenceComplete(fenceValue))
            return;

        fence->SetEventOnCompletion(fenceValue, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
        lastCompletedFenceValue = fenceValue;
    }

    ID3D12CommandAllocator* D3D12CommandQueue::RequestAllocator()
    {
        uint64_t completedFenceValue = fence->GetCompletedValue();
        return allocatorPool.RequestAllocator(completedFenceValue);
    }

    D3D12CommandContext& D3D12CommandQueue::RequestCommandBuffer(const std::string& id)
    {
        D3D12CommandContext* result = nullptr;
        if (availableContexts.empty())
        {
            //result = new D3D12CommandContext(device, this, type);
            contextPool.emplace_back(result);
        }
        else
        {
            result = availableContexts.front();
            availableContexts.pop();
            result->Reset();
        }

        ALIMER_ASSERT(result != nullptr);
        return *result;
    }
}
