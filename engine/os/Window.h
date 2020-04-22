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

#include "core/Utils.h"
#include "math/math.h"
#include "math/size.h"
#include <string>

#if ALIMER_WINDOWS
typedef struct HWND__* HWND;
typedef struct HMONITOR__* HMONITOR;
#elif ALIMER_UWP
#include <Inspectable.h>
#elif ALIMER_ANDROID
typedef struct ANativeWindow ANativeWindow;
#elif ALIMER_LINUX && defined(ALIMER_WAYLAND)
struct wl_surface;
struct wl_display;
#endif

#if defined(GLFW_BACKEND)
struct GLFWwindow;
#endif

namespace alimer
{
#if ALIMER_WINDOWS
    // Window handle (HWND)
    using WindowHandle = HWND;

    // Monitor handle (HMONITOR)
    using MonitorHandle = HMONITOR;
#elif ALIMER_UWP
    // Window handle (IUnknown - CoreWindow)
    using WindowHandle = SDL_SysWMinfo*;
#elif ALIMER_ANDROID
    // Window handle (IInspectable*)
    using WindowHandle = IInspectable*;
#elif ALIMER_LINUX
#if defined(ALIMER_WAYLAND)
    using WindowHandle = wl_surface*;
    using DisplayHandle = wl_display*;
#else
    using WindowHandle = uintptr_t;
    using DisplayHandle = uintptr_t;
#endif
#endif

    enum class WindowStyle : uint32_t
    {
        None = 0,
        Resizable = 1 << 0,
        Fullscreen = 1 << 1,
        ExclusiveFullscreen = 1 << 2,
        Hidden = 1 << 3,
        Borderless = 1 << 4,
        Minimized = 1 << 5,
        Maximized = 1 << 6,
    };
    ALIMER_DEFINE_ENUM_BITWISE_OPERATORS(WindowStyle);

    constexpr static const int32_t centered = std::numeric_limits<int32_t>::max();

    /// Defines an OS Window.
    class ALIMER_API Window final
    {
    public:
        /// Constructor.
        Window(const std::string& title, const usize& size, WindowStyle style);

        /// Constructor.
        Window(const std::string& title, int32_t x, int32_t y, uint32_t w, uint32_t h, WindowStyle style);

        /// Constructor.
        Window(const std::string& title, const int2& pos, const usize& size, WindowStyle style);

        /// Destructor.
        ~Window();

        /// Close the window.
        void Close();

        /// Return the window id.
        uint32_t GetId() const noexcept { return id; }

        /// Return whether or not the window is open.
        bool IsOpen() const noexcept;

        /// Return whether the window is minimized.
        bool IsMinimized() const noexcept;

        /// Return whether the window is maximized.
        bool IsMaximized() const noexcept;

        void SetPosition(int32_t x, int32_t y) noexcept;
        void SetPosition(const int2& pos) noexcept;
        int2 GetPosition() const noexcept;

        /// Get the window size.
        usize GetSize() const noexcept;

        /// Return the window title.
        std::string GetTitle() const noexcept;

        /// Set the window title.
        void SetTitle(const std::string& newTitle) noexcept;

        WindowHandle GetHandle() const;
#if ALIMER_WINDOWS
        MonitorHandle GetMonitor() const;
#elif ALIMER_LINUX
        DisplayHandle GetDisplay() const;
#endif

#if defined(GLFW_BACKEND)
        inline GLFWwindow* GetImpl() const { return window; }
#endif

    private:
        void Create(WindowStyle style);

        uint32_t id = 0;
        std::string title;
        usize size;
        bool resizable = false;
        bool fullscreen = false;
        bool exclusiveFullscreen = false;
        bool visible = true;
        bool borderless = false;

#if defined(GLFW_BACKEND)
        GLFWwindow* window = nullptr;
#endif
    };
}
