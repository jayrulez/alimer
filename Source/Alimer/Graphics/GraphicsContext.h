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
#include "Math/Color.h"

namespace alimer
{
    class CommandContext;

    class GraphicsContext : public RefCounted
    {
    protected:
        /// Constructor.
        GraphicsContext(GraphicsDevice& device, const GraphicsContextDescription& desc);

    public:
        virtual ~GraphicsContext() = default;

        void Resize(uint32_t newWidth, uint32_t newHeight);

        /// Begin command recording.
        virtual void Begin(const char* name, bool profile) = 0;

        /// End command recording.
        virtual void End() = 0;

        /// End active frame and present on screen (if required).
        virtual void Flush(bool wait = false) = 0;

        /// Get the current SwapChain or offscreen texture.
        virtual Texture* GetCurrentColorTexture() const = 0;

        virtual void BeginRenderPass(const RenderPassDescriptor* descriptor) = 0;
        virtual void EndRenderPass() = 0;
        virtual void SetBlendColor(const Color& color) = 0;

    protected:
        /// Release the GPU resources.
        virtual void Destroy() = 0;

        GraphicsDevice& device;

        uint32_t width;
        uint32_t height;
        PixelFormat colorFormat = PixelFormat::BGRA8UNormSrgb;
        PixelFormat depthStencilFormat = PixelFormat::Depth32Float;
    };
}
