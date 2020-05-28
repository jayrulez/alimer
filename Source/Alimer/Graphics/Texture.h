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

#pragma once

#include "graphics/GraphicsResource.h"
#include "math/size.h"

namespace alimer
{
    class ALIMER_API Texture : public GraphicsResource
    {
    protected:
        Texture(GraphicsDevice& device, const TextureDescriptor* descriptor);

    public:
        virtual ~Texture() = default;

        static RefPtr<Texture> Create2D(GraphicsDevice& device, uint32_t width, uint32_t height, PixelFormat format);

        /**
        * Get a mip-level width.
        */
        uint32_t GetWidth(uint32_t mipLevel = 0) const { return (mipLevel == 0) || (mipLevel < mipLevelCount) ? max(1u, width >> mipLevel) : 0; }

        /**
        * Get a mip-level height.
        */
        uint32_t GetHeight(uint32_t mipLevel = 0) const { return (mipLevel == 0) || (mipLevel < mipLevelCount) ? max(1u, height >> mipLevel) : 0; }

        /**
        * Get a mip-level depth.
        */
        uint32_t GetDepth(uint32_t mipLevel = 0) const { return (mipLevel == 0) || (mipLevel < mipLevelCount) ? max(1U, depth >> mipLevel) : 0; }

    protected:
        TextureType type = TextureType::Type2D;
        TextureUsage usage = TextureUsage::Sampled;
        u32 width = 1u;
        u32 height = 1u;
        u32 depth = 1u;
        PixelFormat format = PixelFormat::RGBA8UNorm;
        u32 mipLevelCount = 1u;
        TextureSampleCount sampleCount = TextureSampleCount::Count1;
    };
}
