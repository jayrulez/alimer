//
// Copyright (c) 2019-2020 Amer Koleci and contributors.
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

#pragma once

#include "Graphics/CommandContext.h"
#include "D3D12Backend.h"
#include <vector>
#include <queue>
#include <mutex>

namespace alimer
{
    class D3D12CommandBuffer;

    class D3D12CommandAllocatorPool final
    {
    public:
        D3D12CommandAllocatorPool(D3D12GraphicsDevice* device_, D3D12_COMMAND_LIST_TYPE type_);
        ~D3D12CommandAllocatorPool();

        void Shutdown();

        ID3D12CommandAllocator* RequestAllocator(uint64_t completedFenceValue);
        void DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* allocator);

        ALIMER_FORCEINLINE size_t Size() { return allocatorPool.size(); }

    private:
        const D3D12_COMMAND_LIST_TYPE type;

        D3D12GraphicsDevice* device;
        std::vector<ID3D12CommandAllocator*> allocatorPool;
        std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> readyAllocators;
        std::mutex allocatorMutex;
    };

    class D3D12CommandQueue final
    {
    public:
        D3D12CommandQueue(D3D12GraphicsDevice* device, CommandQueueType queueType, const std::string_view& name);
        ~D3D12CommandQueue();

        void WaitIdle();

        uint64_t Signal();
        bool IsFenceComplete(uint64_t fenceValue);
        void WaitForFence(uint64_t fenceValue);

        uint64_t ExecuteCommandList(ID3D12GraphicsCommandList* commandList);
        ID3D12CommandAllocator* RequestAllocator();
        void DiscardAllocator(uint64_t fenceValueForReset, ID3D12CommandAllocator* commandAllocator);

        ALIMER_FORCEINLINE ID3D12CommandQueue* GetCommandQueue() { return commandQueue; }
        ALIMER_FORCEINLINE uint64_t GetNextFenceValue() { return nextFenceValue; }

    private:
        D3D12GraphicsDevice* device;
        const D3D12_COMMAND_LIST_TYPE type;
        ID3D12CommandQueue* commandQueue;

        D3D12CommandAllocatorPool allocatorPool;
        std::mutex fenceMutex;
        std::mutex eventMutex;

        // Lifetime of these objects is managed by the descriptor cache
        ID3D12Fence* fence;
        HANDLE fenceEvent;
        uint64_t nextFenceValue;
        uint64_t lastCompletedFenceValue;
    };
}
