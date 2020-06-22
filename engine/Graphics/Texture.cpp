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

#include "graphics/Texture.h"
#include "graphics/GraphicsDevice.h"

namespace alimer
{
    Texture::Texture(GraphicsDevice& device)
        : GraphicsResource(device, GraphicsResourceUsage::Default)
    {

    }

    Texture::Texture(Texture&& other) noexcept
        : GraphicsResource(other.device, other.resourceUsage)
        , textureDesc{ other.textureDesc }
    {
        handle = other.handle;
        other.handle = kInvalidTextureHandle;
    }

    Texture::~Texture()
    {
        Destroy();
    }

    void Texture::Destroy()
    {
        if (handle.isValid()) {
            device.DestroyTexture(handle);
        }
    }

    static inline uint32_t CalculateMipLevels(uint32_t width, uint32_t height = 0, uint32_t depth = 0)
    {
        uint32_t levels = 1;
        for (uint32_t size = max(max(width, height), depth); size > 1; levels += 1)
        {
            size /= 2;
        }

        return levels;
    }


    bool Texture::Define2D(GraphicsDevice& device, uint32_t width, uint32_t height, PixelFormat format, uint32_t mipLevels, uint32_t arrayLayers, TextureUsage usage, const void* pInitData)
    {
        const bool autoGenerateMipmaps = mipLevels == kMaxPossibleMipLevels;
        const bool hasInitData = pInitData != nullptr;
        if (autoGenerateMipmaps && hasInitData)
        {
            usage |= TextureUsage::RenderTarget;
        }

        textureDesc.type = TextureType::Type2D;
        textureDesc.format = format;
        textureDesc.usage = usage;
        textureDesc.width = width;
        textureDesc.height = height;
        textureDesc.depth = autoGenerateMipmaps ? CalculateMipLevels(width, height) : mipLevels;
        textureDesc.mipLevels = 1;
        textureDesc.arrayLayers = arrayLayers;
        textureDesc.sampleCount = TextureSampleCount::Count1;

        handle = device.CreateTexture(textureDesc, pInitData, autoGenerateMipmaps);
        return handle.isValid();
    }

    uint32_t Texture::GetWidth(uint32_t mipLevel) const
    {
        return (mipLevel == 0) || (mipLevel < textureDesc.mipLevels) ? max(1u, textureDesc.width >> mipLevel) : 0;
    }

    uint32_t Texture::GetHeight(uint32_t mipLevel) const
    {
        return (mipLevel == 0) || (mipLevel < textureDesc.mipLevels) ? max(1u, textureDesc.height >> mipLevel) : 0;
    }

    uint32_t Texture::GetDepth(uint32_t mipLevel) const
    {
        return (mipLevel == 0) || (mipLevel < textureDesc.mipLevels) ? max(1U, textureDesc.depth >> mipLevel) : 0;
    }
}

