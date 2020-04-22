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

#include "graphics/Types.h"
#include "D3D12CommandAllocatorPool.h"

namespace alimer
{
    /*class D3D12CommandAllocatorPool;

    class D3D12CommandQueue final 
    {
    public:
        D3D12CommandQueue(QueueType type_);
        ~D3D12CommandQueue();

        void Create(ID3D12Device* device);
        void Destroy();

        uint64_t IncrementFence(void);
        bool IsFenceComplete(uint64_t fenceValue);
        void WaitForFence(uint64_t fenceValue);
        void WaitForIdle(void) { WaitForFence(IncrementFence()); }
        ID3D12CommandQueue* GetHandle() { return handle; }
        uint64_t GetNextFenceValue() { return nextFenceValue; }
        uint64_t ExecuteCommandList(ID3D12GraphicsCommandList* commandList);

        ID3D12CommandAllocator* RequestAllocator();

    private:
        const QueueType type;
        const D3D12_COMMAND_LIST_TYPE commandListType;
        ID3D12CommandQueue* handle = nullptr;

        // Lifetime of these objects is managed by the descriptor cache
        ID3D12Fence* d3d12Fence = nullptr;
        uint64_t nextFenceValue;
        uint64_t lastCompletedFenceValue;
        HANDLE fenceEvent = INVALID_HANDLE_VALUE;

        D3D12CommandAllocatorPool allocatorPool;
        std::mutex fenceMutex;
        std::mutex eventMutex;
    };*/
}
