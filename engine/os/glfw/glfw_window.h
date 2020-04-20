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

#include "os_glfw.h"
#include "os/window.h"
#include <algorithm>
#include <vector>

namespace alimer
{
    namespace impl
    {
        inline auto get_windows() noexcept -> std::vector<WindowImpl*>&
        {
            static std::vector<WindowImpl*> windows;
            return windows;
        }

        inline auto register_window(WindowImpl* impl) -> uint32_t
        {
            static uint32_t id{ 0 };
            auto& windows = get_windows();
            windows.emplace_back(impl);
            return ++id;
        }

        inline void unregister_window(uint32_t id)
        {
            auto& windows = get_windows();
            windows.erase(std::remove_if(std::begin(windows), std::end(windows),
                [id](const auto& win) { return win->get_id() == id; }),
                std::end(windows));
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

    class WindowImpl final
    {
    public:
        WindowImpl(const std::string& title, const int2& pos, const usize& size, WindowStyle style);
        ~WindowImpl();

        auto get_id() const noexcept -> uint32_t
        {
            return id_;
        }

        void* GetNativeHandle() const
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

        void* GetNativeDisplay() const
        {
#if defined(GLFW_EXPOSE_NATIVE_WIN32)
            return (HINSTANCE)GetWindowLongPtrW(glfwGetWin32Window(window_), GWLP_HINSTANCE);
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

        auto is_open() const noexcept -> bool
        {
            return !glfwWindowShouldClose(window_);
        }

        auto is_minimized() const noexcept -> bool
        {
            return glfwGetWindowAttrib(window_, GLFW_ICONIFIED) == GLFW_TRUE;
        }

        auto is_maximized() const noexcept -> bool
        {
            return glfwGetWindowAttrib(window_, GLFW_MAXIMIZED) == GLFW_TRUE;
        }

        auto get_size() const noexcept -> usize
        {
            int w;
            int h;
            glfwGetWindowSize(window_, &w, &h);
            usize result{};
            result.width = static_cast<uint32_t>(w);
            result.height = static_cast<uint32_t>(h);
            return result;
        }

        void set_position(int32_t x, int32_t y) noexcept
        {
            if (x == centered && y == centered)
            {
                impl::glfwSetWindowCenter(window_);
            }
            else
            {
                glfwSetWindowPos(window_, static_cast<int>(x), static_cast<int>(y));
            }
        }

        auto get_title() const noexcept -> std::string
        {
            return title_;
        }

        void set_title(const std::string& str) noexcept
        {
            title_ = str;
            glfwSetWindowTitle(window_, str.c_str());
        }

        auto get_impl() const noexcept -> GLFWwindow*
        {
            return window_;
        }

    private:
        uint32_t id_{};
        GLFWwindow* window_ = nullptr;
        std::string title_{};
    };
}
