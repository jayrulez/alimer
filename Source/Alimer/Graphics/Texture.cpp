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

#include "Core/Log.h"
#include "Graphics/Texture.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/GraphicsImpl.h"

namespace alimer
{
    Texture::Texture()
        : GraphicsResource(ResourceDimension::Texture2D)
    {

    }

    Texture::Texture(const TextureDescription& desc)
        : GraphicsResource(ResourceDimension::Texture2D)
        , dimension(desc.dimension)
        , format(desc.format)
        , usage(desc.usage)
        , width(desc.width)
        , height(desc.height)
        , depth(desc.depth)
        , arrayLayers(desc.arrayLayers)
        , mipLevels(desc.mipLevels)
        , sampleCount(desc.sampleCount)
    {
        //handle = graphics->GetImpl()->CreateTexture(&desc, nullptr);
        //ALIMER_ASSERT(handle.isValid());
    }

    Texture::~Texture()
    {
        Destroy();
    }


    void Texture::RegisterObject()
    {
        RegisterFactory<Texture>();
    }

    void Texture::Destroy()
    {
        if (handle.isValid())
        {
            //graphics->GetImpl()->Destroy(handle);
            handle.id = kInvalidHandleId;
        }
    }

    bool Texture::Define2D(uint32_t width_, uint32_t height_, bool mipMap, PixelFormat format_, TextureUsage usage_)
    {
        ALIMER_VERIFY_MSG(width_ > 0, "Width must be greather than zero");
        ALIMER_VERIFY_MSG(height_ > 0, "Height must be greather than zero");
        ALIMER_VERIFY_MSG(format_ != PixelFormat::Invalid, "Formate must be valid");

        Destroy();

        dimension = TextureDimension::Texture2D;
        format = format_;
        usage = usage_;
        width = width_;
        height = height_;
        depth = 1u;
        arrayLayers = 1u;
        mipLevels = mipMap ? CalculateMipLevels(width, height, 1u) : 1u;
        sampleCount = 1u;
        return true;
    }

    RefPtr<Texture> Texture::CreateExternalTexture(void* externalHandle, uint32_t width, uint32_t height, PixelFormat format, bool mipMap)
    {
        TextureDescription description;
        description.dimension = TextureDimension::Texture2D;
        description.format = format;
        description.usage = TextureUsage::Sampled;
        description.width = width;
        description.height = height;
        description.depth = 1u;
        description.arrayLayers = 1u;
        description.mipLevels = mipMap ? CalculateMipLevels(width, height, 1u) : 1u;
        description.sampleCount = 1u;
        description.externalHandle = externalHandle;

        return RefPtr<Texture>(new Texture(description));
    }

    uint32_t Texture::GetWidth(uint32_t mipLevel) const
    {
        return (mipLevel == 0) || (mipLevel < mipLevels) ? Max(1u, width >> mipLevel) : 0;
    }

    uint32_t Texture::GetHeight(uint32_t mipLevel) const
    {
        return (mipLevel == 0) || (mipLevel < mipLevels) ? Max(1u, height >> mipLevel) : 0;
    }

    uint32_t Texture::GetDepth(uint32_t mipLevel) const
    {
        if (dimension == TextureDimension::Texture3D)
            return 1u;

        return (mipLevel == 0) || (mipLevel < mipLevels) ? Max(1U, depth >> mipLevel) : 0;
    }

    uint32_t Texture::CalculateMipLevels(uint32_t width, uint32_t height, uint32_t depth)
    {
        uint32_t mipLevels = 0;
        uint32_t size = Max(Max(width, height), depth);
        while (1u << mipLevels <= size) {
            ++mipLevels;
        }

        if (1u << mipLevels < size) {
            ++mipLevels;
        }

        return mipLevels;
    }
}

