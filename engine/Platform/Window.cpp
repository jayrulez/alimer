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

#include "Platform/Window.h"

#if defined(GLFW_BACKEND)
#   include "Platform/glfw/GLFW_Window.h"
#endif

namespace Alimer
{
    Window::Window(const std::string& title, int32_t x, int32_t y, uint32_t width, uint32_t height, WindowFlags flags)
        : impl(new WindowImpl(title, x, y, width, height, flags))
    {
    }

    Window::~Window()
    {
        SafeDelete(impl);
    }

    bool Window::IsOpen() const noexcept
    {
        return impl != nullptr && impl->IsOpen();
    }

    uint32_t Window::GetId() const noexcept
    {
        return impl->GetId();
    }

    NativeHandle Window::GetNativeHandle() const
    {
        return impl->GetNativeHandle();
    }

    NativeDisplay Window::GetNativeDisplay() const
    {
        return impl->GetNativeDisplay();
    }

    void Window::SetBrightness(float value)
    {
        return impl->SetBrightness(value);
    }

    float Window::GetBrightness() const noexcept
    {
        return impl->GetBrightness();
    }

    void Window::SetSize(uint32_t width, uint32_t height) noexcept
    {
        impl->SetSize(SizeI(width, height));
    }

    void Window::SetSize(const SizeI& size) noexcept
    {
        impl->SetSize(size);
    }

    SizeI Window::GetSize() const noexcept
    {
        return impl->GetSize();
    }
}
