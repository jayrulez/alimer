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
    struct TSize2
    {
    public:
        static constexpr size_t SIZE = 2;

        union
        {
            T data[2];
            struct
            {
                T width, height;
            };
        };

        constexpr TSize2() = default;
        constexpr TSize2(const TSize2&) = default;

        template <typename U>
        explicit constexpr TSize2(const TSize2<U> & u)
        {
            width = T(u.width);
            height = T(u.height);
        }

        constexpr TSize2(T width_, T height_)
        {
            width = width_;
            height = height_;
        }

        inline constexpr T const& operator[](size_t i) const noexcept {
            assert(i < SIZE);
            return data[i];
        }

        inline constexpr T& operator[](size_t i) noexcept {
            assert(i < SIZE);
            return data[i];
        }
    };

    template <typename T>
    struct TSize3
    {
    public:
        static constexpr size_t SIZE = 3;

        union
        {
            T data[SIZE];
            struct
            {
                T width, height, depth;
            };
        };

        constexpr TSize3() = default;
        constexpr TSize3(const TSize3&) = default;

        template <typename U>
        explicit constexpr TSize3(const TSize3<U>& u)
        {
            width = T(u.width);
            height = T(u.height);
            depth = T(u.depth);
        }

        constexpr TSize3(T width_, T height_, T depth_)
        {
            width = width_;
            height = height_;
            depth = depth_;
        }

        inline constexpr T const& operator[](size_t i) const noexcept {
            assert(i < SIZE);
            return data[i];
        }

        inline constexpr T& operator[](size_t i) noexcept {
            assert(i < SIZE);
            return data[i];
        }
    };

    using Size = TSize2<int32_t>;
    using SizeU = TSize2<uint32_t>;
    using SizeF = TSize2<float>;

    using Size3 = TSize3<int32_t>;
    using Size3U = TSize3<uint32_t>;
    using Size3F = TSize3<float>;
} 
