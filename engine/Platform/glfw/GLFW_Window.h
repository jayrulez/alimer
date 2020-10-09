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

#include "Platform/Window.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace Alimer
{
    class WindowImpl final
    {
    public:
        WindowImpl(const std::string& title, int32_t x, int32_t y, uint32_t width, uint32_t height, WindowFlags flags);
        ~WindowImpl();

        bool IsOpen() const noexcept;
        uint32_t GetId() const noexcept { return id; }

        WindowHandle GetHandle() const;
        GLFWwindow* GetGLFWwindow() const noexcept { return window; }

        float GetBrightness() const noexcept { return 1.0f; }
        void SetBrightness(float) { }

        void SetSize(const SizeI& size) noexcept
        {
            glfwSetWindowSize(window, static_cast<int>(size.width), static_cast<int>(size.height));
        }

        SizeI GetSize() const noexcept
        {
            SizeI result{};
            int w{};
            int h{};
            glfwGetWindowSize(window, &w, &h);
            result.width = static_cast<int32_t>(w);
            result.height = static_cast<int32_t>(h);
            return result;
        }

        void SetMaximumSize(const SizeI& size) noexcept
        {
            maxSize = size;
            glfwSetWindowSizeLimits(window, GLFW_DONT_CARE, GLFW_DONT_CARE, static_cast<int>(size.width), static_cast<int>(size.height));
        }

        SizeI GetMaximumSize() const noexcept
        {
            return maxSize;
        }

        void SetMinimumSize(const SizeI& size) noexcept
        {
            minSize = size;
            glfwSetWindowSizeLimits(window, static_cast<int>(size.width), static_cast<int>(size.height), GLFW_DONT_CARE, GLFW_DONT_CARE);
        }

        SizeI GetMinimumSize() const noexcept
        {
            return minSize;
        }

    private:
        GLFWwindow* window{ nullptr };
        std::string title{};
        uint32_t id;
        SizeI minSize{};
        SizeI maxSize{};
    };

    void PumpEvents() noexcept;
}
