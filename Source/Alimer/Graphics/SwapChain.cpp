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

#include "Graphics/SwapChain.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/Texture.h"
#include "Core/Log.h"

namespace alimer
{
    SwapChain::SwapChain(void* windowHandle, uint32_t width, uint32_t height, bool isFullscreen, bool enableVSync, PixelFormat preferredColorFormat, PixelFormat preferredDepthStencilFormat)
        : GraphicsResource(ResourceDimension::Texture2D)
    {
        handle = GetGraphics()->CreateSwapChain(windowHandle, width, height, isFullscreen, enableVSync, preferredColorFormat);
        colorTextures.resize(GetGraphics()->GetImageCount(handle));
        for (eastl_size_t i = 0; i < colorTextures.size(); i++)
        {
            //colorTextures[i] = new Texture();
        }
    }

    SwapChain::~SwapChain()
    {
        Destroy();
    }

    void SwapChain::Destroy()
    {
        colorTextures.clear();
        depthStencilTexture.Reset();

        if (handle.isValid())
        {
            GetGraphics()->Destroy(handle);
            handle.id = kInvalidHandleId;
        }
    }
}

