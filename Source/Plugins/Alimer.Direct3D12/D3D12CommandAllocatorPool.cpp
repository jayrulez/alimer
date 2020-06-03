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

#include "D3D12CommandAllocatorPool.h"
#include "D3D12GraphicsDevice.h"
#include <algorithm>

namespace alimer
{
    D3D12CommandAllocatorPool::D3D12CommandAllocatorPool(D3D12GraphicsDevice* device, D3D12_COMMAND_LIST_TYPE type_)
        : device{ device }
        , type(type_)
    {

    }

    D3D12CommandAllocatorPool::~D3D12CommandAllocatorPool()
    {
        Destroy();
    }

    void D3D12CommandAllocatorPool::Destroy()
    {
        for (auto allocator : allocators)
        {
            allocator->Release();
        }

        allocators.Clear();
    }

    ID3D12CommandAllocator* D3D12CommandAllocatorPool::RequestAllocator(uint64_t fenceValue)
    {
        std::lock_guard<std::mutex> LockGuard(allocatorMutex);

        ID3D12CommandAllocator* commandAllocator = nullptr;

        if (!readyAllocators.empty())
        {
            std::pair<uint64_t, ID3D12CommandAllocator*>& pair = readyAllocators.front();

            if (pair.first <= fenceValue)
            {
                commandAllocator = pair.second;
                VHR(commandAllocator->Reset());
                readyAllocators.pop();
                LOG_DEBUG("Direct3D12: Reusing CommandAllocator %u", pair.first);
            }
        }

        // If no allocator's were ready to be reused, create a new one
        if (commandAllocator == nullptr)
        {
            VHR(device->GetHandle()->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));
#if defined(_DEBUG)
            wchar_t allocatorName[32];
            swprintf(allocatorName, 32, L"CommandAllocator %u", allocators.Size());
            LOG_DEBUG("Direct3D12: Allocated CommandAllocator %u", allocators.Size());
            commandAllocator->SetName(allocatorName);
#endif

            allocators.Push(commandAllocator);
        }

        return commandAllocator;
    }

    void D3D12CommandAllocatorPool::DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* commandAllocator)
    {
        std::lock_guard<std::mutex> LockGuard(allocatorMutex);

        // That fence value indicates we are free to reset the allocator
        readyAllocators.push(std::make_pair(fenceValue, commandAllocator));
    }
}

