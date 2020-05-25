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
//#include "D3D12GraphicsDevice.h"
#include "core/Assert.h"

namespace alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
    PFN_D3D12_CREATE_DEVICE D3D12CreateDevice = nullptr;
    PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface = nullptr;
    PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignature = nullptr;
    PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER D3D12CreateRootSignatureDeserializer = nullptr;
    PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE D3D12SerializeVersionedRootSignature = nullptr;
    PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER D3D12CreateVersionedRootSignatureDeserializer = nullptr;
#endif

    /* Fence */
    FenceD3D12::FenceD3D12(D3D12GraphicsDevice* device)
        : device{ device }
    {
    }

    FenceD3D12::~FenceD3D12()
    {
        Shutdown();
    }

    void FenceD3D12::Init(uint64_t initialValue)
    {
        Shutdown();
        //ThrowIfFailed(device->GetHandle()->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&handle)));
        fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        ALIMER_ASSERT(fenceEvent != INVALID_HANDLE_VALUE);
    }

    void FenceD3D12::Shutdown()
    {
        if (handle == nullptr)
            return;

        CloseHandle(fenceEvent);
        //device->DeferredRelease(handle);
    }

    void FenceD3D12::Signal(ID3D12CommandQueue* queue, uint64_t fenceValue)
    {
        ALIMER_ASSERT(handle != nullptr);
        ThrowIfFailed(queue->Signal(handle, fenceValue));
    }

    void FenceD3D12::Wait(uint64_t fenceValue)
    {
        ALIMER_ASSERT(handle != nullptr);
        if (handle->GetCompletedValue() < fenceValue)
        {
            ThrowIfFailed(handle->SetEventOnCompletion(fenceValue, fenceEvent));
            WaitForSingleObject(fenceEvent, INFINITE);
        }
    }

    void FenceD3D12::GPUWait(ID3D12CommandQueue* queue, uint64_t fenceValue)
    {
        ALIMER_ASSERT(queue != nullptr);
        ALIMER_ASSERT(handle != nullptr);
        ThrowIfFailed(queue->Wait(handle, fenceValue));
    }

    bool FenceD3D12::IsSignaled(uint64_t fenceValue)
    {
        ALIMER_ASSERT(handle != nullptr);
        return handle->GetCompletedValue() >= fenceValue;
    }

    void FenceD3D12::Clear(uint64_t fenceValue)
    {
        ALIMER_ASSERT(handle != nullptr);
        handle->Signal(fenceValue);
    }

    /* D3D12DescriptorHeap */
    D3D12DescriptorHeap::D3D12DescriptorHeap(D3D12GraphicsDevice* device_, D3D12_DESCRIPTOR_HEAP_TYPE type_, bool shaderVisible_)
        : device(device_)
        , type(type_)
        , shaderVisible(shaderVisible_)
        , numHeaps(shaderVisible ? 2 : 1)
    {

    }

    D3D12DescriptorHeap::~D3D12DescriptorHeap()
    {

    }

    void D3D12DescriptorHeap::Init(uint32_t numPersistent_, uint32_t numTemporary_)
    {
        Shutdown();

        /*descriptorSize = device->GetD3DDevice()->GetDescriptorHandleIncrementSize(type);

        numPersistent = numPersistent_;
        numTemporary = numTemporary_;
        uint32_t totalNumDescriptors = numPersistent_ + numTemporary_;
        ALIMER_ASSERT(totalNumDescriptors > 0);

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type = type;
        heapDesc.NumDescriptors = totalNumDescriptors;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        if (shaderVisible)
        {
            heapDesc.Flags |= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        }
        heapDesc.NodeMask = 0;

        for (uint32_t i = 0; i < numHeaps; ++i)
        {
            ThrowIfFailed(device->GetD3DDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heaps[i])));
            CPUStart[i] = heaps[i]->GetCPUDescriptorHandleForHeapStart();
            if (shaderVisible)
            {
                GPUStart[i] = heaps[i]->GetGPUDescriptorHandleForHeapStart();
            }
        }

        deadList.resize(numPersistent);
        for (uint32_t i = 0; i < numPersistent; ++i)
            deadList[i] = uint32_t(i);*/
    }

    void D3D12DescriptorHeap::Shutdown()
    {
        const uint32_t NumHeaps = shaderVisible ? 2 : 1;

        ALIMER_ASSERT(persistentAllocated == 0);
        for (uint32_t i = 0; i < NumHeaps; ++i)
        {
            SAFE_RELEASE(heaps[i]);
        }
    }

    PersistentDescriptorAlloc D3D12DescriptorHeap::AllocatePersistent()
    {
        ALIMER_ASSERT(heaps[0] != nullptr);

        AcquireSRWLockExclusive(&lock);

        ALIMER_ASSERT(persistentAllocated < numPersistent);
        uint32_t idx = deadList[persistentAllocated];
        ++persistentAllocated;

        ReleaseSRWLockExclusive(&lock);

        PersistentDescriptorAlloc alloc;
        alloc.index = idx;
        for (uint32_t i = 0; i < numHeaps; ++i)
        {
            alloc.handles[i] = CPUStart[i];
            alloc.handles[i].ptr += idx * descriptorSize;
        }

        return alloc;
    }

    void D3D12DescriptorHeap::FreePersistent(uint32_t& index)
    {
        if (index == uint32_t(-1))
            return;

        ALIMER_ASSERT(index < numPersistent);
        ALIMER_ASSERT(heaps[0] != nullptr);

        AcquireSRWLockExclusive(&lock);

        ALIMER_ASSERT(persistentAllocated > 0);
        deadList[persistentAllocated - 1] = index;
        --persistentAllocated;

        ReleaseSRWLockExclusive(&lock);

        index = uint32_t(-1);
    }

    void D3D12DescriptorHeap::FreePersistent(D3D12_CPU_DESCRIPTOR_HANDLE& handle)
    {
        ALIMER_ASSERT(numHeaps == 1);
        if (handle.ptr != 0)
        {
            uint32_t index = IndexFromHandle(handle);
            FreePersistent(index);
            handle = { };
        }
    }

    void D3D12DescriptorHeap::FreePersistent(D3D12_GPU_DESCRIPTOR_HANDLE& handle)
    {
        ALIMER_ASSERT(numHeaps == 1);
        if (handle.ptr != 0)
        {
            uint32_t index = IndexFromHandle(handle);
            FreePersistent(index);
            handle = { };
        }
    }

    uint32_t D3D12DescriptorHeap::IndexFromHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle)
    {
        ALIMER_ASSERT(heaps[0] != nullptr);
        ALIMER_ASSERT(handle.ptr >= CPUStart[heapIndex].ptr);
        ALIMER_ASSERT(handle.ptr < CPUStart[heapIndex].ptr + descriptorSize * TotalNumDescriptors());
        ALIMER_ASSERT((handle.ptr - CPUStart[heapIndex].ptr) % descriptorSize == 0);
        return uint32_t(handle.ptr - CPUStart[heapIndex].ptr) / descriptorSize;
    }

    uint32_t D3D12DescriptorHeap::IndexFromHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle)
    {
        ALIMER_ASSERT(heaps[0] != nullptr);
        ALIMER_ASSERT(handle.ptr >= GPUStart[heapIndex].ptr);
        ALIMER_ASSERT(handle.ptr < GPUStart[heapIndex].ptr + descriptorSize * TotalNumDescriptors());
        ALIMER_ASSERT((handle.ptr - GPUStart[heapIndex].ptr) % descriptorSize == 0);
        return uint32_t(handle.ptr - GPUStart[heapIndex].ptr) / descriptorSize;
    }
}
