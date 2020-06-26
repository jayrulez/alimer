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
#include "Math/Size.h"

namespace alimer
{
    class ALIMER_API Texture : public GraphicsResource
    {
    public:
        Texture(GraphicsDevice& device);

        bool DefineExternal(uint64_t externalHandle, const TextureDescription& desc);
        bool Define2D(uint32_t width, uint32_t height, PixelFormat format, uint32_t mipLevels = kMaxPossibleMipLevels, uint32_t arraySize = 1, TextureUsage usage = TextureUsage::Sampled, const void* pInitData = nullptr);
        bool DefineCube(uint32_t size, PixelFormat format, uint32_t mipLevels = kMaxPossibleMipLevels, uint32_t arraySize = 1, TextureUsage usage = TextureUsage::Sampled, const void* pInitData = nullptr);

        /**
        * Get the texture type.
        */
        TextureType GetTextureType() const { return desc.type; }

        /**
        * Get the texture pixel format.
        */
        PixelFormat GetPixelFormat() const { return desc.format; }

        /**
        * Get the texture usage.
        */
        TextureUsage GetUsage() const { return desc.usage; }

        /**
        * Get a mip-level width.
        */
        uint32_t GetWidth(uint32_t mipLevel = 0) const;

        /**
        * Get a mip-level height.
        */
        uint32_t GetHeight(uint32_t mipLevel = 0) const;

        /**
        * Get a mip-level depth.
        */
        uint32_t GetDepth(uint32_t mipLevel = 0) const;

        /**
        * Gets number of mipmap levels of the texture.
        */
        uint32_t GetMipLevels() const { return desc.mipLevels; }

        /**
        * Get the array size of the texture.
        */
        uint32_t GetArraySize() const { return desc.arraySize; }

        /**
        * Gets the texture description.
        */
        const TextureDescription& GetDescription() const { return desc; }

        /**
        * Get the array index of a subresource.
        */
        uint32_t GetSubresourceArraySlice(uint32_t subresource) const { return subresource / desc.mipLevels; }

        /**
        * Get the mip-level of a subresource.
        */
        uint32_t GetSubresourceMipLevel(uint32_t subresource) const { return subresource % desc.mipLevels; }

        /**
        * Get the subresource index.
        */
        uint32_t GetSubresourceIndex(uint32_t mipLevel, uint32_t arraySlice) const { return mipLevel + arraySlice * desc.mipLevels; }

        /**
        * Calculates the resulting size at a single level for an original size.
        */
        static uint32_t CalculateMipSize(uint32_t mipLevel, uint32_t baseSize)
        {
            baseSize = baseSize >> mipLevel;
            return baseSize > 0u ? baseSize : 1u;
        }

    private:
        TextureDescription desc{};
    };
}
