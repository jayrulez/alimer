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
#include "core/Platform.h"
#include "core/Log.h"

namespace alimer
{

    WindowImpl::WindowImpl(const std::string& title, const int2& pos, const usize& size, WindowStyle style)
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        /*if (opengl_) {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        }*/

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

        window_ = glfwCreateWindow(static_cast<int>(size.width), static_cast<int>(size.height), title.c_str(), monitor, nullptr);
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

        set_position(pos.x, pos.y);
    }

    WindowImpl::~WindowImpl()
    {
        impl::unregister_window(id_);
        glfwDestroyWindow(window_);
    }
}
