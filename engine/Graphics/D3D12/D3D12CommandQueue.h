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

#include "Graphics/CommandQueue.h"
#include "D3D12Backend.h"
#include <EASTL/vector.h>
#include <EASTL/queue.h>
#include <mutex>

#include "Core/ThreadSafeQueue.h"

namespace Alimer
{
    class D3D12CommandAllocatorPool final
    {
    public:
        D3D12CommandAllocatorPool(D3D12GraphicsDevice* device_, D3D12_COMMAND_LIST_TYPE type_);
        ~D3D12CommandAllocatorPool();

        void Shutdown();

        ID3D12CommandAllocator* RequestAllocator(uint64_t completedFenceValue);
        void DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* allocator);

        ALIMER_FORCE_INLINE size_t Size() { return allocatorPool.size(); }

    private:
        const D3D12_COMMAND_LIST_TYPE type;

        D3D12GraphicsDevice* device;
        eastl::vector<ID3D12CommandAllocator*> allocatorPool;
        eastl::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> readyAllocators;
        std::mutex allocatorMutex;
    };

    class D3D12CommandBuffer;

    class D3D12CommandQueue final : public CommandQueue
    {
    public:
        D3D12CommandQueue(D3D12GraphicsDevice* device, CommandQueueType queueType);
        ~D3D12CommandQueue();

        GraphicsDevice* GetGraphicsDevice() const override;
        RefPtr<CommandBuffer> GetCommandBuffer() override;

        uint64_t Signal();
        bool IsFenceComplete(uint64_t fenceValue);
        void WaitForFenceValue(uint64_t fenceValue);
        void WaitIdle() override;

        uint64_t ExecuteCommandList(ID3D12GraphicsCommandList* commandList);
        ID3D12CommandAllocator* RequestAllocator();
        void DiscardAllocator(uint64_t fenceValueForReset, ID3D12CommandAllocator* commandAllocator);

        ID3D12CommandQueue* GetHandle() { return handle.Get(); }

    private:
        D3D12GraphicsDevice* device;
        const D3D12_COMMAND_LIST_TYPE type;
        ComPtr<ID3D12CommandQueue> handle;
        ComPtr<ID3D12Fence> fence;
        std::atomic_uint64_t fenceValue;

        ThreadSafeQueue<RefPtr<D3D12CommandBuffer>> availableCommandBuffers;

        D3D12CommandAllocatorPool allocatorPool;
    };
}
