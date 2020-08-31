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

#include "platform/platform.h"
#include "application.h"
#include "core/log.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#ifdef _WIN32
#   define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>

using namespace Alimer;

namespace
{
    GLFWwindow* window = nullptr;

    void on_glfw_error(int code, const char* description)
    {
        LOGE("GLFW Error {} - {}", description, code);
    }
}

bool Platform::init(const Config* config)
{
    glfwSetErrorCallback(on_glfw_error);
#ifdef __APPLE__
    glfwInitHint(GLFW_COCOA_CHDIR_RESOURCES, GLFW_FALSE);
#endif
    if (!glfwInit())
    {
        return false;
    }

    if (config->graphics_backend == graphics::BackendType::OpenGL)
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, config->debug);
        glfwWindowHint(GLFW_CONTEXT_NO_ERROR, !config->debug);
        glfwWindowHint(GLFW_SAMPLES, config->sample_count);
        glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
    }
    else
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    uint32_t width = config->width ? config->width : (uint32_t)mode->width;
    uint32_t height = config->height ? config->height : (uint32_t)mode->height;

    if (config->fullscreen) {
        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    }

    window = glfwCreateWindow(width, height, config->title, config->fullscreen ? monitor : nullptr, nullptr);

    if (!window) {
        return false;
    }

    if (config->graphics_backend == graphics::BackendType::OpenGL)
    {
        glfwMakeContextCurrent(window);
        glfwSwapInterval(config->vsync ? 1 : 0);
    }

    return true;
}

void Platform::shutdown(void) noexcept
{
    glfwTerminate();
}

void Platform::run(void)
{
    while (!glfwWindowShouldClose(window))
    {
        App::tick();
        glfwPollEvents();
    }
}

void* Platform::get_gl_proc_address(const char* name)
{
    return (void*)glfwGetProcAddress(name);
}

void Platform::swap_buffers(void)
{
    glfwSwapBuffers(window);
}

#if defined(GLFW_EXPOSE_NATIVE_WIN32)
HWND Platform::get_native_handle(void) noexcept
{
    return glfwGetWin32Window(window);
}
#endif
