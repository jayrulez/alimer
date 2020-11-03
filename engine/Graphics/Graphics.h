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

#include "Core/Object.h"
#include "Graphics/Resources.h"
#include <string>

namespace alimer
{
    class Window;

#ifdef _DEBUG
#    define DEFAULT_ENABLE_DEBUG_LAYER true
#else
#    define DEFAULT_ENABLE_DEBUG_LAYER false
#endif

    struct GraphicsSettings final
    {
        std::string applicationName    = "Alimer";
        PixelFormat colorFormat        = PixelFormat::BGRA8UnormSrgb;
        PixelFormat depthStencilFormat = PixelFormat::Depth32Float;
        bool        enableDebugLayer   = DEFAULT_ENABLE_DEBUG_LAYER;
        bool        verticalSync       = false;
    };

    /// Defines a Graphics module
    class ALIMER_API Graphics : public Object
    {
        ALIMER_OBJECT(Graphics, Object);

    public:
        static Graphics* Create(Window& window, const GraphicsSettings& settings);

    protected:
        /// Constructor
        Graphics(Window& window, const GraphicsSettings& settings);

        Window&       window;
        PixelFormat   colorFormat;
        PixelFormat   depthStencilFormat;
        bool          enableDebugLayer;
        bool          verticalSync;
    };
}
