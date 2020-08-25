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

#include "Core/Object.h"
#include "Math/Rect.h"
//#include "Core/Delegate.h"

namespace Alimer
{
    enum class WindowFlags : uint32_t
    {
        None = 0,
        Resizable = (1 << 0),
        Fullscreen = (1 << 1),
        ExclusiveFullscreen = (1 << 2),
        Hidden = (1 << 3),
        Borderless = (1 << 4),
        Minimized = (1 << 5),
        Maximized = (1 << 6),
        OpenGL = (1 << 7),
    };
    ALIMER_DEFINE_ENUM_FLAG_OPERATORS(WindowFlags, uint32_t);

    using NativeHandle = void*;

    /// Defines an OS window.
    class ALIMER_API Window : public Object
    {
        ALIMER_OBJECT(Window, Object);

    public:
        /// Destructor.
        virtual ~Window() = default;

        /// Gets the bounding rectangle of the window.
        virtual Rect GetBounds() const = 0;

        /// Set window title.
        void SetTitle(const std::string& newTitle);
        /// Return window title.
        const std::string& GetTitle() const;

        bool IsVisible() const;
        bool IsMaximized() const;
        bool IsMinimized() const;
        bool IsFullscreen() const;

        /// The dot-per-inch scale factor
        virtual float GetDpiFactor() const;

        /// The scale factor for systems with heterogeneous window and pixel coordinates
        virtual float GetContentScaleFactor() const;

        /// Get the native window handle
        virtual NativeHandle GetNativeHandle() const = 0;

        //Delegate<void()> SizeChanged;
    private:
        virtual void SetPlatformTitle(const std::string& newTitle) = 0;

    protected:
        /// Constructor.
        Window() = default;

        std::string title;
    };
}
