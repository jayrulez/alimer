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
    class ALIMER_API Texture final : public GraphicsResource
    {
    public:
        Texture(GraphicsDevice& device);
        Texture(const Texture&) = delete;
        Texture(Texture&& other) noexcept;
        Texture& operator=(const Texture&) = delete;
        Texture& operator=(Texture&&) = delete;
        ~Texture();

        void Destroy() override;

        bool Define2D(GraphicsDevice& device, uint32_t width, uint32_t height, PixelFormat format, uint32_t mipLevels = kMaxPossibleMipLevels, uint32_t arrayLayers = 1, TextureUsage usage = TextureUsage::Sampled, const void* pInitData = nullptr);

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

    private:
        TextureDesc textureDesc{};
        TextureHandle handle{ kInvalidHandle };
    };
}
