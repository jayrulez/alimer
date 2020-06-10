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

#include "graphics/CommandQueue.h"
#include "D3D12Backend.h"
#include <atomic>               
#include <cstdint>              
#include <condition_variable>   
#include <queue>
#include <mutex>

namespace alimer
{
    class D3D12CommandBuffer;

    class D3D12CommandQueue final : public CommandQueue
    {
    public:
        D3D12CommandQueue(D3D12GraphicsDevice* device, CommandQueueType queueType, const char* name);
        ~D3D12CommandQueue() override;

        void WaitIdle() override;
        uint64_t Signal();
        bool IsFenceComplete(uint64_t fenceValue);
        void WaitForFenceValue(uint64_t fenceValue);

        ID3D12CommandQueue* GetHandle() const { return handle; }

    private:
        // Free any command buffers that are finished processing on the command queue.
        void ProccessCommandBuffers();

        RefPtr<CommandBuffer> GetCommandBuffer() override;

        ID3D12CommandQueue* handle;
        ID3D12Fence* fence;
        std::atomic_uint64_t fenceValue;

        std::queue<RefPtr<D3D12CommandBuffer>> availableCommandBuffers;
        std::mutex availableCommandBuffersMutex;

        using CommandListEntry = std::tuple<uint64_t, RefPtr<D3D12CommandBuffer>>;
        std::queue<CommandListEntry> inFlightCommandBuffers;
        std::mutex inFlightCommandBuffersMutex;

        // A thread to process in-flight command buffers.
        std::atomic_bool processCommandBuffers;
        std::thread processCommandBuffersThread;
        std::mutex processCommandBuffersThreadMutex;
        std::condition_variable processCommandBuffersThreadCV;
    };
}
