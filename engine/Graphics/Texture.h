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
#include "Graphics/BackendTypes.h"

namespace Alimer
{
    class ALIMER_API Texture final : public Object, public GraphicsResource
    {
        ALIMER_OBJECT(Texture, Object);

    public:
        Texture(GraphicsDevice& device, TextureHandle handle, const Extent3D& extent, PixelFormat format, TextureLayout layout, TextureUsage usage = TextureUsage::Sampled, TextureSampleCount sampleCount = TextureSampleCount::Count1);
        ~Texture() override;
        void Destroy() override;

        /// Get the texture pixel format.
        PixelFormat GetFormat() const { return desc.format; }

        /// Get a mip-level width.
        uint32 GetWidth(uint32 mipLevel = 0) const { return (mipLevel == 0) || (mipLevel < desc.mipLevels) ? Max(1u, desc.width >> mipLevel) : 0; }

        /// Get a mip-level height.
        uint32 GetHeight(uint32 mipLevel = 0) const { return (mipLevel == 0) || (mipLevel < desc.mipLevels) ? Max(1u, desc.height >> mipLevel) : 0; }

        /// Get a mip-level depth.
        uint32 GetDepth(uint32 mipLevel = 0) const { return (mipLevel == 0) || (mipLevel < desc.mipLevels) ? Max(1u, desc.depthOrArraySize >> mipLevel) : 0; }

        /// Gets number of mipmap levels of the texture.
        uint32 GetMipLevels() const { return desc.mipLevels; }

        /// Get the array layers of the texture.
        uint32 GetArrayLayers() const { return desc.depthOrArraySize; }

        /// Get the texture usage.
        TextureUsage GetUsage() const { return desc.usage; }

        /// Get the current texture layout.
        TextureLayout GetLayout() const { return layout; }

        /// Get the array index of a subresource.
        uint32 GetSubresourceArraySlice(uint32_t subresource) const { return subresource / desc.mipLevels; }

        /// Get the mip-level of a subresource.
        uint32 GetSubresourceMipLevel(uint32_t subresource) const { return subresource % desc.mipLevels; }

        /// Get the subresource index.
        uint32 GetSubresourceIndex(uint32 mipLevel, uint32 arraySlice) const { return mipLevel + arraySlice * desc.mipLevels; }

        /// Calculates the resulting size at a single level for an original size.
        static uint32 CalculateMipSize(uint32 mipLevel, uint32 baseSize)
        {
            baseSize = baseSize >> mipLevel;
            return baseSize > 0u ? baseSize : 1u;
        }

        TextureHandle GetHandle() const { return handle; }

        void SetName(const std::string& newName) override;

    protected:
        /// Constructor
        Texture(GraphicsDevice& device, const TextureDescription& desc);

        TextureDescription desc;
        TextureLayout layout;

        TextureHandle handle{};
        AllocationHandle allocation{};
        TextureApiFormat apiFormat;
    };
}
