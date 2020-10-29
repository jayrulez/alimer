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

#include "Graphics/Texture.h"
#include "D3D12GraphicsDevice.h"

namespace Alimer
{
    namespace
    {
        inline D3D12_RESOURCE_DIMENSION D3D12GetResourceDimension(TextureType type)
        {
            switch (type)
            {
            case TextureType::Type2D:
            case TextureType::TypeCube:
                return D3D12_RESOURCE_DIMENSION_TEXTURE2D;

            case TextureType::Type3D:
                return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
            default:
                ALIMER_UNREACHABLE();
                return D3D12_RESOURCE_DIMENSION_UNKNOWN;
            }
        }

        inline TextureType D3D12GetTextureType(D3D12_RESOURCE_DIMENSION dimension)
        {
            switch (dimension)
            {
            case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
                return TextureType::Type2D;

            case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
                return TextureType::Type3D;

            default:
                ALIMER_UNREACHABLE();
                return TextureType::Type2D;
            }
        }

        inline TextureDescription ConvertResourceDesc(D3D12_RESOURCE_DESC resourceDesc, TextureLayout initialLayout)
        {
            TextureDescription description = {};
            description.type = D3D12GetTextureType(resourceDesc.Dimension);
            description.width = resourceDesc.Width;
            description.height = resourceDesc.Height;
            description.depthOrArraySize = resourceDesc.DepthOrArraySize;
            description.initialLayout = initialLayout;
            return description;
        }
    }

    Texture::Texture(GraphicsDevice& device, TextureHandle handle, const Extent3D& extent, PixelFormat format, TextureLayout layout, TextureUsage usage, TextureSampleCount sampleCount)
        : GraphicsResource(device, Type::Texture)
        , handle{ handle }
        , allocation{}
        , layout{ layout }
        , apiFormat(ToDXGIFormatWitUsage(format, usage))
    {

    }

    Texture::Texture(GraphicsDevice& device, const TextureDescription& desc)
        : GraphicsResource(device, Type::Texture)
        , desc{ desc }
        , layout(desc.initialLayout)
        , apiFormat(ToDXGIFormatWitUsage(desc.format, desc.usage))
    {
        D3D12MA::ALLOCATION_DESC allocationDesc = {};
        allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12GetResourceDimension(desc.type);
        resourceDesc.Alignment = 0;
        resourceDesc.Width = desc.width;
        resourceDesc.Height = desc.height;
        if (desc.type == TextureType::TypeCube)
        {
            resourceDesc.DepthOrArraySize = desc.depthOrArraySize * 6;
        }
        else
        {
            resourceDesc.DepthOrArraySize = desc.depthOrArraySize;
        }

        resourceDesc.MipLevels = desc.mipLevels;
        resourceDesc.Format = apiFormat;
        resourceDesc.SampleDesc.Count = desc.sampleCount;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12_CLEAR_VALUE clearValue = {};
        D3D12_CLEAR_VALUE* pClearValue = nullptr;

        D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
        if (any(desc.usage & TextureUsage::RenderTarget))
        {
            // Render and Depth/Stencil targets are always committed resources
            allocationDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;

            clearValue.Format = resourceDesc.Format;
            if (IsDepthStencilFormat(desc.format))
            {
                initialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
                resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

                if (!any(desc.usage & TextureUsage::Sampled))
                {
                    resourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
                }

                clearValue.DepthStencil.Depth = 1.0f;
            }
            else
            {
                initialState = D3D12_RESOURCE_STATE_RENDER_TARGET;
                resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            }

            pClearValue = &clearValue;
        }

        if (any(desc.usage & TextureUsage::Storage))
        {
            //initialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        // usageState = initialData != nullptr ? D3D12_RESOURCE_STATE_COPY_DEST : initialState;

         /*HRESULT hr = device->GetAllocator()->CreateResource(
             &allocationDesc,
             &resourceDesc,
             state,
             pClearValue,
             &allocation,
             IID_PPV_ARGS(&handle)
         );

         if (FAILED(hr))
         {
             LOGE("Direct3D12: Failed to create texture");
             return;
         }*/

         /*const UINT NumSubresources = max(1u, desc.depth) * max(1u, desc.mipLevels);
         device->GetD3DDevice()->GetCopyableFootprints(&resourceDesc, 0, NumSubresources, 0, nullptr, nullptr, nullptr, &sizeInBytes);

         if (initialData != nullptr)
         {
             UploadTextureData(initialData);
         }

         if (!IsDepthStencilFormat(desc.format))
         {
             D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
             srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
             srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
             srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
             srvDesc.Texture2D.MostDetailedMip = 0;
             srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;
             srvDesc.Texture2D.PlaneSlice = 0;
             srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

             SRV = device->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, false);
             device->GetD3DDevice()->CreateShaderResourceView(resource, &srvDesc, SRV);
         }*/
    }

    Texture::~Texture()
    {
        Destroy();
    }

    void Texture::Destroy()
    {
        //device.ReleaseResource(resource);
        SafeRelease(allocation);
    }

    void Texture::SetName(const std::string& newName)
    {
        auto wideName = ToUtf16(newName);
        handle->SetName(wideName.c_str());
    }
}
