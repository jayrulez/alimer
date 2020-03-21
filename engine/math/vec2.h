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

#include <stdint.h>
#include <assert.h>

namespace alimer
{
    template <typename T>
    struct tvec2
    {
    public:
        static constexpr size_t SIZE = 2;

        union
        {
            T data[2];
            struct
            {
                T x, y;
            };
        };

        constexpr tvec2() = default;
        constexpr tvec2(const tvec2&) = default;

        template <typename U>
        explicit constexpr tvec2(const tvec2 <U> & u)
        {
            x = T(u.x);
            y = T(u.y);
        }

        constexpr tvec2(T x_, T y_)
        {
            x = x_;
            y = y_;
        }

        inline constexpr T const& operator[](size_t i) const noexcept {
            assert(i < SIZE);
            return v[i];
        }

        inline constexpr T& operator[](size_t i) noexcept {
            assert(i < SIZE);
            return v[i];
        }

        inline constexpr tvec2 xx() const { return tvec2(x, x); }
        inline constexpr tvec2 xy() const { return tvec2(x, y); }
        inline constexpr tvec2 yx() const { return tvec2(y, x); }
        inline constexpr tvec2 yy() const { return tvec2(y, y); }
    };

    using point = tvec2<int32_t>;
    using double2 = tvec2<double>;
    using float2 = tvec2<float>;
    //using half2 = tvec2<half>;
    using int2 = tvec2<int32_t>;
    using uint2 = tvec2<uint32_t>;
    using short2 = tvec2<int16_t>;
    using ushort2 = tvec2<uint16_t>;
    using byte2 = tvec2<int8_t>;
    using ubyte2 = tvec2<uint8_t>;
    using bool2 = tvec2<bool>;
} 
