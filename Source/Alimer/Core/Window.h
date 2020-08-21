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

#include "config.h"
#include "Core/Object.h"
#include "Math/Size.h"
#include "Core/Delegate.h"

#if defined(ALIMER_GLFW)
struct GLFWwindow;
#endif

namespace alimer
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
        constexpr static const int32 Centered = Limits<int32_t>::Max;

        Window(const std::string& title, int32 x, int32 y, int32 width, int32 height, WindowFlags flags = WindowFlags::None);

        virtual ~Window() override;
        virtual void Destroy();
        void Close();
        void BeginFrame();

        /// Set window title.
        void SetTitle(const String& newTitle);

        /// Return window title.
        const std::string& GetTitle() const;
        /// Return window client size.
        SizeI GetSize() const;

        bool ShouldClose() const;
        bool IsVisible() const;
        bool IsMaximized() const;
        bool IsMinimized() const;
        bool IsFullscreen() const;

        virtual void SetVerticalSync(bool value);
        virtual void Present();

        /// The dot-per-inch scale factor
        float GetDpiFactor() const;

        /// The scale factor for systems with heterogeneous window and pixel coordinates
        float GetContentScaleFactor() const;

        /// Get the native window handle
        NativeHandle GetNativeHandle() const noexcept;

        //Delegate<void()> SizeChanged;

    protected:
        /// Resizable flag.
        bool resizable = false;
        /// Fullscreen flag.
        bool fullscreen = false;
        bool exclusiveFullscreen = false;

#if defined(ALIMER_GLFW)
    //public:
        //GLFWwindow* GetWindow() const { return window; }

    private:
        std::string title;
        GLFWwindow* window = nullptr;
#else
        void* window = nullptr;
#endif
    };
}
