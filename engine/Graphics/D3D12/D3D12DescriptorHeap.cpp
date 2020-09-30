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

#if TODO
#include "D3D12DescriptorHeap.h"
#include "D3D12GraphicsDevice.h"

namespace Alimer
{
    D3D12DescriptorHeap::D3D12DescriptorHeap(D3D12GraphicsDevice* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 numDescriptorsPerHeap)
        : device{ device }
        , type{ type }
        , numDescriptorsPerHeap{ numDescriptorsPerHeap }
    {
    }

    D3D12DescriptorHeap::~D3D12DescriptorHeap()
    {
    }

    D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::Allocate(uint32_t count)
    {
        if (currentHeap == nullptr || remainingFreeHandles < count)
        {
            D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
            heapDesc.Type = type;
            heapDesc.NumDescriptors = numDescriptorsPerHeap;
            heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            heapDesc.NodeMask = 1;

            Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
            ThrowIfFailed(device->GetD3DDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(heap.GetAddressOf())));
            descriptorHeaps.emplace_back(heap);

            currentHeap = heap.Get();
            currentHandle = currentHeap->GetCPUDescriptorHandleForHeapStart();
            remainingFreeHandles = numDescriptorsPerHeap;

            if (descriptorSize == 0)
            {
                descriptorSize = device->GetD3DDevice()->GetDescriptorHandleIncrementSize(type);
            }
        }

        D3D12_CPU_DESCRIPTOR_HANDLE ret = currentHandle;
        currentHandle.ptr += (SIZE_T)count * descriptorSize;
        remainingFreeHandles -= count;
        return ret;
    }
}

#endif // TODO
