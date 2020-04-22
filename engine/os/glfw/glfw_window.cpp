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

#include "config.h"
#include "core/Platform.h"
#include "core/Log.h"
#include "os/Window.h"
#include "os_glfw.h"
#include <algorithm>

using namespace std;

namespace alimer
{
    namespace impl
    {
        inline vector<Window*>& get_windows() noexcept
        {
            static vector<Window*> windows;
            return windows;
        }

        inline uint32_t register_window(Window* window)
        {
            static uint32_t id{ 0 };
            auto& windows = get_windows();
            windows.emplace_back(window);
            return ++id;
        }

        inline void unregister_window(uint32_t id)
        {
            auto& windows = get_windows();
            windows.erase(remove_if(begin(windows), end(windows),
                [id](const auto& win) { return win->GetId() == id; }),
                end(windows)
            );
        }

        inline bool glfwSetWindowCenter(GLFWwindow* window)
        {
            if (!window)
                return false;

            int sx = 0, sy = 0;
            int px = 0, py = 0;
            int mx = 0, my = 0;
            int monitor_count = 0;
            int best_area = 0;
            int final_x = 0, final_y = 0;

            glfwGetWindowSize(window, &sx, &sy);
            glfwGetWindowPos(window, &px, &py);

            // Iterate throug all monitors
            GLFWmonitor** m = glfwGetMonitors(&monitor_count);
            if (!m)
                return false;

            for (int j = 0; j < monitor_count; ++j)
            {

                glfwGetMonitorPos(m[j], &mx, &my);
                const GLFWvidmode* mode = glfwGetVideoMode(m[j]);
                if (!mode)
                    continue;

                // Get intersection of two rectangles - screen and window
                int minX = std::max(mx, px);
                int minY = std::max(my, py);

                int maxX = std::min(mx + mode->width, px + sx);
                int maxY = std::min(my + mode->height, py + sy);

                // Calculate area of the intersection
                int area = std::max(maxX - minX, 0) * std::max(maxY - minY, 0);

                // If its bigger than actual (window covers more space on this monitor)
                if (area > best_area)
                {
                    // Calculate proper position in this monitor
                    final_x = mx + (mode->width - sx) / 2;
                    final_y = my + (mode->height - sy) / 2;

                    best_area = area;
                }
            }

            // We found something
            if (best_area)
                glfwSetWindowPos(window, final_x, final_y);

            // Something is wrong - current window has NOT any intersection with any monitors. Move it to the default
            // one.
            else
            {
                GLFWmonitor* primary = glfwGetPrimaryMonitor();
                if (primary)
                {
                    const GLFWvidmode* desktop = glfwGetVideoMode(primary);

                    if (desktop)
                        glfwSetWindowPos(window, (desktop->width - sx) / 2, (desktop->height - sy) / 2);
                    else
                        return false;
                }
                else
                    return false;
            }

            return true;
        }
    }

    void Window::Create(WindowStyle style)
    {
#if defined(ALIMER_GRAPHICS_OPENGL)
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#else
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif

        glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_VISIBLE, visible ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_DECORATED, borderless ? GLFW_FALSE : GLFW_TRUE);

        if (any(style & WindowStyle::Minimized))
        {
            glfwWindowHint(GLFW_ICONIFIED, GLFW_TRUE);
        }
        else if (any(style & WindowStyle::Maximized))
        {
            glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
        }

        GLFWmonitor* monitor = nullptr;
        if (any(style & WindowStyle::Fullscreen))
        {
            monitor = glfwGetPrimaryMonitor();
        }

        if (any(style & WindowStyle::ExclusiveFullscreen))
        {
            monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);

            glfwWindowHint(GLFW_RED_BITS, mode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

            glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        }

        window = glfwCreateWindow(static_cast<int>(size.width), static_cast<int>(size.height), title.c_str(), monitor, nullptr);
        if (!window)
        {
            ALIMER_LOGERROR("GLFW: Failed to create window.");
            return;
        }

        glfwDefaultWindowHints();
        glfwSetWindowUserPointer(window, this);
        id = impl::register_window(this);

        glfwSetWindowCloseCallback(window, [](GLFWwindow* window) {
            auto user_data = glfwGetWindowUserPointer(window);
            auto impl = reinterpret_cast<Window*>(user_data);

            os::Event ev{};
            ev.type = os::Event::Type::Window;
            ev.window.windowId = impl->GetId();
            ev.window.type = os::WindowEventId::Close;

            os::push_event(std::move(ev));
            }
        );

        /*glfwSetWindowCloseCallback(window, window_close_callback);
        glfwSetWindowSizeCallback(window, window_size_callback);
        glfwSetWindowFocusCallback(window, window_focus_callback);
        glfwSetKeyCallback(window, key_callback);
        glfwSetCursorPosCallback(window, cursor_position_callback);
        glfwSetMouseButtonCallback(window, mouse_button_callback);*/

        //set_position(pos.x, pos.y);
        int w;
        int h;
        glfwGetWindowSize(window, &w, &h);
        size.width = static_cast<uint32_t>(w);
        size.height = static_cast<uint32_t>(h);
    }

    void Window::Close()
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        glfwDestroyWindow(window);
        impl::unregister_window(id);
    }

    bool Window::IsOpen() const noexcept
    {
        return window != nullptr && !glfwWindowShouldClose(window);
    }

    bool Window::IsMinimized() const noexcept
    {
        return glfwGetWindowAttrib(window, GLFW_ICONIFIED) == GLFW_TRUE;
    }

    bool Window::IsMaximized() const noexcept
    {
        return glfwGetWindowAttrib(window, GLFW_MAXIMIZED) == GLFW_TRUE;
    }

    void Window::SetPosition(int32_t x, int32_t y) noexcept
    {
        if (x == centered && y == centered)
        {
            impl::glfwSetWindowCenter(window);
        }
        else
        {
            glfwSetWindowPos(window, x, y);
        }
    }

    void Window::SetPosition(const int2& pos) noexcept
    {
        SetPosition(pos.x, pos.y);
    }

    int2 Window::GetPosition() const noexcept
    {
        int2 result;
        glfwGetWindowPos(window, &result.x, &result.y);
        return result;
    }

    void Window::SetTitle(const std::string& newTitle) noexcept
    {
        title = newTitle;
        glfwSetWindowTitle(window, newTitle.c_str());
    }


    WindowHandle Window::GetHandle() const
    {
#if defined(GLFW_EXPOSE_NATIVE_WIN32)
        return glfwGetWin32Window(window);
#elif defined(GLFW_EXPOSE_NATIVE_X11)
        return (uintptr_t)glfwGetX11Window(window);
#elif defined(GLFW_EXPOSE_NATIVE_COCOA)
        return glfwGetCocoaWindow(window);
#elif defined(GLFW_EXPOSE_NATIVE_WAYLAND)
        return glfwGetWaylandWindow(window);
#else
        return {};
#endif
    }

#if ALIMER_WINDOWS
    MonitorHandle Window::GetMonitor() const
    {
        return MonitorFromWindow(glfwGetWin32Window(window), MONITOR_DEFAULTTOPRIMARY);
    }

#elif ALIMER_LINUX
    DisplayHandle Window::GetDisplay() const
    {
#if defined(GLFW_EXPOSE_NATIVE_X11)
        return (uintptr_t)glfwGetX11Display();
#elif defined(GLFW_EXPOSE_NATIVE_WAYLAND)
        return glfwGetWaylandDisplay();
#else
        return {};
#endif
    }
#endif

    namespace os
    {
        void pump_events() noexcept
        {
            glfwPollEvents();

            // Fire quit event when all windows are closed.
            auto& windows = impl::get_windows();
            auto all_closed = std::all_of(std::begin(windows), std::end(windows),
                [](const auto& e) { return glfwWindowShouldClose(e->GetImpl()); });
            if (all_closed)
            {
                Event evt = {};
                evt.type = Event::Type::Quit;
                push_event(evt);
            }
        }
    }
}
