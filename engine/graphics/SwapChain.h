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

#include "graphics/Texture.h"
#include <vector>

namespace alimer
{
    class GPUDevice;
    class Texture;


    enum class PresentMode : uint32_t
    {
        Immediate,
        Mailbox,
        Fifo
    };

    class ALIMER_API SwapChain final
    {
    public:
        /// Constructor.
        SwapChain(GPUDevice& device, void* windowHandle, const usize& extent);
        ~SwapChain();

        enum class ResizeResult
        {
            Success,
            NoSurface,
            Error
        };

        ResizeResult Resize(uint32_t newWidth, uint32_t newHeight);

        Texture* GetCurrentTexture() const;

        const usize& GetExtent() const;

        /**
        * Get the native API handle.
        */
        SwapChainHandle GetHandle() const { return handle; }

    private:
        void Destroy();
        ResizeResult ApiResize();

        GPUDevice& device;
        SwapChainHandle handle{};
        usize extent{};
        void* windowHandle;

        PixelFormat colorFormat = PixelFormat::BGRA8Unorm;
        PixelFormat depthStencilFormat = PixelFormat::Unknown;
        TextureSampleCount sampleCount = TextureSampleCount::Count1;
        PresentMode presentMode = PresentMode::Fifo;
        uint32_t imageCount = 2u;

        std::vector<RefPtr<Texture>> textures;
        mutable uint32_t textureIndex{ 0 };
    };
}
