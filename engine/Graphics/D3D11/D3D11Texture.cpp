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
    namespace
    {
        inline D3D11_RTV_DIMENSION D3D11GetResourceDimension(TextureType type, bool isArray)
        {
            switch (type)
            {
            //case TextureType::Texture1D:
            //    return (isArray) ? D3D11_RTV_DIMENSION_TEXTURE1DARRAY : D3D11_RTV_DIMENSION_TEXTURE1D;
            case TextureType::Texture2D:
                return (isArray) ? D3D11_RTV_DIMENSION_TEXTURE2DARRAY : D3D11_RTV_DIMENSION_TEXTURE2D;
            case TextureType::Texture3D:
                ALIMER_ASSERT(isArray == false);
                return D3D11_RTV_DIMENSION_TEXTURE3D;
            //case TextureType::Texture2DMultisample:
            //    return (isArray) ? D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY : D3D11_RTV_DIMENSION_TEXTURE2DMS;
            case TextureType::TextureCube:
                return D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            default:
                ALIMER_UNREACHABLE();
                return D3D11_RTV_DIMENSION_UNKNOWN;
            }
        }

        inline TextureDescription ConvertResourceDesc(ID3D11Texture2D* resource)
        {
            D3D11_TEXTURE2D_DESC d3dDesc;
            resource->GetDesc(&d3dDesc);

            TextureDescription description = {};
            description.type = TextureType::Texture2D;
            description.width = d3dDesc.Width;
            description.height = d3dDesc.Height;
            return description;
        }
    }

    D3D11Texture::D3D11Texture(D3D11GraphicsDevice* device, ID3D11Texture2D* resource)
        : Texture(ConvertResourceDesc(resource))
        , _device(device)
        , _handle(resource)
    {

    }

    D3D11Texture::D3D11Texture(D3D11GraphicsDevice* device, const TextureDescription& desc, const void* initialData)
        : Texture(desc)
        , _device(device)
    {
        const DXGI_FORMAT format = ToDXGIFormatWitUsage(desc.format, desc.usage);
    }

    D3D11Texture::~D3D11Texture()
    {
        Destroy();
    }

    void D3D11Texture::Destroy()
    {
        SafeRelease(_handle);
    }

    void D3D11Texture::BackendSetName()
    {
        D3D11SetObjectName(_handle, name);
    }
}
