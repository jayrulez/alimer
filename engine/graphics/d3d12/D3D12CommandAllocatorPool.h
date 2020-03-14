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

#include "Graphics/GraphicsBuffer.h"
#include "D3D12Backend.h"
#include <EASTL/vector.h>
#include <EASTL/queue.h>
#include <mutex>

namespace Alimer
{
    class D3D12CommandAllocatorPool final
    {
    public:
        D3D12CommandAllocatorPool(ID3D12Device* device_, CommandQueueType queueType_);
        ~D3D12CommandAllocatorPool();

        void Destroy();

        ID3D12CommandAllocator* RequestAllocator(uint64_t fenceValue);
        void DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* commandAllocator);

    private:
        ID3D12Device* device;
        CommandQueueType queueType;
        const D3D12_COMMAND_LIST_TYPE commandListType;

        eastl::vector<ID3D12CommandAllocator*> allocators;
        eastl::queue<eastl::pair<uint64_t, ID3D12CommandAllocator*>> freeAllocators;
        std::mutex allocatorMutex;
    };
}