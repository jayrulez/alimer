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

#include "Core/Utils.h"
#include "math/vec2.h"
#include "math/size.h"
#include <string>

namespace alimer
{
    enum class WindowStyle : uint32_t
    {
        None = 0,
        Fullscreen = 1 << 0,
        FullscreenDesktop = 1 << 1,
        Hidden = 1 << 2,
        Borderless = 1 << 3,
        Resizable = 1 << 4,
        Minimized = 1 << 5,
        Maximized = 1 << 6,
    };
    ALIMER_DEFINE_ENUM_BITWISE_OPERATORS(WindowStyle);

    constexpr static const int32_t centered = std::numeric_limits<int32_t>::max();
    using native_handle = void*;
    using native_display = void*;

    class WindowImpl;

    /// Defines an OS Window.
    class ALIMER_API Window final
    {
    public:
        /// Constructor.
        Window() = default;

        /// Constructor.
        Window(const std::string& title, const usize& newSize, WindowStyle style);

        /// Destructor.
        ~Window();

        /// Create the window.
        bool create(const std::string& title, const point& pos, const usize& size, WindowStyle style);

        /// Close the window.
        void close();

        native_handle get_native_handle() const;
        native_display get_native_display() const;

        /// Return the window id.
        auto get_id() const noexcept->uint32_t;

        /// Return whether or not the window is open.
        auto is_open() const noexcept -> bool;

        /// Return whether the window is minimized.
        auto is_minimized() const noexcept -> bool;

        /// Return whether the window is maximized.
        auto is_maximized() const noexcept -> bool;

        /// Get the window size.
        auto get_size() const noexcept->usize;

        /// Return the window title.
        auto get_title() const noexcept->std::string;

        /// Set the window title.
        void set_title(const std::string& title) noexcept;

    private:
        WindowImpl* impl = nullptr;

        ALIMER_DISABLE_COPY_MOVE(Window);
    };
}
