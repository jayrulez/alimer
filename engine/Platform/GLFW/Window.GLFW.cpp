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

#include "Application/Window.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/SwapChain.h"
#include "Core/Log.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#if defined(_WIN32)
#   define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#   define GLFW_EXPOSE_NATIVE_X11
#elif defined(__APPLE__)
#   define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>
#include "imgui_impl_glfw.h"

namespace alimer
{
    namespace
    {
        uint32_t windowCount = 0;

        static const char* ImGui_ImplGlfw_GetClipboardText(void* user_data)
        {
            return glfwGetClipboardString((GLFWwindow*)user_data);
        }

        static void ImGui_ImplGlfw_SetClipboardText(void* user_data, const char* text)
        {
            glfwSetClipboardString((GLFWwindow*)user_data, text);
        }
    }

    bool Window::Create(const String& title, const SizeI& size, WindowFlags flags)
    {
        this->title = title;
        this->size = size;
        fullscreen = any(flags & WindowFlags::Fullscreen);
        exclusiveFullscreen = any(flags & WindowFlags::ExclusiveFullscreen);

        /*if ((flags & WINDOW_FLAG_OPENGL)) {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        }
        else
        {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        }*/

        glfwWindowHint(GLFW_RESIZABLE, any(flags & WindowFlags::Resizable) ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_VISIBLE, !any(flags & WindowFlags::Hidden));
        //glfwWindowHint(GLFW_DECORATED, (flags & WINDOW_FLAG_BORDERLESS) ? GLFW_FALSE : GLFW_TRUE);

        if (any(flags & WindowFlags::Minimized))
        {
            glfwWindowHint(GLFW_ICONIFIED, GLFW_TRUE);
        }
        else if (any(flags & WindowFlags::Maximized))
        {
            glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
        }

        GLFWmonitor* monitor = fullscreen ? glfwGetPrimaryMonitor() : nullptr;

        if (exclusiveFullscreen)
        {
            monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);

            glfwWindowHint(GLFW_RED_BITS, mode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

            glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        }

        GLFWwindow* handle = glfwCreateWindow(static_cast<int>(size.width), static_cast<int>(size.height), title.c_str(), monitor, NULL);
        if (!handle)
        {
            LOG_ERROR("GLFW: Failed to create window.");
            return false;
        }

        /*if ((flags & WINDOW_FLAG_OPENGL)) {
            glfwMakeContextCurrent(handle);
            glfwSwapInterval(1);
        }*/

        glfwDefaultWindowHints();
        glfwSetWindowUserPointer(handle, this);
        //glfwSetKeyCallback(handle, glfw_key_callback);
        window = handle;

        // Init imgui stuff
        ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)window, true);

        // Create SwapChain
        SwapChainDescription swapChainDesc = {};
        swapChainDesc.width = size.width;
        swapChainDesc.height = size.height;
        swapChainDesc.windowHandle = GetHandle();
        _swapChain = GraphicsDevice::Instance->CreateSwapChain(swapChainDesc);

        if (windowCount == 0) {
            _isMain = true;
        }

        windowCount++;

        return true;
    }

    void Window::Close()
    {
        glfwSetWindowShouldClose((GLFWwindow*)window, GLFW_TRUE);
        _swapChain.Reset();
        windowCount--;
    }

    bool Window::ShouldClose() const
    {
        return glfwWindowShouldClose((GLFWwindow*)window) == GLFW_TRUE;
    }

    bool Window::IsVisible() const
    {
        return glfwGetWindowAttrib((GLFWwindow*)window, GLFW_VISIBLE) == GLFW_TRUE;
    }

    bool Window::IsMaximized() const
    {
        return glfwGetWindowAttrib((GLFWwindow*)window, GLFW_ICONIFIED) == GLFW_TRUE;
    }

    bool Window::IsMinimized() const
    {
        return glfwGetWindowAttrib((GLFWwindow*)window, GLFW_MAXIMIZED) == GLFW_TRUE;
    }

    bool Window::IsFullscreen() const
    {
        return fullscreen || exclusiveFullscreen;
    }

    bool Window::IsMain() const
    {
        return _isMain;
    }

    uintptr_t Window::GetHandle() const
    {
#if defined(GLFW_EXPOSE_NATIVE_WIN32)
        return (uintptr_t)glfwGetWin32Window((GLFWwindow*)window);
#elif defined(GLFW_EXPOSE_NATIVE_X11)
        return (uintptr_t)glfwGetX11Window((GLFWwindow*)window);
#elif defined(GLFW_EXPOSE_NATIVE_COCOA)
        return (uintptr_t)glfwGetCocoaWindow((GLFWwindow*)window);
#else
        return 0;
#endif
}
}
