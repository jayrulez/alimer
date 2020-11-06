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

#include "Core/Object.h"
#include "Graphics/GraphicsResource.h"

namespace alimer
{

    struct TextureDescription
    {
        uint32_t width = 1u;
        uint32_t height = 1u;
        uint32_t depth = 1u;
        uint32_t mipLevels = 1u;
        uint32_t arrayLayers = 1u;
        PixelFormat format = PixelFormat::RGBA8Unorm;
        TextureUsage usage = TextureUsage::Sampled;
        TextureType type = TextureType::Type2D;
        TextureSampleCount sampleCount = TextureSampleCount::Count1;
        USAGE Usage = USAGE_DEFAULT;
        uint32_t CPUAccessFlags = 0;
        uint32_t MiscFlags = 0;
        ClearValue clear = {};
        IMAGE_LAYOUT layout = IMAGE_LAYOUT_GENERAL;

        TextureDescription() = default;

        static inline TextureDescription Texure2D(PixelFormat format, uint32_t width, uint32_t height,
                                                  uint32_t mipLevels, uint32_t arrayLayers = 1,
                                                  TextureUsage usage = TextureUsage::Sampled,
                                                  TextureSampleCount sampleCount = TextureSampleCount::Count1,
                                                  IMAGE_LAYOUT layout = IMAGE_LAYOUT_GENERAL) noexcept
        {
            TextureDescription description;
            description.width = width;
            description.height = height;
            description.depth = 1;
            description.mipLevels = mipLevels;
            description.arrayLayers = arrayLayers;
            description.format = format;
            description.usage = usage;
            description.type = TextureType::Type2D;
            description.sampleCount = sampleCount;
            description.layout = layout;
            return description;
        }
    };

    class ALIMER_API Texture : public GraphicsResource, public Object
    {
        ALIMER_OBJECT(Texture, Object);

    public:
        Texture(const TextureDescription& description);

        /// Get the number of mip-levels.
        uint32_t GetMipLevels() const
        {
            return description.mipLevels;
        }

        const TextureDescription& GetDescription() const
        {
            return description;
        }

    protected:
        TextureDescription description;
    };
}
