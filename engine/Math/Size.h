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
        explicit constexpr TSize2(const TSize2<U>& u)
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

    struct Extent3D
    {
    public:
        uint32_t width;
        uint32_t height;
        uint32_t depth;

        Extent3D() noexcept : width(0), height(0), depth(0) {}

        constexpr Extent3D(uint32_t width, uint32_t height, uint32_t depth) noexcept
            : width{ width }, height{ height }, depth{ depth }
        {
        }

        Extent3D(const Extent3D&) = default;
        Extent3D& operator=(const Extent3D&) = default;

        Extent3D(Extent3D&&) = default;
        Extent3D& operator=(Extent3D&&) = default;

        // Comparison operators
        bool operator == (const Extent3D& rhs) const noexcept { return (width == rhs.width) && (height == rhs.height) && (depth == rhs.depth); }
        bool operator != (const Extent3D& rhs) const noexcept { return (width != rhs.width) || (height != rhs.height) || (depth != rhs.depth); }
    };

    using Size = TSize2<int32_t>;
    using SizeU = TSize2<uint32_t>;
    using SizeF = TSize2<float>;
}
