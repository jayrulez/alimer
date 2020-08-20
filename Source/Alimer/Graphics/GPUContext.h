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

#include "Graphics/GPUBuffer.h"
#include "Graphics/GPUTexture.h"
#include "Math/Color.h"
#include "Math/Viewport.h"

namespace alimer
{
    /// A container that stores commands for the GPU to execute.
    class ALIMER_API GPUContext
    {
    public:
        /// Constructor.
        GPUContext();
        virtual ~GPUContext() = default;

        virtual void Flush() = 0;

        virtual void PushDebugGroup(const String& name) = 0;
        virtual void PopDebugGroup() = 0;
        virtual void InsertDebugMarker(const String& name) = 0;

        virtual void BeginRenderPass(uint32_t numColorAttachments, const RenderPassColorAttachment* colorAttachments, const RenderPassDepthStencilAttachment* depthStencil) = 0;
        virtual void EndRenderPass() = 0;

        virtual void SetScissorRect(const URect& scissorRect) = 0;
        virtual void SetScissorRects(const URect* scissorRects, uint32_t count) = 0;
        virtual void SetViewport(const Viewport& viewport) = 0;
        virtual void SetViewports(const Viewport* viewports, uint32_t count) = 0;
        virtual void SetBlendColor(const Color& color) = 0;

        virtual void BindBuffer(uint32_t slot, GPUBuffer* buffer) = 0;
        virtual void BindBufferData(uint32_t slot, const void* data, uint32_t size) = 0;
    };
}