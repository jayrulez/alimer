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

#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"
#include "Math/Color.h"

namespace alimer::graphics
{
    /// A container that stores commands for the GPU to execute.
    class ALIMER_API CommandContext
    {
    public:
        /// Constructor.
        CommandContext();
        virtual ~CommandContext() = default;

        virtual void Commit(bool waitForCompletion = false) = 0;

        virtual void PushDebugGroup(const String& name) = 0;
        virtual void PopDebugGroup() = 0;
        virtual void InsertDebugMarker(const String& name) = 0;

        virtual void BeginRenderPass(const RenderPassDescription& renderPass) = 0;
        virtual void EndRenderPass() = 0;

        virtual void SetScissorRect(uint32 x, uint32 y, uint32 width, uint32 height) = 0;
        virtual void SetScissorRects(const Rect* scissorRects, uint32_t count) = 0;
        virtual void SetViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f) = 0;
        virtual void SetBlendColor(const Color& color) = 0;

        virtual void BindBuffer(uint32_t slot, Buffer* buffer) = 0;
        virtual void BindBufferData(uint32_t slot, const void* data, uint32_t size) = 0;
    };
}
