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

#include "Diagnostics/Assert.h"

namespace alimer
{
    /// Class specifying a two-dimensional size.
    template <typename T>
    class TSize final
    {
    public:
        static constexpr size_t SIZE = 2;

        union
        {
            T data[SIZE];
            struct
            {
                T width, height;
            };
        };

        TSize() = default;
        TSize(const TSize&) = default;
        TSize& operator=(const TSize&) = default;
        TSize(TSize&&) = default;
        TSize& operator=(TSize&&) = default;

        explicit constexpr TSize(T v)
        {
            width = v;
            height = v;
        }

        template <typename U>
        explicit constexpr TSize(const TSize<U>& u)
        {
            width = T(u.width);
            height = T(u.height);
        }

        constexpr TSize(T width_, T height_)
        {
            width = width_;
            height = height_;
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

    /// Class specifying a three-dimensional size.
    template <typename T>
    class TSize3 final
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

        TSize3() = default;
        TSize3(const TSize3&) = default;
        TSize3& operator=(const TSize3&) = default;
        TSize3(TSizeTSize3) = default;
        TSize3& operator=(TSize3&&) = default;

        explicit constexpr TSize3(T v)
        {
            width = v;
            height = v;
            depth = v;
        }

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

    using Size = TSize<float>;
    using SizeI = TSize<int32_t>;
    using SizeU = TSize<uint32_t>;
    using Size3 = TSize3<float>;
    using Size3I = TSize3<int32_t>;
    using Size3U = TSize3<uint32_t>;
} 
