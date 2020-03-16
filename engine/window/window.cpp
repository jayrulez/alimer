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

#include "window/window.h"
#include "graphics/GPUDevice.h"
#include "graphics/SwapChain.h"

#if defined(ALIMER_GLFW)
#include "window/glfw/glfw_window.h"
#endif

using namespace std;

namespace alimer
{
    Window::Window(const string& newTitle, const SizeU& newSize, WindowStyle style)
        : title(newTitle)
        , size(newSize)
        , resizable(any(style& WindowStyle::Resizable))
        , fullscreen(any(style& WindowStyle::Fullscreen))
        , exclusiveFullscreen(any(style& WindowStyle::ExclusiveFullscreen))
        , impl(new WindowImpl(false, newTitle, newSize, style))
    {

    }

    Window::~Window()
    {
        Close();
    }

    void Window::Close()
    {
        // Delete the window implementation
        delete impl;
        impl = nullptr;
    }

    bool Window::IsOpen() const
    {
        return impl != nullptr && impl->IsOpen();
    }

    void Window::set_title(const string& newTitle)
    {
        title = newTitle;
        impl->set_title(title.c_str());
    }

    bool Window::IsMinimized() const
    {
        return impl->IsMinimized();
    }

    native_handle Window::get_native_handle() const
    {
        return impl->get_native_handle();
    }

    native_display Window::get_native_display() const
    {
        return impl->get_native_display();
    }
}
