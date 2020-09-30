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

#include "D3D12Backend.h"
#include "D3D12GraphicsDevice.h"

namespace Alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    PFN_DXC_CREATE_INSTANCE DxcCreateInstance = nullptr;
#endif

    void D3D12SetObjectName(ID3D12Object* obj, const std::string& name)
    {
#ifdef _DEBUG
        if (obj != nullptr)
        {
            if (!name.empty())
            {
                std::wstring wideStr = ToUtf16(name);
                obj->SetName(wideStr.c_str());
            }
            else
                obj->SetName(nullptr);
        }
#else
        ALIMER_UNUSED(obj);
        ALIMER_UNUSED(name);
#endif
    }

    D3D12Fence::D3D12Fence(D3D12GraphicsDevice* device)
        : device{ device }
        , cpuValue(0)
    {
        //ThrowIfFailed(device->GetD3DDevice()->CreateFence(cpuValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&handle)));
        cpuValue++;

        fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
        ALIMER_ASSERT(fenceEvent != 0);
    }

    D3D12Fence::~D3D12Fence()
    {
        Shutdown();
    }

    void D3D12Fence::Shutdown()
    {
        if (!handle)
            return;

        CloseHandle(fenceEvent);
        //device->ReleaseResource(handle);
    }

    uint64_t D3D12Fence::GpuSignal(ID3D12CommandQueue* queue)
    {
        ThrowIfFailed(queue->Signal(handle, cpuValue));
        cpuValue++;
        return cpuValue - 1;
    }

    void D3D12Fence::SyncGpu(ID3D12CommandQueue* queue)
    {
        ThrowIfFailed(queue->Wait(handle, cpuValue - 1));
    }

    void D3D12Fence::SyncCpu()
    {
        SyncCpu(cpuValue - 1);
    }

    void D3D12Fence::SyncCpu(uint64_t value)
    {
        ALIMER_ASSERT(value <= cpuValue - 1);

        uint64_t gpuVal = GetGpuValue();
        if (gpuVal < value)
        {
            ThrowIfFailed(handle->SetEventOnCompletion(value, fenceEvent));
            WaitForSingleObject(fenceEvent, INFINITE);
        }
    }

    uint64_t D3D12Fence::GetGpuValue() const
    {
        return handle->GetCompletedValue();
    }

    /* D3D12CommandAllocatorPool */
    D3D12CommandAllocatorPool::D3D12CommandAllocatorPool(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
        : device{ device }
        , type{ type }
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
            std::pair<uint64_t, ID3D12CommandAllocator*>& allocatorPair = readyAllocators.front();

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

    void D3D12CommandAllocatorPool::DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* commandAllocator)
    {
        std::lock_guard<std::mutex> LockGuard(allocatorMutex);

        // That fence value indicates we are free to reset the allocator
        readyAllocators.push(std::make_pair(fenceValue, commandAllocator));
    }
}
