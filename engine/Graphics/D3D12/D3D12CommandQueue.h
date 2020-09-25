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

namespace Alimer
{
    class D3D12CommandQueue final
    {
    public:
        D3D12CommandQueue(D3D12GraphicsDevice* device, D3D12_COMMAND_LIST_TYPE type);
        ~D3D12CommandQueue();

        uint64 IncrementFence(void);
        bool IsFenceComplete(uint64 fenceValue);
        void StallForFence(uint64 fenceValue);
        void StallForProducer(D3D12CommandQueue& Producer);
        void WaitForFence(uint64 fenceValue);
        void WaitForIdle(void) { WaitForFence(IncrementFence()); }

        uint64 ExecuteCommandList(ID3D12GraphicsCommandList* commandList);
        ID3D12CommandAllocator* RequestAllocator();
        void DiscardAllocator(uint64 fenceValue, ID3D12CommandAllocator* commandAllocator);

        ID3D12CommandQueue* GetHandle() const noexcept { return handle; }
        D3D12_COMMAND_LIST_TYPE GetType() const noexcept { return type; }
        uint64_t GetNextFenceValue() const noexcept { return nextFenceValue; }
        uint64_t GetFenceCompletedValue() const { return fence->GetCompletedValue(); }

    private:
        D3D12GraphicsDevice* device;
        const D3D12_COMMAND_LIST_TYPE type;

        ID3D12CommandQueue* handle;

        D3D12CommandAllocatorPool allocatorPool;
        std::mutex fenceMutex;
        std::mutex eventMutex;

        ID3D12Fence* fence;
        HANDLE fenceEvent;
        uint64_t nextFenceValue;
        uint64_t lastCompletedFenceValue;
    };
}
