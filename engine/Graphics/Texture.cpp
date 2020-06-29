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
        : GraphicsResource(device, name, MemoryUsage::GpuOnly)
    {

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

    bool Texture::Define2D(uint32_t width, uint32_t height, PixelFormat format, bool mipmapped, const void* initialData, TextureUsage usage)
    {
        Destroy();

        description.type = TextureType::Texture2D;
        description.format = format;
        description.usage = usage;
        description.width = width;
        description.height = height;
        description.depth = 1u;
        description.mipLevels = mipmapped ? CalculateMipLevels(width, height) : 1u;
        description.sampleCount = 1u;
        description.label = name.c_str();

        const bool hasInitData = initialData != nullptr;
        if (mipmapped && hasInitData)
        {
            description.usage |= TextureUsage::RenderTarget;
        }

        handle = device.CreateTexture(description, 0, initialData, mipmapped);
        return handle.isValid();
    }

    uint32_t Texture::GetWidth(uint32_t mipLevel) const
    {
        return (mipLevel == 0) || (mipLevel < description.mipLevels) ? max(1u, description.width >> mipLevel) : 0;
    }

    uint32_t Texture::GetHeight(uint32_t mipLevel) const
    {
        return (mipLevel == 0) || (mipLevel < description.mipLevels) ? max(1u, description.height >> mipLevel) : 0;
    }

    uint32_t Texture::GetDepth(uint32_t mipLevel) const
    {
        if (description.type == TextureType::Texture3D)
            return 1u;

        return (mipLevel == 0) || (mipLevel < description.mipLevels) ? max(1U, description.depth >> mipLevel) : 0;
    }
}

