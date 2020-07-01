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

namespace alimer
{
    namespace
    {
        inline D3D12_RESOURCE_DIMENSION D3D12GetResourceDimension(TextureType type)
        {
            switch (type)
            {
            case TextureType::Texture1D:
                return D3D12_RESOURCE_DIMENSION_TEXTURE1D;

            case TextureType::Texture2D:
            case TextureType::TextureCube:
                return D3D12_RESOURCE_DIMENSION_TEXTURE2D;

            case TextureType::Texture3D:
                return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
            default:
                ALIMER_UNREACHABLE();
                return D3D12_RESOURCE_DIMENSION_UNKNOWN;
            }
        }

        inline TextureDescription ConvertResourceDesc(D3D12_RESOURCE_DESC resourceDesc)
        {
            TextureDescription description = {};
            return description;
        }
    }

    D3D12Texture::D3D12Texture(D3D12GraphicsDevice& device, ID3D12Resource* resource, D3D12_RESOURCE_STATES state)
        : Texture(device, ConvertResourceDesc(resource->GetDesc()))
        , state{ state }
    {

    }

    D3D12Texture::D3D12Texture(D3D12GraphicsDevice& device, const TextureDescription& desc, const void* initialData)
        : Texture(device, desc)
    {
        format = ToDXGIFormatWitUsage(desc.format, desc.usage);

        D3D12MA::ALLOCATION_DESC allocationDesc = {};
        allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12GetResourceDimension(desc.type);
        resourceDesc.Alignment = 0;
        resourceDesc.Width = desc.width;
        resourceDesc.Height = desc.height;
        if (desc.type == TextureType::TextureCube)
        {
            resourceDesc.DepthOrArraySize = desc.depth * 6;
        }
        else if (desc.type == TextureType::Texture3D)
        {
            resourceDesc.DepthOrArraySize = desc.depth;
        }
        else
        {
            resourceDesc.DepthOrArraySize = desc.depth;
        }

        resourceDesc.MipLevels = desc.mipLevels;
        resourceDesc.Format = format;
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
            //allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
        }

        if (any(desc.usage & TextureUsage::Storage))
        {
            //initialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        state = initialData != nullptr ? D3D12_RESOURCE_STATE_COPY_DEST : initialState;

        HRESULT hr = device.GetAllocator()->CreateResource(
            &allocationDesc,
            &resourceDesc,
            state,
            pClearValue,
            &allocation,
            IID_PPV_ARGS(&resource)
        );

        if (FAILED(hr)) {
            LOG_ERROR("Direct3D12: Failed to create texture");
            return;
        }

        const UINT NumSubresources = max(1u, desc.depth) * max(1u, desc.mipLevels);
        device.GetD3DDevice()->GetCopyableFootprints(&resourceDesc, 0, NumSubresources, 0, nullptr, nullptr, nullptr, &sizeInBytes);

        if (initialData != nullptr)
        {
            UINT uploadPitch = (desc.width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);

            D3D12_RESOURCE_DESC resourceDesc = {};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Alignment = 0;
            resourceDesc.Width = sizeInBytes;
            resourceDesc.Height = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.SampleDesc.Quality = 0;
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

            D3D12_HEAP_PROPERTIES props = {};
            props.Type = D3D12_HEAP_TYPE_UPLOAD;

            ID3D12Resource* uploadBuffer;
            HRESULT hr = device.GetD3DDevice()->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
            ThrowIfFailed(hr);

            void* mapped = NULL;
            ThrowIfFailed(uploadBuffer->Map(0, nullptr, &mapped));
            for (uint32_t y = 0; y < desc.height; y++)
            {
                memcpy((void*)((uintptr_t)mapped + y * uploadPitch), (uint8_t*)initialData + y * desc.width * 4, desc.width * 4);
            }

            uploadBuffer->Unmap(0, nullptr);

            D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
            srcLocation.pResource = uploadBuffer;
            srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srcLocation.PlacedFootprint.Footprint.Width = desc.width;
            srcLocation.PlacedFootprint.Footprint.Height = desc.height;
            srcLocation.PlacedFootprint.Footprint.Depth = 1;
            srcLocation.PlacedFootprint.Footprint.RowPitch = uploadPitch;

            D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
            dstLocation.pResource = resource;
            dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dstLocation.SubresourceIndex = 0;

            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = resource;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

            ID3D12CommandAllocator* commandAlloc = NULL;
            hr = device.GetD3DDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAlloc));
            ThrowIfFailed(hr);

            ID3D12GraphicsCommandList* cmdList = NULL;
            hr = device.GetD3DDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAlloc, NULL, IID_PPV_ARGS(&cmdList));
            ThrowIfFailed(hr);

            cmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, NULL);
            cmdList->ResourceBarrier(1, &barrier);

            hr = cmdList->Close();
            ThrowIfFailed(hr);

            device.GetGraphicsQueue()->ExecuteCommandLists(1, (ID3D12CommandList* const*)&cmdList);
            device.WaitForGPU();

            cmdList->Release();
            commandAlloc->Release();
            uploadBuffer->Release();
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

            SRV = device.AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, false);
            device.GetD3DDevice()->CreateShaderResourceView(resource, &srvDesc, SRV);
        }
    }

    D3D12Texture::~D3D12Texture()
    {
        Destroy();
    }

    void D3D12Texture::Destroy()
    {
        SafeRelease(allocation);
        SafeRelease(resource);
    }

    void D3D12Texture::BackendSetName()
    {
        auto wideName = ToUtf16(name);
        resource->SetName(wideName.c_str());
    }

    void D3D12Texture::UploadTextureData(const Texture& texture, const void* initData)
    {
    }

    void D3D12Texture::UploadTextureData(const Texture& texture, const void* initData, ID3D12GraphicsCommandList* cmdList, ID3D12Resource* uploadResource, void* uploadCPUMem, uint64_t resourceOffset)
    {

    }
}
