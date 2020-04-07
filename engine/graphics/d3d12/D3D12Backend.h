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

#include "core/Utils.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "graphics/Types.h"
#include "../d3d/D3DCommon.h"

#include <d3d12.h>
#include <wrl.h>

// DXProgrammableCapture.h takes a dependency on other platform header
// files, so it must be defined after them.
#include <DXProgrammableCapture.h>
#include <dxgidebug.h>

// Forward declare memory allocator classes
namespace D3D12MA
{
    class Allocator;
    class Allocation;
};

#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

namespace alimer
{
    using Microsoft::WRL::ComPtr;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
    // D3D12 functions.
    extern PFN_D3D12_CREATE_DEVICE D3D12CreateDevice;
    extern PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface;
    extern PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignature;
    extern PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER D3D12CreateRootSignatureDeserializer;
    extern PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE D3D12SerializeVersionedRootSignature;
    extern PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER D3D12CreateVersionedRootSignatureDeserializer;
#endif

    class D3D12GraphicsDevice;

    class D3D12GpuResource
    {
    public:
        D3D12GpuResource()
            : handle(nullptr)
            , usageState(D3D12_RESOURCE_STATE_COMMON)
            , transitioningState((D3D12_RESOURCE_STATES)-1)
            , gpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL)
        {
        }

        D3D12GpuResource(ID3D12Resource* handle_, D3D12_RESOURCE_STATES currentState)
            : handle(handle_)
            , usageState(currentState)
            , transitioningState((D3D12_RESOURCE_STATES)-1)
            , gpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL)
        {
        }

        ~D3D12GpuResource()
        {
            Destroy();
        }

        virtual void Destroy()
        {
            SafeRelease(handle);
            gpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
        }

        ID3D12Resource* operator->() { return handle; }
        const ID3D12Resource* operator->() const { return handle; }

        ID3D12Resource* GetResource() { return handle; }
        const ID3D12Resource* GetResource() const { return handle; }

        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return gpuVirtualAddress; }

    protected:
        ID3D12Resource* handle;
        D3D12_RESOURCE_STATES usageState;
        D3D12_RESOURCE_STATES transitioningState;
        D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress;

    };

    class D3D12GPUFence
    {
    public:
        D3D12GPUFence(D3D12GraphicsDevice* device_);
        ~D3D12GPUFence();

        void Init(uint64_t initialValue = 0);
        void Shutdown();
        void Signal(ID3D12CommandQueue* queue, uint64_t fenceValue);
        void Wait(uint64_t fenceValue);
        bool IsSignaled(uint64_t fenceValue);
        void Clear(uint64_t fenceValue);

    private:
        D3D12GraphicsDevice* device;

        ID3D12Fence* handle = nullptr;
        HANDLE fenceEvent = INVALID_HANDLE_VALUE;
    };

    class D3D12DescriptorHeap
    {
    public:
        D3D12DescriptorHeap(D3D12GraphicsDevice* device_, D3D12_DESCRIPTOR_HEAP_TYPE type_, bool shaderVisible_);
        ~D3D12DescriptorHeap();

        void Init(uint32_t numPersistent_, uint32_t numTemporary_);
        void Shutdown();

    private:
        D3D12GraphicsDevice* device;
        const D3D12_DESCRIPTOR_HEAP_TYPE type;
        bool shaderVisible;

        uint32_t numHeaps = 0;
        uint32_t descriptorSize = 0;
        uint32_t numPersistent = 0;
        uint32_t persistentAllocated = 0;
        uint32_t numTemporary = 0;

        ID3D12DescriptorHeap* heaps[kMaxFrameLatency] = {};
        D3D12_CPU_DESCRIPTOR_HANDLE CPUStart[kMaxFrameLatency] = {};
        D3D12_GPU_DESCRIPTOR_HANDLE GPUStart[kMaxFrameLatency] = {};
    };
}
