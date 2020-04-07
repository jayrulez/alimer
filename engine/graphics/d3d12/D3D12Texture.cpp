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
    D3D12Texture::D3D12Texture(D3D12GraphicsDevice* device, PixelFormat format_, ID3D12Resource* resource, D3D12_RESOURCE_STATES currentState)
        : Texture(device)
        , D3D12GpuResource(resource, currentState)
    {
        auto desc = resource->GetDesc();
        width = static_cast<uint32_t>(desc.Width);
        height = static_cast<uint32_t>(desc.Height);
        depth = desc.DepthOrArraySize;
        mipLevels = desc.MipLevels;
        sampleCount = desc.SampleDesc.Count;
        dimension = TextureDimension::Dimension2D;
        format = format_;
        usage = TextureUsage::None;

        if (!(desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE))
        {
            usage |= TextureUsage::Sampled;
        }

        if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
        {
            usage |= TextureUsage::Storage;
        }

        if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) ||
            desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
        {
            usage |= TextureUsage::OutputAttachment;
        }
    }

    D3D12Texture::~D3D12Texture()
    {
        Destroy();
    }

    void D3D12Texture::Destroy()
    {

    }
}


