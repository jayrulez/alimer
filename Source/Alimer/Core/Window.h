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
#include "Core/Ptr.h"
#include "Core/Math.h"
#include "Core/Delegate.h"

#if defined(ALIMER_GLFW)
struct GLFWwindow;
#endif

#if ALIMER_PLATFORM_WINDOWS
struct WindowHandle
{
    HWND window;
    HINSTANCE hinstance;
};
#elif ALIMER_PLATFORM_UWP
struct WindowHandle
{
    IUnknown* window;
};
#elif ALIMER_PLATFORM_LINUX
struct WindowHandle
{
    Display* display;
    Window window;
};
#elif ALIMER_PLATFORM_MACOS
struct WindowHandle
{
#if defined(__OBJC__) && defined(__has_feature) && __has_feature(objc_arc)
    NSWindow __unsafe_unretained* window;
#else
    NSWindow* window;
#endif
};
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
        Maximized = (1 << 6)
    };
    ALIMER_DEFINE_ENUM_FLAG_OPERATORS(WindowFlags, uint32_t);

    /// Defines an OS window.
    class ALIMER_API Window final : public RefCounted
    {
    public:
        Window() = default;
        ~Window() override;
        void Close();
        void BeginFrame();

        /// Set window size. Creates the window if not created yet. Return true on success.
        bool SetSize(const UInt2& size, WindowFlags flags = WindowFlags::None);

        /// Set window title.
        void SetTitle(const String& newTitle);

        /// Return window title.
        const String& GetTitle() const { return title; }
        /// Return window client size.
        const UInt2& GetSize() const { return size; }
        /// Return window client area width.
        uint32_t GetWidth() const { return size.x; }
        /// Return window client area height.
        uint32_t GetHeight() const { return size.y; }

        bool ShouldClose() const;
        bool IsVisible() const;
        bool IsMaximized() const;
        bool IsMinimized() const;
        bool IsFullscreen() const;

        /// Get the native window handle
        bool GetHandle(WindowHandle* handle) const;

        //Delegate<void()> SizeChanged;

    private:
        String title = "Alimer";
        UInt2 size = UInt2::Zero;
        /// Resizable flag.
        bool resizable = false;
        /// Fullscreen flag.
        bool fullscreen = false;
        bool exclusiveFullscreen = false;

#if defined(ALIMER_GLFW)
    //public:
        //GLFWwindow* GetWindow() const { return window; }

    private:
        GLFWwindow* window = nullptr;
#else
        void* window = nullptr;
#endif
    };
}
