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

#include "graphics/Direct3D/D3DCommon.h"

#include <d3d12.h>

#if defined(NTDDI_WIN10_RS2)
#include <dxgi1_6.h>
#else
#include <dxgi1_5.h>
#endif

// To use graphics and CPU markup events with the latest version of PIX, change this to include <pix3.h>
// then add the NuGet package WinPixEventRuntime to the project.
#include <pix.h>

#include "Core/Vector.h"

// Forward declare memory allocator classes
namespace D3D12MA
{
    class Allocator;
    class Allocation;
};

#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)
#define VHR(hr) if (FAILED(hr)) { ALIMER_ASSERT_FAIL("Failure with HRESULT of %08X", static_cast<unsigned int>(hr)); }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY2)(UINT flags, REFIID _riid, void** _factory);
typedef HRESULT(WINAPI* PFN_GET_DXGI_DEBUG_INTERFACE1)(UINT flags, REFIID _riid, void** _debug);
#endif

namespace alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    extern PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2;
    extern PFN_GET_DXGI_DEBUG_INTERFACE1 DXGIGetDebugInterface1;

    extern PFN_D3D12_CREATE_DEVICE D3D12CreateDevice;
    extern PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface;
    extern PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignature;
    extern PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER D3D12CreateRootSignatureDeserializer;
    extern PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE D3D12SerializeVersionedRootSignature;
    extern PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER D3D12CreateVersionedRootSignatureDeserializer;
#endif

    class D3D12GraphicsDevice;

    static inline D3D12_HEAP_TYPE GetD3D12HeapType(HeapType type)
    {
        switch (type)
        {
        case HeapType::Upload:
            return D3D12_HEAP_TYPE_UPLOAD;

        case HeapType::Readback:
            return D3D12_HEAP_TYPE_READBACK;

        case HeapType::Default:
        default:
            return D3D12_HEAP_TYPE_DEFAULT;
        }
    }

    D3D12_RESOURCE_STATES GetD3D12ResourceState(GraphicsResource::State state)
    {
        switch (state)
        {
        case GraphicsResource::State::Undefined:
        case GraphicsResource::State::General:
            return D3D12_RESOURCE_STATE_COMMON;
        case GraphicsResource::State::RenderTarget:
            return D3D12_RESOURCE_STATE_RENDER_TARGET;
        case GraphicsResource::State::DepthStencil:
            return D3D12_RESOURCE_STATE_DEPTH_WRITE;
        case GraphicsResource::State::DepthStencilReadOnly:
            return D3D12_RESOURCE_STATE_DEPTH_READ;
        case GraphicsResource::State::ShaderRead:
            return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        case GraphicsResource::State::ShaderWrite:
            return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        case GraphicsResource::State::CopyDest:
            return D3D12_RESOURCE_STATE_COPY_DEST;
        case GraphicsResource::State::CopySource:
            return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case GraphicsResource::State::Present:
            return D3D12_RESOURCE_STATE_PRESENT;

        default:
            ALIMER_UNREACHABLE();
            return D3D12_RESOURCE_STATE_GENERIC_READ;
        }
    };

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
        Vector<uint32_t> deadList;

        ID3D12DescriptorHeap* heaps[kMaxFrameLatency] = {};
        D3D12_CPU_DESCRIPTOR_HANDLE CPUStart[kMaxFrameLatency] = {};
        D3D12_GPU_DESCRIPTOR_HANDLE GPUStart[kMaxFrameLatency] = {};

        SRWLOCK lock = SRWLOCK_INIT;
        uint32_t heapIndex = 0;
    };
}
