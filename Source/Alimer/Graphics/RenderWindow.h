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

#include "Core/Window.h"
#include "Math/Size.h"
#include "Graphics/PixelFormat.h"

namespace alimer
{
    struct RenderWindowDescription
    {
        std::string title = "Alimer";
        SizeI size = { 1280, 720 };
        WindowFlags windowFlags = WindowFlags::None;

        /// Whether to try use sRGB backbuffer color format.
        bool colorFormatSrgb = false;

        /// The depth format.
        PixelFormat depthStencilFormat = PixelFormat::Depth32Float;

        /// Should the window wait for vertical sync before swapping buffers.
        bool verticalSync = false;
        bool fullscreen = false;

        uint32 sampleCount = 1u;
    };

    class ALIMER_API RenderWindow : public Window
    {
        ALIMER_OBJECT(RenderWindow, Window);

    public:
        RenderWindow(const RenderWindowDescription& desc);
        
    protected:
        PixelFormat colorFormat;
        PixelFormat depthStencilFormat;
    };
}
