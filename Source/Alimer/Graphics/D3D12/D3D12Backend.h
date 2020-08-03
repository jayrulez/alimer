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

#include "core/Assert.h"
#include "core/Log.h"
#include "graphics/Types.h"
#include "graphics/D3D/D3DHelpers.h"

#include "D3D12MemAlloc.h"

// To use graphics and CPU markup events with the latest version of PIX, change this to include <pix3.h>
// then add the NuGet package WinPixEventRuntime to the project.
#include <pix.h>

#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

namespace alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    extern PFN_D3D12_CREATE_DEVICE D3D12CreateDevice;
    extern PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface;
    extern PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignature;
    extern PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER D3D12CreateRootSignatureDeserializer;
    extern PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE D3D12SerializeVersionedRootSignature;
    extern PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER D3D12CreateVersionedRootSignatureDeserializer;
#endif

    class D3D12GraphicsDevice;

    struct D3D12MapResult
    {
        void* CPUAddress = nullptr;
        uint64_t GPUAddress = 0;
        uint64_t ResourceOffset = 0;
        ID3D12Resource* Resource = nullptr;
    };

    static inline D3D12_COMMAND_LIST_TYPE D3D12GetCommandListType(CommandQueueType queueType)
    {
        switch (queueType)
        {
        case CommandQueueType::Graphics:
            return D3D12_COMMAND_LIST_TYPE_DIRECT;

        case CommandQueueType::Compute:
            return D3D12_COMMAND_LIST_TYPE_COMPUTE;

        case CommandQueueType::Copy:
            return D3D12_COMMAND_LIST_TYPE_COPY;

        default:
            ALIMER_UNREACHABLE();
        }
    }

    static inline CommandQueueType D3D12GetCommandQueueType(D3D12_COMMAND_LIST_TYPE type)
    {
        switch (type)
        {
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
            return CommandQueueType::Graphics;

        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            return CommandQueueType::Compute;

        case D3D12_COMMAND_LIST_TYPE_COPY:
            return CommandQueueType::Copy;

        default:
            ALIMER_UNREACHABLE();
        }
    }

    static inline D3D12_HEAP_TYPE GetD3D12HeapType(MemoryUsage usage)
    {
        switch (usage)
        {
        case MemoryUsage::GpuOnly:
            return D3D12_HEAP_TYPE_DEFAULT;

        case MemoryUsage::CpuOnly:
            return D3D12_HEAP_TYPE_UPLOAD;

        case MemoryUsage::GpuToCpu:
            return D3D12_HEAP_TYPE_READBACK;

        default:
            ALIMER_UNREACHABLE();
        }
    }

    static inline D3D12_RESOURCE_STATES GetD3D12ResourceState(MemoryUsage usage)
    {
        switch (usage)
        {
        case MemoryUsage::GpuOnly:
            return D3D12_RESOURCE_STATE_COMMON;

        case MemoryUsage::CpuOnly:
            return D3D12_RESOURCE_STATE_GENERIC_READ;

        case MemoryUsage::GpuToCpu:
            return D3D12_RESOURCE_STATE_COPY_DEST;

        default:
            ALIMER_UNREACHABLE();
        }
    }

    class D3D12GpuResource
    {
    public:
        D3D12GpuResource() noexcept
            : resource(nullptr)
            , state(D3D12_RESOURCE_STATE_COMMON)
            , transitioningState((D3D12_RESOURCE_STATES)-1)
            , gpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL)
        {
        }

        D3D12GpuResource(ID3D12Resource* resource_, D3D12_RESOURCE_STATES currentState)
            : resource(resource_)
            , state(currentState)
            , transitioningState((D3D12_RESOURCE_STATES)-1)
            , gpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL)
        {
        }

        virtual void Destroy()
        {
            gpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
        }

        D3D12_RESOURCE_STATES GetState() const { return state; }
        void SetState(D3D12_RESOURCE_STATES newState) { state = newState; }

        D3D12_RESOURCE_STATES GetTransitioningState() const { return transitioningState; }
        void SetTransitioningState(D3D12_RESOURCE_STATES newState) { transitioningState = newState; }

        ID3D12Resource* operator->() { return resource.Get(); }
        const ID3D12Resource* operator->() const { return resource.Get(); }

        ID3D12Resource* GetResource() { return resource.Get(); }
        const ID3D12Resource* GetResource() const { return resource.Get(); }

        
        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return gpuVirtualAddress; }

    protected:
        ComPtr<ID3D12Resource> resource;
        D3D12_RESOURCE_STATES state;
        D3D12_RESOURCE_STATES transitioningState;
        D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress;
    };

    struct DescriptorHeap
    {
        ID3D12DescriptorHeap* handle;
        D3D12_CPU_DESCRIPTOR_HANDLE CpuStart;
        D3D12_GPU_DESCRIPTOR_HANDLE GpuStart;
        uint32_t Size;
        uint32_t Capacity;
        uint32_t DescriptorSize;
    };

    DescriptorHeap D3D12CreateDescriptorHeap(ID3D12Device* device, uint32_t capacity, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags);
}