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

#include "D3D12Backend.h"

namespace Alimer
{
    class D3D12CommandContext;

    class D3D12CommandAllocatorPool
    {
    public:
        D3D12CommandAllocatorPool(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE Type);
        ~D3D12CommandAllocatorPool();
        void Shutdown();

        ID3D12CommandAllocator* RequestAllocator(uint64_t completedFenceValue);
        void DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* commandAllocator);

        inline size_t Size() { return allocatorPool.size(); }

    private:
        ID3D12Device* device;
        const D3D12_COMMAND_LIST_TYPE type;

        std::vector<ID3D12CommandAllocator*> allocatorPool;
        std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> readyAllocators;
        std::mutex allocatorMutex;
    };

    class D3D12CommandQueue final 
    {
    public:
        D3D12CommandQueue(D3D12GraphicsDevice& device, D3D12_COMMAND_LIST_TYPE type);
        ~D3D12CommandQueue();

        uint64_t Signal();
        bool IsFenceComplete(uint64_t fenceValue);
        void WaitForFenceValue(uint64_t fenceValue);
        void WaitForIdle(void) { WaitForFenceValue(Signal()); }

        ID3D12CommandAllocator* RequestAllocator();
        D3D12CommandContext& RequestCommandBuffer(const std::string& id);

        ID3D12CommandQueue* GetHandle() const { return handle.Get(); }

    private:
        D3D12GraphicsDevice& device;
        const D3D12_COMMAND_LIST_TYPE type;

        D3D12CommandAllocatorPool allocatorPool;

        ComPtr<ID3D12CommandQueue> handle;
        ComPtr<ID3D12Fence> fence;
        HANDLE fenceEvent;
        std::atomic_uint64_t nextFenceValue;
        uint64_t lastCompletedFenceValue;

        std::queue<D3D12CommandContext*> availableContexts;
        std::vector<std::unique_ptr<D3D12CommandContext> > contextPool;
    };
}
