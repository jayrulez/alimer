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

namespace alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    PFN_D3D12_CREATE_DEVICE D3D12CreateDevice;
    PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface;
    PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignature;
    PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER D3D12CreateRootSignatureDeserializer;
    PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE D3D12SerializeVersionedRootSignature;
    PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER D3D12CreateVersionedRootSignatureDeserializer;
#endif

    DescriptorHeap D3D12CreateDescriptorHeap(ID3D12Device* device, uint32_t capacity, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
    {
        ALIMER_ASSERT(device && capacity > 0);

        DescriptorHeap descriptorHeap = {};
        descriptorHeap.Size = 0;
        descriptorHeap.Capacity = capacity;

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = capacity;
        desc.Type = type;
        desc.Flags = flags;
        ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap.handle)));

        descriptorHeap.CpuStart = descriptorHeap.handle->GetCPUDescriptorHandleForHeapStart();
        if (flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
        {
            descriptorHeap.GpuStart = descriptorHeap.handle->GetGPUDescriptorHandleForHeapStart();
        }

        descriptorHeap.DescriptorSize = device->GetDescriptorHandleIncrementSize(type);
        return descriptorHeap;
    }
}
