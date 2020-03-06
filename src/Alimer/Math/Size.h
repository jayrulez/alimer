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
#include <EASTL/string.h>

namespace Alimer
{
    /// Class specifying a two-dimensional size.
    template <typename T>
    class TSize final
    {
    public:
        static constexpr size_t SIZE = 2;

        TSize() = default;
        TSize(const TSize&) = default;

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

        union
        {
            T data[2];
            struct
            {
                T width, height;
            };
        };

        // array access
        inline constexpr T const& operator[](size_t i) const noexcept {
            ALIMER_ASSERT(i < SIZE);
            return v[i];
        }

        inline constexpr T& operator[](size_t i) noexcept {
            ALIMER_ASSERT(i < SIZE);
            return v[i];
        }
    };
    using Size = TSize<float>;
    using SizeU = TSize<uint32_t>;
} 
