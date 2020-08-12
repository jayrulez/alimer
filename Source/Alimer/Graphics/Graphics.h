//
// Copyright (c) 2020 Amer Koleci and contributors.
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

#include "Core/Window.h"
#include "Graphics/CommandContext.h"
#include "Graphics/Buffer.h"
#include <EASTL/vector.h>

namespace alimer
{
    class GraphicsImpl;

    /// Defines the graphics subsystem.
    class ALIMER_API Graphics final : public Object
    {
        friend class GraphicsResource;

        ALIMER_OBJECT(Graphics, Object);

    public:
        /// Destructor.
        ~Graphics() override;

        static Graphics* Create(RendererType preferredRendererType = RendererType::Count);

        /// Set graphics mode. Create the window and rendering context if not created yet. Return true on success.
        bool SetMode(const UInt2& size, WindowFlags windowFlags = WindowFlags::None, uint32_t sampleCount = 1);

        /// Set vertical sync on/off.
        void SetVerticalSync(bool value);

        bool BeginFrame();
        void EndFrame();

        /// Return the rendering window.
        ALIMER_FORCE_INLINE Window* GetRenderWindow() const { return window.get(); }

        /// Get the device capabilities.
        const GraphicsCapabilities& GetCaps() const;

        /// Return whether is using vertical sync.
        bool GetVerticalSync() const;

        /// Total number of CPU frames completed (completed means all command buffers submitted to the GPU)
        ALIMER_FORCE_INLINE uint64_t GetFrameCount() const { return frameCount; }

    private:
        Graphics(RendererType rendererType);

        GraphicsImpl* impl;
        RefPtr<Window> window;
        UInt2 resolution = UInt2::Zero;
        PixelFormat colorFormat = PixelFormat::BGRA8UnormSrgb;
        PixelFormat depthStencilFormat = PixelFormat::Depth32Float;
        uint32_t sampleCount = 1;
        uint64_t frameCount = 0;

    private:
        ALIMER_DISABLE_COPY_MOVE(Graphics);
    };

    ALIMER_API extern RefPtr<Graphics> graphics;
}

