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

#include "Math/Math.h"
#include "Math/Size.h"

namespace alimer
{
    /// Class specifying a two-dimensional rectangle.
    template <typename T>
    class TRect final
    {
    public:
        static constexpr size_t SIZE = 4;

        union
        {
            T data[SIZE];
            struct
            {
                /// Specifies the x-coordinate of the rectangle.
                T x;
                /// Specifies the y-coordinate of the rectangle.
                T y;
                /// Specifies the width of the rectangle.
                T width;
                /// Specifies the height of the rectangle.
                T height;
            };
        };

        TRect() = default;
        TRect(const TRect&) = default;
        TRect& operator=(const TRect&) = default;

        TRect(TRect&&) = default;
        TRect& operator=(TRect&&) = default;

        explicit constexpr TRect(T v)
        {
            x = T(0);
            y = T(0);
            width = v;
            height = v;
        }

        template <typename U>
        explicit constexpr TRect(const TRect<U>& u)
        {
            x = T(u.x);
            y = T(u.y);
            width = T(u.width);
            height = T(u.height);
        }

        constexpr TRect(T x_, T y_, T width_, T height_)
        {
            x = x_;
            y = y_;
            width = width_;
            height = height_;
        }

        constexpr TRect(T width_, T height_)
        {
            x = T(0);
            y = T(0);
            width = width_;
            height = height_;
        }

        constexpr TRect(const TSize<T>& size)
        {
            x = T(0);
            y = T(0);
            width = size.width;
            height = size.height;
        }

        // array access
        inline constexpr T const& operator[](size_t i) const noexcept {
            ALIMER_ASSERT(i < SIZE);
            return data[i];
        }

        inline constexpr T& operator[](size_t i) noexcept {
            ALIMER_ASSERT(i < SIZE);
            return data[i];
        }
    };

    using Rect = TRect<float>;
    using RectI = TRect<int32_t>;
    using RectU = TRect<uint32_t>;
} 
