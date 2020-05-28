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

#include "D3D12Texture.h"
#include "D3D12GraphicsDevice.h"
#include "D3D12MemAlloc.h"

namespace alimer
{
    D3D12Texture::D3D12Texture(D3D12GraphicsDevice& device, const TextureDescriptor* descriptor, const void* initialData)
        : Texture(device, descriptor, State::Undefined)
    {
        D3D12MA::ALLOCATION_DESC allocationDesc = {};
        allocationDesc.HeapType = GetD3D12HeapType(heapType);

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = descriptor->width;
        resourceDesc.Height = descriptor->height;
        resourceDesc.DepthOrArraySize = descriptor->depth;
        resourceDesc.MipLevels = descriptor->mipLevels;
        resourceDesc.Format = ToDXGIFormat(descriptor->format);
        resourceDesc.SampleDesc.Count = static_cast<UINT>(descriptor->sampleCount);
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        if (any(descriptor->usage & TextureUsage::Storage))
        {
            resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        D3D12_CLEAR_VALUE clearValue = {};
        D3D12_CLEAR_VALUE* pClearValue = nullptr;
        if (any(descriptor->usage & TextureUsage::OutputAttachment))
        {
            clearValue.Format = resourceDesc.Format;
            if (IsDepthStencilFormat(descriptor->format))
            {
                clearValue.DepthStencil.Depth = 1.0f;
            }

            pClearValue = &clearValue;
        }

        D3D12_RESOURCE_STATES resourceStates = GetD3D12ResourceState(state);

        if (heapType == HeapType::Upload)
        {
            resourceStates = D3D12_RESOURCE_STATE_GENERIC_READ;
        }
        else if (heapType == HeapType::Readback)
        {
            resourceStates = D3D12_RESOURCE_STATE_COPY_DEST;
        }

        HRESULT hr = device.GetMemoryAllocator()->CreateResource(
            &allocationDesc,
            &resourceDesc,
            resourceStates,
            pClearValue,
            &allocation,
            IID_PPV_ARGS(&resource)
        );
    }

    D3D12Texture::D3D12Texture(GraphicsDevice& device, const TextureDescriptor* descriptor, ID3D12Resource* resource_, State state_)
        : Texture(device, descriptor, state_)
        , resource(resource_)
    {

    }

    D3D12Texture::~D3D12Texture()
    {
        Destroy();
    }

    void D3D12Texture::Destroy()
    {
        SAFE_RELEASE(resource);
        SAFE_RELEASE(allocation);
    }

    D3D12Texture* D3D12Texture::CreateFromExternal(GraphicsDevice& device, ID3D12Resource* resource, GraphicsResource::State state)
    {
        const D3D12_RESOURCE_DESC& desc = resource->GetDesc();

        TextureDescriptor textureDesc = {};
        textureDesc.width = static_cast<u32>(desc.Width);
        textureDesc.height = desc.Height;
        textureDesc.depth = desc.DepthOrArraySize;
        return new D3D12Texture(device, &textureDesc, resource, state);
    }
}

