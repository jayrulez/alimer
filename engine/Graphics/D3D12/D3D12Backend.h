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
#include "graphics/D3D/D3DHelpers.h"
#include "d3d12.h"

// To use graphics and CPU markup events with the latest version of PIX, change this to include <pix3.h>
// then add the NuGet package WinPixEventRuntime to the project.
#include <pix.h>

#ifdef _DEBUG
#   include <dxgidebug.h>
#   pragma comment(lib,"dxguid.lib")
#endif

#include <wrl/client.h>
#include <stdexcept>

// Forward declare memory allocator classes
namespace D3D12MA
{
    class Allocator;
    class Allocation;
};

#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

#define VHR(hr) if (FAILED(hr)) { ALIMER_ASSERT_FAIL("Failure with HRESULT of %08X", static_cast<unsigned int>(hr)); }
#define SAFE_RELEASE(obj) if ((obj)) { obj->Release(); (obj) = nullptr; }

namespace alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    extern PFN_D3D12_CREATE_DEVICE D3D12CreateDevice;
    extern PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface;
#endif

    static inline D3D12_COMMAND_LIST_TYPE GetD3D12CommandListType(CommandQueueType queueType)
    {
        switch (queueType)
        {
        case CommandQueueType::Compute:
            return D3D12_COMMAND_LIST_TYPE_COMPUTE;

        case CommandQueueType::Copy:
            return D3D12_COMMAND_LIST_TYPE_COPY;

        default:
            return D3D12_COMMAND_LIST_TYPE_DIRECT;
        }
    }

    static inline D3D12_HEAP_TYPE GetD3D12HeapType(GraphicsResourceUsage usage)
    {
        switch (usage)
        {
        case GraphicsResourceUsage::Default:
        case GraphicsResourceUsage::Immutable:
            return D3D12_HEAP_TYPE_DEFAULT;

        case GraphicsResourceUsage::Dynamic:
            return D3D12_HEAP_TYPE_UPLOAD;

        case GraphicsResourceUsage::Staging:
            return D3D12_HEAP_TYPE_READBACK;

        default:
            ALIMER_UNREACHABLE();
        }
    }

    static inline D3D12_RESOURCE_STATES GetD3D12ResourceState(GraphicsResourceUsage usage)
    {
        switch (usage)
        {
        case GraphicsResourceUsage::Default:
        case GraphicsResourceUsage::Immutable:
            return D3D12_RESOURCE_STATE_COMMON;

        case GraphicsResourceUsage::Dynamic:
            return D3D12_RESOURCE_STATE_GENERIC_READ;

        case GraphicsResourceUsage::Staging:
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
            SAFE_RELEASE(resource);
            gpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
        }

        D3D12_RESOURCE_STATES GetState() const { return state; }
        void SetState(D3D12_RESOURCE_STATES newState) { state = newState; }

        D3D12_RESOURCE_STATES GetTransitioningState() const { return transitioningState; }
        void SetTransitioningState(D3D12_RESOURCE_STATES newState) { transitioningState = newState; }

        ID3D12Resource* operator->() { return resource; }
        const ID3D12Resource* operator->() const { return resource; }

        ID3D12Resource* GetResource() { return resource; }
        const ID3D12Resource* GetResource() const { return resource; }

        
        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return gpuVirtualAddress; }

    protected:
        ID3D12Resource* resource;
        D3D12_RESOURCE_STATES state;
        D3D12_RESOURCE_STATES transitioningState;
        D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress;
    };
}
