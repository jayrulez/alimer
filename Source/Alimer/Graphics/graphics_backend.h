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

#include "graphics/graphics.h"

namespace alimer
{
    namespace graphics
    {
        struct Renderer
        {
            bool (*Init)(const Configuration& config);
            void (*Shutdown)(void);

            ContextHandle(*CreateContext)(const ContextInfo& info);
            void(*DestroyContext)(ContextHandle handle);
            bool(*ResizeContext)(ContextHandle handle, uint32_t width, uint32_t height);

            bool(*BeginFrame)(ContextHandle handle);
            void(*EndFrame)(ContextHandle handle);
            //void(*BeginRenderPass)(ContextHandle handle, const Color& clearColor, float clearDepth, uint8_t clearStencil);
            void(*EndRenderPass)(ContextHandle handle);

            /* Texture */
            TextureHandle(*CreateTexture)(const TextureInfo& info);
            void(*DestroyTexture)(TextureHandle handle);

            /* RenderPass */
            RenderPassHandle(*CreateRenderPass)(const RenderPassInfo& info);
            void(*DestroyRenderPass)(RenderPassHandle handle);
        };
    }
}
