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

#include "graphics/Swapchain.h"
#include "graphics/GraphicsDevice.h"
#include "graphics/GraphicsImpl.h"

namespace alimer
{
    Swapchain::Swapchain(GraphicsDevice& device, GraphicsSurface* surface)
        : device{ device }
        , extent(surface->GetSize())
    {
        handle = device.GetImpl()->CreateSwapChain(surface->GetHandle(), extent.width, extent.height, presentMode);
        textures.resize(device.GetImpl()->GetImageCount(handle));
        for (uint32_t i = 0; i < uint32_t(textures.size()); i++)
        {
            GPUTexture textureHandle = device.GetImpl()->GetTexture(handle, i);
            textures[i] = new Texture(device, textureHandle, { extent.width, extent.height, 1u });
        }
    }

    Swapchain::~Swapchain()
    {
        textures.clear();
        textureIndex = 0;

        if (handle.isValid())
        {
            device.GetImpl()->DestroySwapChain(handle);
            handle.id = kInvalidHandle;
        }
    }

    Swapchain::ResizeResult Swapchain::Resize(uint32_t newWidth, uint32_t newHeight)
    {
        return ResizeResult::Success;
    }

    Texture* Swapchain::GetCurrentTexture() const
    {
        textureIndex = device.GetImpl()->GetNextTexture(handle);
        return textures[textureIndex].Get();
    }

    const usize& Swapchain::GetExtent() const
    {
        return extent;
    }
}

