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
#include "Platform/Event.h"

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

        std::vector<WindowImpl*>& GetWindows() noexcept
        {
            static std::vector<WindowImpl*> windows;
            return windows;
        }


        inline uint32_t RegisterWindow(WindowImpl* impl)
        {
            static uint32_t id{ 0 };
            auto& windows = GetWindows();
            windows.emplace_back(impl);
            return ++id;
        }

        inline void UnregisterWindow(uint32_t id)
        {
            auto& windows = GetWindows();
            windows.erase(std::remove_if(std::begin(windows), std::end(windows),
                [id](const auto& win) { return win->GetId() == id; }),
                std::end(windows)
            );
        }
    }

    WindowImpl::WindowImpl(const std::string& title, int32_t x, int32_t y, uint32_t width, uint32_t height, WindowFlags flags)
        : title{ title }
        , id{}
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
                LOGE("GLFW couldn't be initialized.");
            }
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        auto visible = any(flags & WindowFlags::Hidden) ? GLFW_FALSE : GLFW_TRUE;
        glfwWindowHint(GLFW_VISIBLE, visible);

        auto decorated = any(flags & WindowFlags::Borderless) ? GLFW_FALSE : GLFW_TRUE;
        glfwWindowHint(GLFW_DECORATED, decorated);

        auto resizable = any(flags & WindowFlags::Resizable) ? GLFW_TRUE : GLFW_FALSE;
        glfwWindowHint(GLFW_RESIZABLE, resizable);

        auto maximized = any(flags & WindowFlags::Maximized) ? GLFW_TRUE : GLFW_FALSE;
        glfwWindowHint(GLFW_MAXIMIZED, maximized);

        GLFWmonitor* monitor = nullptr;
        if (any(flags & WindowFlags::Fullscreen))
        {
            monitor = glfwGetPrimaryMonitor();
        }

        if (any(flags & WindowFlags::FullscreenDesktop))
        {
            monitor = glfwGetPrimaryMonitor();
            auto mode = glfwGetVideoMode(monitor);

            glfwWindowHint(GLFW_RED_BITS, mode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

            glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        }

        window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.c_str(), monitor, nullptr);
        if (!window)
        {
            LOGE("Couldn't create glfw window.");
            return;
        }
        glfwDefaultWindowHints();

        id = RegisterWindow(this);
        glfwSetWindowUserPointer(window, this);

        s_windowCount++;
    }

    WindowImpl::~WindowImpl()
    {
        UnregisterWindow(id);

        s_windowCount--;

        if (s_windowCount == 0) {
            glfwTerminate();
        }
    }

    bool WindowImpl::IsOpen() const noexcept
    {
        return !glfwWindowShouldClose(window);
    }

    WindowHandle WindowImpl::GetHandle() const
    {
#if defined(GLFW_EXPOSE_NATIVE_WIN32)
        return glfwGetWin32Window(window);
#elif defined(GLFW_EXPOSE_NATIVE_X11)
        WindowHandle handle{};
        handle.display = glfwGetX11Display(window);
        handle.window = glfwGetX11Window(window);
        return handle;
#elif defined(GLFW_EXPOSE_NATIVE_COCOA)
        return glfwGetCocoaWindow(window);
#elif defined(GLFW_EXPOSE_NATIVE_WAYLAND)
        WindowHandle handle{};
        handle.display = glfwGetWaylandDisplay();
        handle.window = glfwGetWaylandWindow(window);
        return handle;
#else
        return nullptr;
#endif
    }

    void PumpEvents() noexcept
    {
        glfwPollEvents();

        static bool reported = false;
        if (!reported)
        {
            auto& windows = GetWindows();
            auto allClosed = std::all_of(std::begin(windows), std::end(windows), [](const auto& e) { return glfwWindowShouldClose(e->GetGLFWwindow()); });
            if (allClosed)
            {
                reported = true;

                Event evt {};
                evt.type = EventType::Quit;
                PushEvent(std::move(evt));
            }
        }
    }
}
