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

namespace alimer
{
    class ALIMER_API Swapchain : public GraphicsResource
    {
    public:
        /// Constructor.
        Swapchain(const SwapchainDescription& description);

        void Resize(uint32_t newWidth, uint32_t newHeight);

        /// Get the current backbuffer texture.
        Texture* GetBackbufferTexture() const;

        /// Get the depth stencil texture.
        Texture* GetDepthStencilTexture() const;

        const RenderPassDescription& GetCurrentRenderPassDescription() const { return currentRenderPassDescription; }

    private:
        virtual void ResizeImpl() = 0;

    protected:
        uint32_t width;
        uint32_t height;
        PixelFormat colorFormat;

        uint32_t backbufferIndex = 0;
        std::vector<SharedPtr<Texture>> backbufferTextures;
        SharedPtr<Texture> depthStencilTexture;
        RenderPassDescription currentRenderPassDescription;
    };
}
