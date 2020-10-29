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

#include "Core/Ptr.h"
#include "Platform/WindowHandle.h"
#include "Graphics/GraphicsResource.h"

namespace Alimer
{
    class ALIMER_API SwapChain : public GraphicsResource, public RefCounted
    {
    public:
        // Number of swapchain back buffers.
        static constexpr uint32_t kBufferCount = 3;

        void SetVerticalSync(bool value) { verticalSync = value; }
        bool GetVerticalSync() const { return verticalSync; }

        PixelFormat GetBackbufferFormat() const noexcept { return colorFormat; }

        virtual Texture* GetCurrentTexture() const = 0;
        virtual uint32_t Present() = 0;

    protected:
        /// Constructor.
        SwapChain(GraphicsDevice& device, WindowHandle windowHandle);

        uint32_t width = 0;
        uint32_t height = 0;
        PixelFormat colorFormat = PixelFormat::BGRA8Unorm;
        bool verticalSync = true;
        bool isFullscreen = false;
        uint32_t currentBackBufferIndex = 0;
    };
}
