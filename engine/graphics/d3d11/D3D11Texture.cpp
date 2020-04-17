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

#include "D3D11Texture.h"
#include "D3D11GraphicsDevice.h"

namespace alimer
{
    D3D11Texture::D3D11Texture(GraphicsDevice* device, const TextureDescriptor* descriptor)
        : Texture(*device, descriptor)
        , handle{}
    {
        dxgiFormat = ToDXGIFormatWithUsage(descriptor->format, descriptor->usage);
        if (external)
        {
            ID3D11Resource* externalHandle = (ID3D11Resource*)descriptor->externalHandle;
            handle.resource = externalHandle;
            handle.resource->AddRef();
        }
        else
        {
            auto d3dDevice = static_cast<D3D11GraphicsDevice*>(device)->GetD3DDevice();
            UINT bindFlags = ToD3D11BindFlags(usage, IsDepthStencilFormat(format));

            if (type == TextureType::Type3D)
            {
                D3D11_TEXTURE3D_DESC d3d11Desc = {};
                d3d11Desc.BindFlags = bindFlags;
                d3d11Desc.CPUAccessFlags = 0;
                d3d11Desc.Format = dxgiFormat;
                d3d11Desc.MiscFlags = 0;
                d3d11Desc.MipLevels = mipLevels;
                d3d11Desc.Width = extent.width;
                d3d11Desc.Height = extent.height;
                d3d11Desc.Depth = extent.depth;
                d3d11Desc.Usage = D3D11_USAGE_DEFAULT;

                ThrowIfFailed(d3dDevice->CreateTexture3D(&d3d11Desc, nullptr, &handle.tex3d));
            }
            else
            {
                const uint32_t multiplier = (type == TextureType::TypeCube) ? 6 : 1;

                D3D11_TEXTURE2D_DESC d3d11Desc = {};

                d3d11Desc.Width = extent.width;
                d3d11Desc.Height = extent.height;
                d3d11Desc.MipLevels = mipLevels;
                d3d11Desc.ArraySize = extent.depth * multiplier;
                d3d11Desc.Format = dxgiFormat;
                d3d11Desc.SampleDesc.Count = (UINT)sampleCount;
                d3d11Desc.SampleDesc.Quality = sampleCount > TextureSampleCount::Count1 ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0;

                d3d11Desc.Usage = D3D11_USAGE_DEFAULT;
                d3d11Desc.BindFlags = bindFlags;
                d3d11Desc.CPUAccessFlags = 0;
                d3d11Desc.MiscFlags = 0;

                if (type == TextureType::TypeCube)
                {
                    d3d11Desc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
                }

                ThrowIfFailed(d3dDevice->CreateTexture2D(&d3d11Desc, NULL, &handle.tex2d));
            }
        }
    }

    D3D11Texture::~D3D11Texture()
    {
        Destroy();
    }

    void D3D11Texture::Destroy()
    {
        if (handle.resource != nullptr)
        {
            handle.resource->Release();
            handle.resource = nullptr;
        }
    }
}
