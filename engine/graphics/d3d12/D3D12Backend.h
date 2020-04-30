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

// To use graphics and CPU markup events with the latest version of PIX, change this to include <pix3.h>
// then add the NuGet package WinPixEventRuntime to the project.
#include <pix.h>

#include <wrl.h>
#include <vector>

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

    class FenceD3D12
    {
    public:
        FenceD3D12(D3D12GraphicsDevice* device);
        ~FenceD3D12();

        void Init(uint64_t initialValue = 0);
        void Shutdown();
        void Signal(ID3D12CommandQueue* queue, uint64_t fenceValue);
        void Wait(uint64_t fenceValue);
        void GPUWait(ID3D12CommandQueue* queue, uint64_t fenceValue);
        bool IsSignaled(uint64_t fenceValue);
        void Clear(uint64_t fenceValue);

    private:
        D3D12GraphicsDevice* device;
        ID3D12Fence* handle = nullptr;
        HANDLE fenceEvent = INVALID_HANDLE_VALUE;
    };

    struct PersistentDescriptorAlloc
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handles[kMaxFrameLatency] = {};
        uint32_t index = uint32_t(-1);
    };

    class D3D12DescriptorHeap
    {
    public:
        D3D12DescriptorHeap(D3D12GraphicsDevice* device_, D3D12_DESCRIPTOR_HEAP_TYPE type_, bool shaderVisible_);
        ~D3D12DescriptorHeap();

        void Init(uint32_t numPersistent_, uint32_t numTemporary_);
        void Shutdown();
        PersistentDescriptorAlloc AllocatePersistent();
        void FreePersistent(uint32_t& index);
        void FreePersistent(D3D12_CPU_DESCRIPTOR_HANDLE& handle);
        void FreePersistent(D3D12_GPU_DESCRIPTOR_HANDLE& handle);

        uint32_t IndexFromHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle);
        uint32_t IndexFromHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle);
        uint32_t TotalNumDescriptors() const { return numPersistent + numTemporary; }

    private:
        D3D12GraphicsDevice* device;
        const D3D12_DESCRIPTOR_HEAP_TYPE type;
        bool shaderVisible;

        uint32_t numHeaps = 0;
        uint32_t descriptorSize = 0;
        uint32_t numPersistent = 0;
        uint32_t persistentAllocated = 0;
        uint32_t numTemporary = 0;
        std::vector<uint32_t> deadList;

        ID3D12DescriptorHeap* heaps[kMaxFrameLatency] = {};
        D3D12_CPU_DESCRIPTOR_HANDLE CPUStart[kMaxFrameLatency] = {};
        D3D12_GPU_DESCRIPTOR_HANDLE GPUStart[kMaxFrameLatency] = {};

        SRWLOCK lock = SRWLOCK_INIT;
        uint32_t heapIndex = 0;
    };
}
