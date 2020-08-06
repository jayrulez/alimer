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

#include "D3D11Buffer.h"
#include "D3D11GraphicsDevice.h"

namespace alimer
{
    namespace
    {
        UINT D3D11GetBindFlags(BufferUsage usage)
        {
            if (any(usage & BufferUsage::Uniform))
            {
                // This cannot be combined with nothing else.
                return D3D11_BIND_CONSTANT_BUFFER;
            }

            UINT flags = {};
            if (any(usage & BufferUsage::Index))
                flags |= D3D11_BIND_INDEX_BUFFER;
            if (any(usage & BufferUsage::Vertex))
                flags |= D3D11_BIND_VERTEX_BUFFER;
            
            if (any(usage & BufferUsage::Storage))
            {
                flags |= D3D11_BIND_SHADER_RESOURCE;
                flags |= D3D11_BIND_UNORDERED_ACCESS;
            }

            return flags;
        }
    }

    D3D11Buffer::D3D11Buffer(D3D11GraphicsDevice* device_, const BufferDescription& desc, const void* initialData)
        : Buffer(desc)
        , device(device_)
    {
        Create(initialData);
    }

    D3D11Buffer::~D3D11Buffer()
    {
        Destroy();
    }

    void D3D11Buffer::Destroy()
    {
        SafeRelease(handle);
    }

    void D3D11Buffer::Create(const void* data)
    {
        static constexpr uint64_t c_maxBytes = D3D11_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM * 1024u * 1024u;
        static_assert(c_maxBytes <= UINT32_MAX, "Exceeded integer limits");

        if (desc.size > c_maxBytes)
        {
            //DebugTrace("ERROR: Resource size too large for DirectX 11 (size %llu)\n", sizeInbytes);
            return;
        }

        uint32_t bufferSize = desc.size;
        if ((desc.usage & BufferUsage::Uniform) != BufferUsage::None)
        {
            bufferSize = Math::AlignTo(desc.size, device->GetCaps().limits.minUniformBufferOffsetAlignment);
        }

        const bool needUav = any(desc.usage & BufferUsage::Storage) || any(desc.usage & BufferUsage::Indirect);
        const bool isDynamic = data == nullptr && !needUav;

        D3D11_BUFFER_DESC d3dDesc = {};
        d3dDesc.ByteWidth = bufferSize;
        d3dDesc.BindFlags = D3D11GetBindFlags(desc.usage);

        if (any(desc.usage & BufferUsage::MapRead)) {
            d3dDesc.Usage = D3D11_USAGE_STAGING;
            d3dDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        }
        else if (any(desc.usage & BufferUsage::MapWrite))
        {
            d3dDesc.Usage = D3D11_USAGE_DYNAMIC;
            d3dDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        }
        else
        {
            d3dDesc.Usage = D3D11_USAGE_DEFAULT;
            d3dDesc.CPUAccessFlags = 0;
        }

        if (needUav)
        {
            d3dDesc.StructureByteStride = desc.stride;
        }

        if (any(desc.usage & BufferUsage::Indirect))
        {
            d3dDesc.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
        }

        D3D11_SUBRESOURCE_DATA* initialDataPtr = nullptr;
        D3D11_SUBRESOURCE_DATA initialData;
        memset(&initialData, 0, sizeof(initialData));
        if (data != nullptr) {
            initialData.pSysMem = data;
            initialDataPtr = &initialData;
        }

        HRESULT hr = device->GetD3DDevice()->CreateBuffer(&d3dDesc, initialDataPtr, &handle);
        if (FAILED(hr)) {
            LOGE("Direct3D11: Failed to create buffer");
        }
    }

    void D3D11Buffer::BackendSetName()
    {
        D3D11SetObjectName(handle, name);
    }
}
