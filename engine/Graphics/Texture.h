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

#include "Graphics/GraphicsResource.h"

namespace Alimer
{
    class ALIMER_API Texture : public GraphicsResource
    {
        ALIMER_OBJECT(Texture, GraphicsResource);

    public:
        /// Register object factory.
        static void RegisterObject();

        /// Construct.
        Texture();
        /// Destruct.
        ~Texture();

        /// Constructor.
        Texture(const TextureDescription& desc);

        void Destroy() override;

        /// Get the texture pixel format.
        PixelFormat GetFormat() const { return format; }

        /// Get a mip-level width.
        uint32 GetWidth(uint32 mipLevel = 0) const;

        /// Get a mip-level height.
        uint32 GetHeight(uint32 mipLevel = 0) const;

        /// Get a mip-level depth.
        uint32 GetDepth(uint32 mipLevel = 0) const;

        /// Gets number of mipmap levels of the texture.
        uint32 GetMipLevels() const { return mipLevels; }

        /// Get the array layers of the texture.
        uint32 GetArrayLayers() const { return arrayLayers; }

        /// Get the texture usage.
        TextureUsage GetUsage() const { return usage; }

        /// Get the array index of a subresource.
        uint32 GetSubresourceArraySlice(uint32_t subresource) const { return subresource / mipLevels; }

        /// Get the mip-level of a subresource.
        uint32 GetSubresourceMipLevel(uint32_t subresource) const { return subresource % mipLevels; }

        /// Get the subresource index.
        uint32 GetSubresourceIndex(uint32 mipLevel, uint32 arraySlice) const { return mipLevel + arraySlice * mipLevels; }

        /// Calculates the resulting size at a single level for an original size.
        static uint32 CalculateMipSize(uint32 mipLevel, uint32 baseSize)
        {
            baseSize = baseSize >> mipLevel;
            return baseSize > 0u ? baseSize : 1u;
        }

        static uint32_t CalculateMipLevels(uint32 width, uint32 height, uint32 depth = 1u);

    protected:
        TextureHandle handle{};
        TextureType type = TextureType::Type2D;
        PixelFormat format = PixelFormat::RGBA8Unorm;
        TextureUsage usage = TextureUsage::Sampled;
        uint32 width = 1u;
        uint32 height = 1u;
        uint32 depth = 1u;
        uint32 mipLevels = 1u;
        uint32 arrayLayers = 1u;
        uint32 sampleCount = 1u;
    };
}
