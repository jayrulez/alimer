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

#include "Platform/WindowHandle.h"
#include "Core/Object.h"
#include "Math/Size.h"

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
    };
    ALIMER_DEFINE_ENUM_FLAG_OPERATORS(WindowFlags, uint32_t);

    class WindowImpl;

    class ALIMER_API Window : public Object
    {
        ALIMER_OBJECT(Window, Object);

    public:
        constexpr static const int Centered = std::numeric_limits<int32_t>::max();

        /// Constructor.
        Window(const std::string& title, int32_t x = Centered, int32_t y = Centered, uint32_t width = 1280u, uint32_t height = 720u, WindowFlags flags = WindowFlags::None);

        /// Destructor.
        ~Window();

        bool IsOpen() const noexcept;
        uint32_t GetId() const noexcept;

        /// Get the native window handle.
        WindowHandle GetHandle() const;

        void SetBrightness(float value);
        float GetBrightness() const noexcept;

        void SetSize(int32_t width, int32_t height) noexcept;
        void SetSize(const SizeI& size) noexcept;
        SizeI GetSize() const noexcept;

        void SetMaximumSize(int32_t width, int32_t height) noexcept;
        void SetMaximumSize(const SizeI& size) noexcept;
        SizeI GetMaximumSize() const noexcept;

        void SetMinimumSize(int32_t width, int32_t height) noexcept;
        void SetMinimumSize(const SizeI& size) noexcept;
        SizeI GetMinimumSize() const noexcept;

    private:
        WindowImpl* impl;
    };
}
