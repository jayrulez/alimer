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

#include "D3D12Texture.h"
#include "D3D12MemAlloc.h"
#include "Graphics_D3D12.h"
#include "Math/MathHelper.h"

namespace alimer
{
    namespace
    {
        inline TextureDescription ConvertDescription(ID3D12Resource* resource)
        {
            auto d3dDesc = resource->GetDesc();
            TextureDescription description{};
            description.width = (uint32_t)d3dDesc.Width;
            description.height = d3dDesc.Height;

            switch (d3dDesc.Dimension)
            {
            case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
                description.type = TextureType::Type1D;
                description.arrayLayers = d3dDesc.DepthOrArraySize;
                break;
            default:
            case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
                description.type = TextureType::Type2D;
                description.arrayLayers = d3dDesc.DepthOrArraySize;
                break;
            case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
                description.type = TextureType::Type3D;
                description.depth = d3dDesc.DepthOrArraySize;
                break;
            }

            description.mipLevels = d3dDesc.MipLevels;
            description.format = PixelFormatFromDXGIFormat(d3dDesc.Format);
            description.usage = TextureUsage::Sampled;
            description.type = TextureType::Type2D;
            description.sampleCount = (TextureSampleCount)d3dDesc.SampleDesc.Count;
            description.layout = IMAGE_LAYOUT_GENERAL;
            return description;
        }
    }

    D3D12Texture::D3D12Texture(D3D12GraphicsDevice* device_, ID3D12Resource* resource_)
        : Texture(ConvertDescription(resource_))
        , device(device_)
        , resource(resource_)
        , allocation(nullptr)
    {
        rtv = {};
        rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        auto d3dDesc = resource->GetDesc();
        device->GetD3DDevice()->GetCopyableFootprints(&d3dDesc, 0, 1, 0, &footprint, nullptr, nullptr, nullptr);
    }

    D3D12Texture::D3D12Texture(D3D12GraphicsDevice* device_, const TextureDescription& description)
        : Texture(description)
        , device(device_)
    {
    }

    D3D12Texture::~D3D12Texture()
    {
        Destroy();
    }

    void D3D12Texture::Destroy()
    {
        if (resource)
        {
            device->DeferredRelease(resource);
        }

        if (allocation)
        {
            device->DeferredRelease(allocation);
        }
    }

    bool D3D12Texture::Init(bool hasInitData)
    {
        // const uint32_t arrayMultiplier = (description->type == TextureType::TypeCube) ? 6 : 1;

        HRESULT hr = E_FAIL;

        D3D12MA::ALLOCATION_DESC allocationDesc = {};
        allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resourceDesc{};
        switch (description.type)
        {
        case TextureType::Type1D:
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
            break;
        case TextureType::Type2D:
        case TextureType::TypeCube:
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            break;
        case TextureType::Type3D:
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
            break;
        default:
            ALIMER_UNREACHABLE();
            break;
        }

        resourceDesc.Alignment = 0;
        resourceDesc.Width = AlignTo(description.width, GetFormatBlockWidth(description.format));
        resourceDesc.Height = AlignTo(description.width, GetFormatBlockHeight(description.format));
        switch (description.type)
        {
        case TextureType::Type1D:
        case TextureType::Type2D:
            resourceDesc.DepthOrArraySize = (UINT16)description.arrayLayers;
            break;
        case TextureType::TypeCube:
            resourceDesc.DepthOrArraySize = (UINT16)(description.arrayLayers * 6);
            break;
        case TextureType::Type3D:
            resourceDesc.DepthOrArraySize = (UINT16)description.depth;
            break;
        default:
            ALIMER_UNREACHABLE();
            break;
        }

        resourceDesc.MipLevels = description.mipLevels;
        if (IsDepthFormat(description.format) &&
            any(description.usage & (TextureUsage::Sampled | TextureUsage::Storage)))
        {
            resourceDesc.Format = GetTypelessFormatFromDepthFormat(description.format);
        }
        else
        {
            resourceDesc.Format = PixelFormatToDXGIFormat(description.format);
        }

        resourceDesc.SampleDesc.Count = static_cast<UINT>(description.sampleCount);
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        const bool isDepthStencil = IsDepthStencilFormat(description.format);

        if (any(description.usage & TextureUsage::RenderTarget) && isDepthStencil)
        {
            resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            allocationDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;
            if (!any(description.usage & TextureUsage::Sampled))
            {
                resourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
            }
        }
        else if (resourceDesc.SampleDesc.Count == 1)
        {
            resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
        }

        if (any(description.usage & TextureUsage::RenderTarget) && !isDepthStencil)
        {
            resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            allocationDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;
        }

        if (any(description.usage & TextureUsage::Storage))
        {
            resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        D3D12_CLEAR_VALUE optimizedClearValue = {};
        optimizedClearValue.Color[0] = description.clear.color[0];
        optimizedClearValue.Color[1] = description.clear.color[1];
        optimizedClearValue.Color[2] = description.clear.color[2];
        optimizedClearValue.Color[3] = description.clear.color[3];
        optimizedClearValue.DepthStencil.Depth = description.clear.depthstencil.depth;
        optimizedClearValue.DepthStencil.Stencil = description.clear.depthstencil.stencil;
        optimizedClearValue.Format = resourceDesc.Format;
        if (optimizedClearValue.Format == DXGI_FORMAT_R16_TYPELESS)
        {
            optimizedClearValue.Format = DXGI_FORMAT_D16_UNORM;
        }
        else if (optimizedClearValue.Format == DXGI_FORMAT_R32_TYPELESS)
        {
            optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        }
        else if (optimizedClearValue.Format == DXGI_FORMAT_R32G8X24_TYPELESS)
        {
            optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        }

        bool useClearValue = any(description.usage & TextureUsage::RenderTarget);

        D3D12_RESOURCE_STATES resourceState = _ConvertImageLayout(description.layout);

        if (description.Usage == USAGE_STAGING)
        {
            UINT64 RequiredSize = 0;
            device->GetD3DDevice()->GetCopyableFootprints(&resourceDesc, 0, 1, 0, &footprint, nullptr, nullptr,
                                                          &RequiredSize);
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Width = RequiredSize;
            resourceDesc.Height = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

            if (description.CPUAccessFlags & CPU_ACCESS_READ)
            {
                allocationDesc.HeapType = D3D12_HEAP_TYPE_READBACK;
                resourceState = D3D12_RESOURCE_STATE_COPY_DEST;
            }
            else
            {
                allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
                resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
            }
        }

        hr = device->GetAllocator()->CreateResource(&allocationDesc, &resourceDesc, resourceState,
                                                    useClearValue ? &optimizedClearValue : nullptr, &allocation,
                                                    IID_PPV_ARGS(&resource));

        if (FAILED(hr))
        {
            return false;
        }

        if (any(description.usage & TextureUsage::RenderTarget))
        {
            if (isDepthStencil)
            {
                device->CreateSubresource(this, DSV, 0, -1, 0, -1);
            }
            else
            {
                device->CreateSubresource(this, RTV, 0, -1, 0, -1);
            }
        }

        if (any(description.usage & TextureUsage::Sampled))
        {
            device->CreateSubresource(this, SRV, 0, -1, 0, -1);
        }
        if (any(description.usage & TextureUsage::Storage))
        {
            device->CreateSubresource(this, UAV, 0, -1, 0, -1);
        }

        return true;
    }
}
