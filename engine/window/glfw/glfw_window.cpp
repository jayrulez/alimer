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
    WindowImpl::WindowImpl(bool opengl_, const std::string& newTitle, const SizeU& newSize, WindowStyle style)
        : opengl(opengl_)
    {
        int window_width = (int) newSize.width;
        int window_height = (int)newSize.height;
        GLFWmonitor* monitor = nullptr;

        const bool fullscreen = any(style & WindowStyle::Fullscreen);
        const bool resizable = any(style & WindowStyle::Resizable);
        if (fullscreen)
        {
            monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            window_width = mode->width;
            window_height = mode->height;
        }
        else
        {
            glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
            glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
            //glfwWindowHint(GLFW_DECORATED, any(style & WindowStyle::Decorated) ? GLFW_TRUE : GLFW_FALSE);
        }

        window = glfwCreateWindow(window_width, window_height, newTitle.c_str(), NULL, nullptr);

        if (!window)
        {
            ALIMER_LOGERROR("GLFW: Failed to create window.");
        }

        glfwSetWindowUserPointer(window, this);

        /*glfwSetWindowCloseCallback(window, window_close_callback);
        glfwSetWindowSizeCallback(window, window_size_callback);
        glfwSetWindowFocusCallback(window, window_focus_callback);
        glfwSetKeyCallback(window, key_callback);
        glfwSetCursorPosCallback(window, cursor_position_callback);
        glfwSetMouseButtonCallback(window, mouse_button_callback);*/

        glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);
        glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, 1);


        if (opengl) {
            glfwMakeContextCurrent(window);
            glfwSwapInterval(1);
        }
    }

    WindowImpl::~WindowImpl()
    {
        glfwDestroyWindow(window);
    }

    void WindowImpl::set_title(const char* title)
    {
        glfwSetWindowTitle(window, title);
    }

    bool WindowImpl::IsOpen() const
    {
        return !glfwWindowShouldClose(window);
    }

    bool WindowImpl::IsMinimized() const
    {
        return glfwGetWindowAttrib(window, GLFW_ICONIFIED) == GLFW_TRUE;
    }

    void WindowImpl::swap_buffers()
    {
        glfwSwapBuffers(window);
    }

    native_handle WindowImpl::get_native_handle() const
    {
#if defined(GLFW_EXPOSE_NATIVE_WIN32)
        return glfwGetWin32Window(window);
#elif defined(GLFW_EXPOSE_NATIVE_X11)
        return (void*)(uintptr_t)glfwGetX11Window(window);
#elif defined(GLFW_EXPOSE_NATIVE_COCOA)
        return glfwGetCocoaWindow(window);
#elif defined(GLFW_EXPOSE_NATIVE_WAYLAND)
        return glfwGetWaylandWindow(window);
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
