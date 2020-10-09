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

#include "Core/Assert.h"
#include "Core/Log.h"
#include "Graphics/GraphicsResource.h"
#include "Graphics/D3D/D3DHelpers.h"

#include <d3d12.h>
#include "D3D12MemAlloc.h"

// To use graphics and CPU markup events with the latest version of PIX, change this to include <pix3.h>
// then add the NuGet package WinPixEventRuntime to the project.
#include <pix.h>

#include <queue>
#include <mutex>

#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

namespace Alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    using PFN_DXC_CREATE_INSTANCE = HRESULT(WINAPI*)(REFCLSID rclsid, REFIID riid, _COM_Outptr_ void** ppCompiler);

    extern PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface;
    extern PFN_D3D12_CREATE_DEVICE D3D12CreateDevice;
    extern PFN_DXC_CREATE_INSTANCE DxcCreateInstance;
#endif

    class D3D12GraphicsDevice;

    void D3D12SetObjectName(ID3D12Object* obj, const std::string& name);

    static constexpr D3D12_RESOURCE_STATES D3D12ResourceState(TextureLayout value)
    {
        switch (value)
        {
        case TextureLayout::Undefined:
        case TextureLayout::General:
            return D3D12_RESOURCE_STATE_COMMON;

        case TextureLayout::RenderTarget:
            return D3D12_RESOURCE_STATE_RENDER_TARGET;
        case TextureLayout::DepthStencil:
            return D3D12_RESOURCE_STATE_DEPTH_WRITE;
        case TextureLayout::DepthStencilReadOnly:
            return D3D12_RESOURCE_STATE_DEPTH_READ;
        case TextureLayout::Present:
            return D3D12_RESOURCE_STATE_PRESENT;

        default:
            return D3D12_RESOURCE_STATE_COMMON;
        }
    }

    class D3D12GpuResource
    {
        friend class D3D12CommandContext;

    public:
        D3D12GpuResource()
            : resource(nullptr)
            , usageState(D3D12_RESOURCE_STATE_COMMON)
            , transitioningState((D3D12_RESOURCE_STATES)-1)
            , gpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL)
        {}

        D3D12GpuResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES currentState)
            : resource{ resource }
            , usageState(currentState)
            , transitioningState((D3D12_RESOURCE_STATES)-1)
            , gpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL)
        {
        }

        ID3D12Resource* operator->() { return resource; }
        const ID3D12Resource* operator->() const { return resource; }
        ID3D12Resource* GetResource() { return resource; }
        const ID3D12Resource* GetResource() const { return resource; }

        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return gpuVirtualAddress; }

    protected:
        ID3D12Resource* resource;
        D3D12_RESOURCE_STATES usageState;
        D3D12_RESOURCE_STATES transitioningState;
        D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress;
    };

    class D3D12Fence
    {
    public:
        D3D12Fence(D3D12GraphicsDevice* device);
        ~D3D12Fence();

        void Shutdown();

        uint64_t GpuSignal(ID3D12CommandQueue* queue);
        void SyncGpu(ID3D12CommandQueue* queue);
        void SyncCpu();
        void SyncCpu(uint64_t value);

        uint64_t GetCpuValue() const { return cpuValue; }
        uint64_t GetGpuValue() const;

    private:
        D3D12GraphicsDevice* device;
        ID3D12Fence* handle;
        HANDLE fenceEvent;
        uint64_t cpuValue;
    };

    class D3D12CommandAllocatorPool
    {
    public:
        D3D12CommandAllocatorPool(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type);
        ~D3D12CommandAllocatorPool();
        void Shutdown();

        ID3D12CommandAllocator* RequestAllocator(uint64_t completedFenceValue);
        void DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* commandAllocator);

    private:
        ID3D12Device* device;
        const D3D12_COMMAND_LIST_TYPE type;

        std::vector<ID3D12CommandAllocator*> allocatorPool;
        std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> readyAllocators;
        std::mutex allocatorMutex;
    };

}
