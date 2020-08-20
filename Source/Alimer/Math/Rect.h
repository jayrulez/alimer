//
// Copyright (c) 2019-2020 Amer Koleci and contributors.
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

#include "Math/MathHelper.h"
#include "Math/Size.h"

namespace alimer
{
    template <typename T>
    struct ALIMER_API TRect
    {
    public:
        union
        {
            T data[4];
            struct
            {
                T x, y, width, height;
            };
        };

        constexpr TRect() = default;
        constexpr TRect(T value) noexcept : x(value), y(value), width(value), height(value) {}
        constexpr TRect(T x_, T y_, T width_, T height_) noexcept : x(x_), y(y_), width(width_), height(height_) {}
        constexpr TRect(T width_, T height_) noexcept : x(T(0)), y(T(0)), width(width_), height(height_) {}

        TRect(const TRect&) = default;
        TRect& operator=(const TRect&) = default;

        TRect(TRect&&) = default;
        TRect& operator=(TRect&&) = default;

        T Left() const { return x; }
        T Right() const { return x + width; }
        T Top() const { return y; }
        T Bottom() const { return y + height; }

        /// Gets a value that indicates whether the rectangle is empty.
        ALIMER_FORCE_INLINE bool IsEmpty() const
        {
            return (x == 0 && y == 0 && width == 0 && height == 0);
        }

        /// Return as string.
        ALIMER_FORCE_INLINE std::string ToString() const
        {
            return fmt::format("{} {} {} {}", x, y, width, height);
        }
    };

    using Rect = TRect<float>;
    using URect = TRect<uint32>;
}
