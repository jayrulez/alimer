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

namespace Alimer
{
    Texture::Texture(const TextureDescription& desc)
        : GraphicsResource(Type::Texture)
        , type(desc.type)
        , format(desc.format)
        , usage(desc.usage)
        , width(desc.width)
        , height(desc.height)
        , depth(desc.depth)
        , arrayLayers(desc.arrayLayers)
        , mipLevels(desc.mipLevels)
        , sampleCount(desc.sampleCount)
    {

    }

    uint32 Texture::GetWidth(uint32 mipLevel) const
    {
        return (mipLevel == 0) || (mipLevel < mipLevels) ? Max(1u, width >> mipLevel) : 0;
    }

    uint32 Texture::GetHeight(uint32 mipLevel) const
    {
        return (mipLevel == 0) || (mipLevel < mipLevels) ? Max(1u, height >> mipLevel) : 0;
    }

    uint32 Texture::GetDepth(uint32 mipLevel) const
    {
        if (type == TextureType::Type3D)
            return 1u;

        return (mipLevel == 0) || (mipLevel < mipLevels) ? Max(1U, depth >> mipLevel) : 0;
    }

    uint32 Texture::CalculateMipLevels(uint32 width, uint32 height, uint32 depth)
    {
        uint32 mipLevels = 0;
        uint32 size = Max(Max(width, height), depth);
        while (1u << mipLevels <= size) {
            ++mipLevels;
        }

        if (1u << mipLevels < size) {
            ++mipLevels;
        }

        return mipLevels;
    }
}

