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
#include "D3D12Texture.h"
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

    D3D12Texture::D3D12Texture(D3D12GraphicsDevice* device, ID3D12Resource* resource, TextureLayout initialLayout)
        : Texture(ConvertResourceDesc(resource->GetDesc(), initialLayout))
        , device{ device }
        , resource{ resource }
    {
        RTV = device->AllocateCpuDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1u);
        device->GetD3DDevice()->CreateRenderTargetView(resource, nullptr, RTV);
    }

    D3D12Texture::D3D12Texture(D3D12GraphicsDevice* device, const TextureDescription& desc, const void* initialData)
        : Texture(desc)
        , device{ device }
    {
        const DXGI_FORMAT dxgiFormat = ToDXGIFormatWitUsage(desc.format, desc.usage);

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
        resourceDesc.Format = dxgiFormat;
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

    D3D12Texture::~D3D12Texture()
    {
        Destroy();
    }

    void D3D12Texture::Destroy()
    {
        device->ReleaseResource(resource);
        SafeRelease(allocation);
    }

    void D3D12Texture::BackendSetName()
    {
        auto wideName = ToUtf16(name);
        resource->SetName(wideName.c_str());
    }

    void D3D12Texture::SetLayout(TextureLayout newLayout)
    {
        layout = newLayout;
    }

    void D3D12Texture::UploadTextureData(const void* initData)
    {
        // Get a GPU upload buffer
        /*UploadContext uploadContext = _device->ResourceUploadBegin(sizeInBytes);

        UploadTextureData(initData, uploadContext.commandList, uploadContext.Resource, uploadContext.CPUAddress, uploadContext.ResourceOffset);

        _device->ResourceUploadEnd(uploadContext);*/
    }

    void D3D12Texture::UploadTextureData(const void* initData, ID3D12GraphicsCommandList* cmdList, ID3D12Resource* uploadResource, void* uploadCPUMem, uint64_t resourceOffset)
    {
        /*D3D12_RESOURCE_DESC textureDesc = resource->GetDesc();
        const uint64_t arraySize = _desc.type == TextureType::TextureCube ? _desc.depth * 6 : _desc.depth;

        const uint64_t numSubResources = max(1u, _desc.mipLevels) * arraySize;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT* layouts = (D3D12_PLACED_SUBRESOURCE_FOOTPRINT*)_alloca(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) * numSubResources);
        uint32_t* numRows = (uint32_t*)_alloca(sizeof(uint32_t) * numSubResources);
        uint64_t* rowSizes = (uint64_t*)_alloca(sizeof(uint64_t) * numSubResources);

        uint64_t textureMemSize = 0;
        _device->GetD3DDevice()->GetCopyableFootprints(&textureDesc, 0, uint32_t(numSubResources), 0, layouts, numRows, rowSizes, &textureMemSize);

        // Get a GPU upload buffer
        uint8_t* uploadMem = reinterpret_cast<uint8_t*>(uploadCPUMem);

        const uint8_t* srcMem = reinterpret_cast<const uint8_t*>(initData);
        const uint64_t srcTexelSize = GetFormatBitsPerPixel(_desc.format) / 8;

        for (uint64_t arrayIdx = 0; arrayIdx < arraySize; ++arrayIdx)
        {
            uint64_t mipWidth = _desc.width;
            for (uint64_t mipIdx = 0; mipIdx < _desc.mipLevels; ++mipIdx)
            {
                const uint64_t subResourceIdx = mipIdx + (arrayIdx * _desc.mipLevels);

                const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& subResourceLayout = layouts[subResourceIdx];
                const uint64_t subResourceHeight = numRows[subResourceIdx];
                const uint64_t subResourcePitch = subResourceLayout.Footprint.RowPitch;
                const uint64_t subResourceDepth = subResourceLayout.Footprint.Depth;
                const uint64_t srcPitch = mipWidth * srcTexelSize;
                uint8_t* dstSubResourceMem = uploadMem + subResourceLayout.Offset;

                for (uint64_t z = 0; z < subResourceDepth; ++z)
                {
                    for (uint64_t y = 0; y < subResourceHeight; ++y)
                    {
                        memcpy(dstSubResourceMem, srcMem, min(subResourcePitch, srcPitch));
                        dstSubResourceMem += subResourcePitch;
                        srcMem += srcPitch;
                    }
                }

                mipWidth = max(mipWidth / 2, 1ull);
            }
        }

        for (uint64_t subResourceIdx = 0; subResourceIdx < numSubResources; ++subResourceIdx)
        {
            D3D12_TEXTURE_COPY_LOCATION dst = {};
            dst.pResource = resource;
            dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dst.SubresourceIndex = uint32_t(subResourceIdx);
            D3D12_TEXTURE_COPY_LOCATION src = {};
            src.pResource = uploadResource;
            src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            src.PlacedFootprint = layouts[subResourceIdx];
            src.PlacedFootprint.Offset += resourceOffset;
            cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        }*/
    }
}

#endif // TODO
