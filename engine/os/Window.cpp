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

#include "os/Window.h"

namespace alimer
{
    Window::Window(const std::string& title, const usize& size, WindowStyle style)
        : Window(title, int2{ centered, centered }, size, style)
    {
    }

    Window::Window(const std::string& title, int32_t x, int32_t y, uint32_t w, uint32_t h, WindowStyle style)
        : Window(title, int2{ x, y }, usize{ w, h }, style)
    {
    }

    Window::Window(const std::string& title, const int2& pos, const usize& size, WindowStyle style)
        : title{ title }
        , size{ size }
        , resizable(any(style& WindowStyle::Resizable))
        , fullscreen(any(style& WindowStyle::Fullscreen))
        , exclusiveFullscreen(any(style& WindowStyle::ExclusiveFullscreen))
        , visible(!any(style& WindowStyle::Hidden))
        , borderless(any(style& WindowStyle::Borderless))
    {
        Create(style);
        SetPosition(pos);
    }

    Window::~Window()
    {
        Close();
    }

    usize Window::GetSize() const noexcept
    {
        return size;
    }

    std::string Window::GetTitle() const noexcept
    {
        return title;
    }
}
