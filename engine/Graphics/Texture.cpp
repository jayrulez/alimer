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

#include "graphics/Texture.h"
#include "graphics/GraphicsDevice.h"

namespace alimer
{
    Texture::Texture(GraphicsDevice& device, const TextureDescription& desc)
        : GraphicsResource(device, GraphicsResourceUsage::Default)
        , type(desc.type)
        , format(desc.format)
        , usage(desc.usage)
        , width(desc.width)
        , height(desc.height)
        , depth(desc.depth)
        , mipLevelCount(desc.mipLevelCount)
        , sampleCount(desc.sampleCount)
    {

    }

    RefPtr<Texture> Texture::Create2D(GraphicsDevice& device, uint32_t width, uint32_t height, PixelFormat format)
    {
        TextureDescription desc = {};
        desc.type = TextureType::Type2D;
        desc.format = format;
        desc.usage = TextureUsage::Sampled;
        desc.width = width;
        desc.height = height;
        desc.depth = 1;
        desc.mipLevelCount = 1;
        desc.sampleCount = TextureSampleCount::Count1;
        return nullptr;
        //return device.CreateTexture(desc, nullptr);
    }
}
