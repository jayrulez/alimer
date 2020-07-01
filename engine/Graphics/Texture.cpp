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
    Texture::Texture(GraphicsDevice& device, const TextureDescription& desc)
        : GraphicsResource(device, desc.name, MemoryUsage::GpuOnly)
        , description(desc)
    {

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

    uint32_t Texture::CalculateMipLevels(uint32_t width, uint32_t height, uint32_t depth)
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

