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

#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

namespace Alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    using PFN_DXC_CREATE_INSTANCE = HRESULT(WINAPI*)(REFCLSID rclsid, REFIID riid, _COM_Outptr_ void** ppCompiler);
    extern PFN_DXC_CREATE_INSTANCE DxcCreateInstance;
#endif

    class D3D12GraphicsDevice;

    void D3D12SetObjectName(ID3D12Object* obj, const std::string& name);

    struct D3D12Fence
    {
        D3D12GraphicsDevice* device = nullptr;
        ID3D12Fence* d3dFence = nullptr;
        HANDLE fenceEvent = INVALID_HANDLE_VALUE;

        ~D3D12Fence();

        void Init(D3D12GraphicsDevice* device, uint64 initialValue = 0);
        void Shutdown();

        void Signal(ID3D12CommandQueue* queue, uint64 fenceValue);
        void Wait(uint64 fenceValue);
        bool IsSignaled(uint64 fenceValue);
        void Clear(uint64 fenceValue);
    };

    struct D3D12DescriptorHeap
    {
        ID3D12DescriptorHeap* Heap;
        uint32 DescriptorSize;
        D3D12_CPU_DESCRIPTOR_HANDLE CPUStart;
        D3D12_GPU_DESCRIPTOR_HANDLE GPUStart;
        uint32 Size;
        uint32 Capacity;
    };

}
