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

#include "Core/Log.h"
#include "GLFW_Window.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#ifdef _WIN32
#   define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>

namespace Alimer
{
    namespace
    {
        uint32 s_windowCount = 0;
        void OnGLFWError(int code, const char* description)
        {
            LOGE("GLFW Error {} - {}", description, code);
        }

    }

    GLFW_Window::GLFW_Window(const char* title, uint32 width, uint32 height, bool resizable, bool fullscreen)
        : Window(width, height)
    {
        // Init glfw at first call
        if (s_windowCount == 0)
        {
            glfwSetErrorCallback(OnGLFWError);
#ifdef __APPLE__
            glfwInitHint(GLFW_COCOA_CHDIR_RESOURCES, GLFW_FALSE);
#endif
            if (!glfwInit())
            {
                throw std::runtime_error("GLFW couldn't be initialized.");
            }
        }

        const bool opengl = false;

        if (opengl)
        {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
            //glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, config->debug);
            //glfwWindowHint(GLFW_CONTEXT_NO_ERROR, !config->debug);
            //glfwWindowHint(GLFW_SAMPLES, config->sample_count);
            //glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
        }
        else
        {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        }

        glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        width = width ? width : (uint32)mode->width;
        height = height ? height : (uint32)mode->height;

        if (fullscreen) {
            glfwWindowHint(GLFW_RED_BITS, mode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
        }

        handle = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title, fullscreen ? monitor : nullptr, nullptr);

        if (!handle)
        {
            throw std::runtime_error("Couldn't create glfw window.");
        }

        glfwDefaultWindowHints();

        if (opengl)
        {
            glfwMakeContextCurrent(handle);
            //glfwSwapInterval(config->vsync ? 1 : 0);
        }

        s_windowCount++;
    }

    GLFW_Window::~GLFW_Window()
    {
        s_windowCount--;

        if (s_windowCount == 0) {
            glfwTerminate();
        }
    }

    void GLFW_Window::PollEvents()
    {
        glfwPollEvents();
    }

    bool GLFW_Window::IsOpen() const
    {
        return !glfwWindowShouldClose(handle);
    }
}