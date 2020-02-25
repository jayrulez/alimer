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

#include "GLFW_Window.h"
#include "Application/Application.h"
#include "Core/Platform.h"
#include "Core/Log.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace Alimer
{
    GLFW_Window::GLFW_Window(const eastl::string& newTitle, uint32_t newWidth, uint32_t newHeight, WindowStyle style)
        : Window(newTitle, newWidth, newHeight, style)
    {
        GLFWmonitor* monitor = nullptr;
        if (fullscreen)
        {
            monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            newWidth = mode->width;
            newHeight = mode->height;
        }
        else
        {
            glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
            glfwWindowHint(GLFW_VISIBLE, visible ? GLFW_TRUE : GLFW_FALSE);
            //glfwWindowHint(GLFW_DECORATED, any(style & WindowStyle::Decorated) ? GLFW_TRUE : GLFW_FALSE);
        }

        window = glfwCreateWindow(newWidth, newHeight, newTitle.c_str(), NULL, nullptr);

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
    }

    GLFW_Window::~GLFW_Window()
    {
        glfwDestroyWindow(window);
    }

    bool GLFW_Window::ShouldClose() const
    {
        return glfwWindowShouldClose(window);
    }
}
