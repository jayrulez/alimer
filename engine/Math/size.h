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
    struct tsize2
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

        constexpr tsize2() = default;
        constexpr tsize2(const tsize2&) = default;

        template <typename U>
        explicit constexpr tsize2(const tsize2<U> & u)
        {
            width = T(u.width);
            height = T(u.height);
        }

        constexpr tsize2(T width_, T height_)
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
    struct tsize3
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

        constexpr tsize3() = default;
        constexpr tsize3(const tsize3&) = default;

        template <typename U>
        explicit constexpr tsize3(const tsize3<U>& u)
        {
            width = T(u.width);
            height = T(u.height);
            depth = T(u.depth);
        }

        constexpr tsize3(T width_, T height_, T depth_)
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

    using size = tsize2<int32_t>;
    using usize = tsize2<uint32_t>;

    using size3 = tsize3<int32_t>;
    using usize3 = tsize3<uint32_t>;
} 
