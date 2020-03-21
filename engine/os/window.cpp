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

#include "os/window.h"

#if defined(GLFW_BACKEND)
#   include "glfw/glfw_window.h"
#elif defined(SDL_BACKEND)
#   include "sdl/WindowImplSDL.hpp"
#endif

using namespace std;

namespace alimer
{
    Window::Window(const string& title, const usize& newSize, WindowStyle style)
    {
        create(title, { centered, centered }, newSize, style);
    }

    bool Window::create(const std::string& title, const point& pos, const usize& size, WindowStyle style)
    {
        close();

        impl = new WindowImpl(title, pos, size, style);
        return impl != nullptr;
    }

    Window::~Window()
    {
        close();
    }

    void Window::close()
    {
        if (impl != nullptr)
        {
            delete impl;
            impl = nullptr;
        }
    }

    native_handle Window::get_native_handle() const
    {
        return impl->get_native_handle();
    }

    native_display Window::get_native_display() const
    {
        return impl->get_native_display();
    }

    auto Window::get_id() const noexcept -> uint32_t
    {
        return impl->get_id();
    }

    auto Window::is_open() const noexcept -> bool
    {
        return impl != nullptr && impl->is_open();
    }

    auto Window::is_minimized() const noexcept -> bool
    {
        return impl->is_minimized();
    }

    auto Window::is_maximized() const noexcept -> bool
    {
        return impl->is_maximized();
    }

    auto Window::get_size() const noexcept -> usize
    {
        return impl->get_size();
    }

    auto Window::get_title() const noexcept -> std::string
    {
        return impl->get_title();
    }

    void Window::set_title(const std::string& title) noexcept
    {
        impl->set_title(title);
    }
}
