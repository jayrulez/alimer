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
    #include "Platform/glfw/GLFW_Window.h"
#endif

namespace alimer
{
    Window::Window(const String& title, int32_t x, int32_t y, uint32_t width, uint32_t height, WindowFlags flags)
        : impl(new WindowImpl(title, x, y, width, height, flags))
    {
    }

    bool Window::IsOpen() const noexcept { return impl != nullptr && impl->IsOpen(); }

    uint32_t Window::GetId() const noexcept { return impl->GetId(); }

    WindowHandle Window::GetHandle() const { return impl->GetHandle(); }

    void Window::SetBrightness(float value) { return impl->SetBrightness(value); }

    float Window::GetBrightness() const noexcept { return impl->GetBrightness(); }

    void Window::SetSize(uint32_t width, uint32_t height) noexcept { impl->SetSize(Extent2D(width, height)); }

    void Window::SetSize(const Extent2D& size) noexcept { impl->SetSize(size); }

    Extent2D Window::GetSize() const noexcept { return impl->GetSize(); }

    void Window::SetMaximumSize(uint32_t width, uint32_t height) noexcept
    {
        impl->SetMaximumSize(Extent2D(width, height));
    }

    void Window::SetMaximumSize(const Extent2D& size) noexcept { impl->SetMaximumSize(size); }

    Extent2D Window::GetMaximumSize() const noexcept { return impl->GetMaximumSize(); }

    void Window::SetMinimumSize(uint32_t width, uint32_t height) noexcept
    {
        impl->SetMinimumSize(Extent2D(width, height));
    }

    void Window::SetMinimumSize(const Extent2D& size) noexcept { impl->SetMinimumSize(size); }

    Extent2D Window::GetMinimumSize() const noexcept { return impl->GetMinimumSize(); }

    std::string Window::GetTitle() const noexcept { return impl->GetTitle(); }

    void Window::SetTitle(const std::string& str) noexcept { impl->SetTitle(str); }
}
