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
    D3D12Texture::D3D12Texture(D3D12GraphicsDevice* device, const TextureDescription& desc, const void* initialData)
        : Texture(*device, desc)
        , dxgiFormat(ToDXGIFormat(desc.format))
    {
        D3D12MA::ALLOCATION_DESC allocationDesc = {};
        allocationDesc.HeapType = GetD3D12HeapType(GraphicsResourceUsage::Default);

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = desc.width;
        resourceDesc.Height = desc.height;
        resourceDesc.DepthOrArraySize = desc.depth;
        resourceDesc.MipLevels = desc.mipLevelCount;
        resourceDesc.Format = dxgiFormat;
        resourceDesc.SampleDesc.Count = static_cast<UINT>(desc.sampleCount);
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        if (any(desc.usage & TextureUsage::Storage))
        {
            resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        D3D12_CLEAR_VALUE clearValue = {};
        D3D12_CLEAR_VALUE* pClearValue = nullptr;
        if (any(desc.usage & TextureUsage::RenderTarget))
        {
            clearValue.Format = resourceDesc.Format;
            if (IsDepthStencilFormat(desc.format))
            {
                clearValue.DepthStencil.Depth = 1.0f;
            }

            pClearValue = &clearValue;
        }

        state = GetD3D12ResourceState(GraphicsResourceUsage::Default);
        if (any(desc.usage & TextureUsage::RenderTarget))
        {
            if (IsDepthStencilFormat(desc.format))
            {
                state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
            }
        }

        HRESULT hr = device->GetMemoryAllocator()->CreateResource(
            &allocationDesc,
            &resourceDesc,
            state,
            pClearValue,
            &allocation,
            IID_PPV_ARGS(&resource)
        );
    }

    D3D12Texture::D3D12Texture(D3D12GraphicsDevice* device, const TextureDescription& desc, ID3D12Resource* resource_, D3D12_RESOURCE_STATES currentState)
        : Texture(*device, desc)
        , D3D12GpuResource(resource_, currentState)
        , dxgiFormat(ToDXGIFormat(desc.format))
    {

    }

    D3D12Texture::~D3D12Texture()
    {
        Destroy();
    }

    void D3D12Texture::Destroy()
    {
        D3D12GpuResource::Destroy();
        SAFE_RELEASE(allocation);
    }

    D3D12Texture* D3D12Texture::CreateFromExternal(D3D12GraphicsDevice* device, ID3D12Resource* resource, PixelFormat format, D3D12_RESOURCE_STATES currentState)
    {
        const D3D12_RESOURCE_DESC& desc = resource->GetDesc();

        TextureDescription textureDesc = {};
        textureDesc.width = static_cast<u32>(desc.Width);
        textureDesc.height = desc.Height;
        textureDesc.depth = desc.DepthOrArraySize;
        textureDesc.format = format;
        return new D3D12Texture(device, textureDesc, resource, currentState);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE D3D12Texture::GetRenderTargetView(uint32_t mipLevel, uint32_t slice)
    {
        RTVInfo view = {};
        view.level = mipLevel;
        view.slice = slice;

        auto it = rtvs.find(view);
        if (it != rtvs.end())
        {
            return it->second;
        }

        D3D12GraphicsDevice* d3d12GraphicsDevice = static_cast<D3D12GraphicsDevice*>(&device);
        D3D12_CPU_DESCRIPTOR_HANDLE handle = d3d12GraphicsDevice->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1);
        d3d12GraphicsDevice->GetD3DDevice()->CreateRenderTargetView(resource, nullptr, handle);
        rtvs[view] = handle;
        return handle;
    }
}

