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
        D3D11_BIND_FLAG D3D11GetBindFlags(BufferUsage usage)
        {
            if ((usage & BufferUsage::Uniform) != BufferUsage::None) {
                // This cannot be combined with nothing else.
                return D3D11_BIND_CONSTANT_BUFFER;
            }

            uint32_t result = {};
            if ((usage & BufferUsage::Vertex) != BufferUsage::None)
                result |= D3D11_BIND_VERTEX_BUFFER;
            if ((usage & BufferUsage::Index) != BufferUsage::None)
                result |= D3D11_BIND_INDEX_BUFFER;
            if ((usage & BufferUsage::Storage) != BufferUsage::None)
                result |= D3D11_BIND_UNORDERED_ACCESS;

            return (D3D11_BIND_FLAG)result;
        }
    }

    D3D11Buffer::D3D11Buffer(D3D11GraphicsDevice* device, const BufferDescription& desc, const void* initialData)
        : Buffer(desc)
        , _device(device)
    {
        Create(initialData);
    }

    D3D11Buffer::~D3D11Buffer()
    {
        Destroy();
    }

    void D3D11Buffer::Destroy()
    {
        SafeRelease(_handle);
    }

    void D3D11Buffer::Create(const void* data)
    {
        uint32_t size = _desc.size;
        if ((_desc.usage & BufferUsage::Uniform) != BufferUsage::None) {
            size = Math::AlignTo(_desc.size, _device->GetCaps().limits.minUniformBufferOffsetAlignment);
        }

        D3D11_BUFFER_DESC d3d11Desc = {};
        d3d11Desc.ByteWidth = size;
        d3d11Desc.Usage = D3D11GetUsage(_desc.memoryUsage);
        d3d11Desc.BindFlags = D3D11GetBindFlags(_desc.usage);
        d3d11Desc.CPUAccessFlags = D3D11GetCpuAccessFlags(_desc.memoryUsage);
        if ((_desc.usage & BufferUsage::Indirect) != BufferUsage::None)
        {
            d3d11Desc.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
        }

        d3d11Desc.StructureByteStride = _desc.stride;

        D3D11_SUBRESOURCE_DATA* initialDataPtr = nullptr;
        D3D11_SUBRESOURCE_DATA initialData;
        memset(&initialData, 0, sizeof(initialData));
        if (data != nullptr) {
            initialData.pSysMem = data;
            initialDataPtr = &initialData;
        }

        HRESULT hr = _device->GetD3DDevice()->CreateBuffer(&d3d11Desc, initialDataPtr, &_handle);
        if (FAILED(hr)) {
            LOG_ERROR("Direct3D11: Failed to create buffer");
        }

        BackendSetName();
    }

    void D3D11Buffer::BackendSetName()
    {
        D3D11SetObjectName(_handle, name);
    }
}
