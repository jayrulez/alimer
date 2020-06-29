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

#include "Graphics/Texture.h"
#include "Graphics/GraphicsDevice.h"
#include "Core/Log.h"

namespace alimer
{
    namespace
    {
        /*inline TextureType FindTextureType(Extent3D extent)
        {
            TextureType result{};

            uint32_t count = 0;

            if (extent.width >= 1)
            {
                count++;
            }

            if (extent.height >= 1)
            {
                count++;
            }

            if (extent.depth > 1)
            {
                count++;
            }

            switch (count)
            {
            case 1:
                result = TextureType::Type1D;
                break;
            case 2:
                result = TextureType::Type2D;
                break;
            case 3:
                result = TextureType::Type3D;
                break;
            default:
                LOG_ERROR("Cannot detect valid texture type.");
                break;
            }

            return result;
        }*/

        static inline uint32_t CalculateMipLevels(uint32_t width, uint32_t height = 0, uint32_t depth = 1)
        {
            uint32_t mipLevels = 0;
            uint32_t size = max(max(width, height), depth);
            while (1u << mipLevels <= size) {
                ++mipLevels;
            }

            if (1u << mipLevels < size) {
                ++mipLevels;
            }

            return mipLevels;
        }
    }

    Texture::Texture(GraphicsDevice& device, const std::string& name)
        : GraphicsResource(device, Type::Unknown, name, MemoryUsage::GpuOnly)
    {

    }

    Texture::Texture(GraphicsDevice& device, const Extent3D& size, PixelFormat format, TextureUsage usage, uint32_t mipLevels, uint32_t sampleCount, const void* initialData)
        : GraphicsResource(device, Type::Unknown, "", MemoryUsage::GpuOnly)
        //, type(FindTextureType(size))
        , size{ size }
        , format{ format }
        , usage{ usage }
        , sampleCount{ sampleCount }
    {
        const uint32_t maxMipLevels = CalculateMipLevels(size.width, size.height);
        if (mipLevels == 0)
            mipLevels = maxMipLevels;

        ALIMER_ASSERT(mipLevels <= maxMipLevels);

        const bool autoGenerateMipmaps = false; // mipLevels == kMaxPossibleMipLevels;
        /*const bool hasInitData = initialData != nullptr;
        if (autoGenerateMipmaps && hasInitData)
        {
            usage |= TextureUsage::RenderTarget | TextureUsage::GenerateMipmaps;
        }*/

        TextureDescription desc{};
        //desc.type = TextureType::Type2D;
        desc.format = format;
        desc.usage = usage;
        desc.size = size;
        desc.mipLevels = mipLevels;
        desc.sampleCount = sampleCount;
        handle = device.CreateTexture(desc, 0, initialData, autoGenerateMipmaps);
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

    uint32_t Texture::GetWidth(uint32_t mipLevel) const
    {
        return (mipLevel == 0) || (mipLevel < mipLevels) ? max(1u, size.width >> mipLevel) : 0;
    }

    uint32_t Texture::GetHeight(uint32_t mipLevel) const
    {
        return (mipLevel == 0) || (mipLevel < mipLevels) ? max(1u, size.height >> mipLevel) : 0;
    }

    uint32_t Texture::GetDepth(uint32_t mipLevel) const
    {
        if (type == Type::Texture3D)
            return 1u;

        return (mipLevel == 0) || (mipLevel < mipLevels) ? max(1U, size.depth >> mipLevel) : 0;
    }
}

