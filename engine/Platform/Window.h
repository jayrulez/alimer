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

#include "Core/Object.h"
#include "Math/Extent.h"
#include "Platform/WindowHandle.h"

namespace alimer
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
        Window(const String& title, int32_t x = Centered, int32_t y = Centered, uint32_t width = 1280u,
               uint32_t height = 720u, WindowFlags flags = WindowFlags::None);

        /// Destructor.
        virtual ~Window() = default;

        bool IsOpen() const noexcept;

        uint32_t GetId() const noexcept;

        /// Get the native window handle.
        WindowHandle GetHandle() const;

        void SetBrightness(float value);
        float GetBrightness() const noexcept;

        void SetSize(uint32_t width, uint32_t height) noexcept;
        void SetSize(const Extent2D& size) noexcept;
        Extent2D GetSize() const noexcept;

        void SetMaximumSize(uint32_t width, uint32_t height) noexcept;
        void SetMaximumSize(const Extent2D& size) noexcept;
        Extent2D GetMaximumSize() const noexcept;

        void SetMinimumSize(uint32_t width, uint32_t height) noexcept;
        void SetMinimumSize(const Extent2D& size) noexcept;
        Extent2D GetMinimumSize() const noexcept;

        std::string GetTitle() const noexcept;
        void SetTitle(const std::string& str) noexcept;

    private:
        std::unique_ptr<WindowImpl> impl;
    };
}
