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

#include "Graphics/Buffer.h"
#include "D3D12GraphicsDevice.h"
#include "D3D12MemAlloc.h"

namespace alimer
{
    namespace
    {
        D3D12_HEAP_TYPE D3D12HeapType(BufferUsage usage)
        {
            if (any(usage & BufferUsage::MapRead)) {
                return D3D12_HEAP_TYPE_READBACK;
            }
            else if (any(usage & BufferUsage::MapWrite)) {
                return D3D12_HEAP_TYPE_UPLOAD;
            }
            else {
                return D3D12_HEAP_TYPE_DEFAULT;
            }
        }
    }

    void Buffer::Destroy()
    {
        SafeRelease(allocation);
    }

    bool Buffer::BackendCreate(const void* data)
    {
        uint32_t alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        if (any(usage & BufferUsage::Uniform))
        {
            alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
        }
        uint32_t alignedSize = Math::AlignTo(size, alignment);

        D3D12_RESOURCE_DESC resourceDesc;
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = alignedSize;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        if (any(usage & BufferUsage::Storage))
        {
            resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        D3D12MA::ALLOCATION_DESC allocDesc = {};
        allocDesc.HeapType = D3D12HeapType(usage);

        //state = initialData != nullptr ? D3D12_RESOURCE_STATE_COPY_DEST : GetD3D12ResourceState(desc.memoryUsage);

        /*HRESULT hr = device->GetAllocator()->CreateResource(
            &allocDesc,
            &resourceDesc,
            state,
            nullptr,
            &allocation,
            IID_PPV_ARGS(&resource)
        );

        if (FAILED(hr)) {
            LOG_ERROR("Direct3D12: Failed to create buffer");
        }

        gpuVirtualAddress = resource->GetGPUVirtualAddress();*/
        return true;
    }

    void Buffer::BackendSetName()
    {
#ifndef NDEBUG
        auto wideName = ToUtf16(name);
        handle->SetName(wideName.c_str());
#endif
    }
}
