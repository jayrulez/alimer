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

struct GLFWwindow;

namespace Alimer
{
    enum class WindowFlags : uint32_t
    {
        None = 0,
        Fullscreen = 1 << 0,
        FullscreenDesktop = 1 << 1,
        Hidden = 1 << 2,
        Borderless = 1 << 3,
        Resizable = 1 << 4,
        Minimized = 1 << 5,
        Maximized = 1 << 6,
        HighDpi = 1 << 7,
        OpenGL = 1 << 8,
    };
    ALIMER_DEFINE_ENUM_FLAG_OPERATORS(WindowFlags, uint32_t);

    class GLFW_Window final : public Window
    {
    public:
        /// Constructor.
        GLFW_Window(const char* title, uint32 width, uint32 height, WindowFlags flags);

        /// Destructor.
        ~GLFW_Window() override;

        static void PollEvents();

        bool IsOpen() const override;

    private:
        GLFWwindow* window{ nullptr };
    };
} 
