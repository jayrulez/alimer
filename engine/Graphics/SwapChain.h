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

#include "Graphics/Texture.h"
#include "Math/Size.h"
#include <vector>

namespace Alimer
{
    class Window;

    class ALIMER_API SwapChain : public GraphicsResource
    {
    public:

        Window* GetWindow() const { return window; }
        void SetWindow(Window* newWindow) { window = newWindow; }

        bool GetAutoResizeDrawable() const noexcept { return autoResizeDrawable; }
        void SetAutoResizeDrawable(bool value) noexcept { autoResizeDrawable = value; }

        SizeI GetDrawableSize() const noexcept { return drawableSize; }
        void SetDrawableSize(const SizeI& value) noexcept { drawableSize = value; }

        PixelFormat GetColorFormat() const noexcept { return colorFormat; }
        void SetColorFormat(PixelFormat value) noexcept { colorFormat = value; }

        PixelFormat GetDepthStencilFormat() const noexcept { return depthStencilFormat; }
        void SetDepthStencilFormat(PixelFormat value) noexcept { depthStencilFormat = value; }

        uint32_t GetSampleCount() const noexcept { return sampleCount; }
        void SetSampleCount(uint32_t value) noexcept { sampleCount = value; }

        virtual bool CreateOrResize() = 0;

        virtual Texture* GetCurrentTexture() const = 0;

    protected:
        /// Constructor.
        SwapChain()
            : GraphicsResource(Type::SwapChain)
        {

        }

        Window* window = nullptr;
        bool autoResizeDrawable = true;
        SizeI drawableSize;
        PixelFormat colorFormat = PixelFormat::BGRA8Unorm;
        PixelFormat depthStencilFormat = PixelFormat::Depth32Float;
        uint32_t sampleCount = 1u;
        bool verticalSync = true;
    };
}
