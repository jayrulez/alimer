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

#include "glfw_window.h"
#include "Core/Platform.h"
#include "Diagnostics/Log.h"
#if defined(_WIN32)
#   define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#   define GLFW_EXPOSE_NATIVE_X11
#elif defined(__APPLE__)
#   define GLFW_EXPOSE_NATIVE_COCOA
#endif
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace alimer
{

    WindowImpl::WindowImpl(bool opengl_, const std::string& title, int32_t x, int32_t y, uint32_t width, uint32_t height, WindowStyle style)
        : opengl(opengl_)
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        if (opengl_) {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        }

        auto visible = any(style & WindowStyle::Hidden) ? GLFW_FALSE : GLFW_TRUE;
        glfwWindowHint(GLFW_VISIBLE, visible);

        auto decorated = any(style & WindowStyle::Borderless) ? GLFW_FALSE : GLFW_TRUE;
        glfwWindowHint(GLFW_DECORATED, decorated);

        auto resizable = any(style & WindowStyle::Resizable) ? GLFW_TRUE : GLFW_FALSE;
        glfwWindowHint(GLFW_RESIZABLE, resizable);

        auto maximized = any(style & WindowStyle::Minimized) ? GLFW_TRUE : GLFW_FALSE;
        glfwWindowHint(GLFW_MAXIMIZED, maximized);

        GLFWmonitor* monitor = nullptr;
        if (any(style & WindowStyle::Fullscreen))
        {
            monitor = glfwGetPrimaryMonitor();
        }

        if (any(style & WindowStyle::FullscreenDesktop))
        {
            monitor = glfwGetPrimaryMonitor();
            auto mode = glfwGetVideoMode(monitor);

            glfwWindowHint(GLFW_RED_BITS, mode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

            glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        }

        window_ = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.c_str(), monitor, nullptr);
        if (!window_)
        {
            ALIMER_LOGERROR("GLFW: Failed to create window.");
        }

        glfwDefaultWindowHints();

        title_ = title;
        id_ = impl::register_window(this);
        glfwSetWindowUserPointer(window_, this);

        /*glfwSetWindowCloseCallback(window, window_close_callback);
        glfwSetWindowSizeCallback(window, window_size_callback);
        glfwSetWindowFocusCallback(window, window_focus_callback);
        glfwSetKeyCallback(window, key_callback);
        glfwSetCursorPosCallback(window, cursor_position_callback);
        glfwSetMouseButtonCallback(window, mouse_button_callback);*/

        set_position(x, y);

        if (opengl) {
            glfwMakeContextCurrent(window_);
            glfwSwapInterval(1);
        }
    }

    WindowImpl::~WindowImpl()
    {
        impl::unregister_window(id_);
        glfwDestroyWindow(window_);
    }

    native_handle WindowImpl::get_native_handle() const
    {
#if defined(GLFW_EXPOSE_NATIVE_WIN32)
        return glfwGetWin32Window(window_);
#elif defined(GLFW_EXPOSE_NATIVE_X11)
        return (void*)(uintptr_t)glfwGetX11Window(window_);
#elif defined(GLFW_EXPOSE_NATIVE_COCOA)
        return glfwGetCocoaWindow(window_);
#elif defined(GLFW_EXPOSE_NATIVE_WAYLAND)
        return glfwGetWaylandWindow(window_);
#else
        return nullptr;
#endif
    }

    native_display WindowImpl::get_native_display() const
    {
#if defined(GLFW_EXPOSE_NATIVE_WIN32)
        return nullptr;
#elif defined(GLFW_EXPOSE_NATIVE_X11)
        return (void*)(uintptr_t)glfwGetX11Display();
#elif defined(GLFW_EXPOSE_NATIVE_COCOA)
        return nullptr;
#elif defined(GLFW_EXPOSE_NATIVE_WAYLAND)
        return glfwGetWaylandDisplay();
#else
        return nullptr;
#endif
    }
}
