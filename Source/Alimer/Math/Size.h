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

#include "Core/String.h"

namespace alimer
{
    template <typename T>
    struct ALIMER_API TSize
    {
    public:
        union
        {
            T data[2];
            struct
            {
                T width, height;
            };
        };

        constexpr TSize() = default;
        constexpr TSize(T value) noexcept : width(value), height(value) {}
        constexpr TSize(T width_, T height_) noexcept : width(width_), height(height_) {}

        TSize(const TSize&) = default;
        TSize& operator=(const TSize&) = default;

        TSize(TSize&&) = default;
        TSize& operator=(TSize&&) = default;

        /// Gets a value that indicates whether the size is empty.
        ALIMER_FORCE_INLINE bool IsEmpty() const
        {
            return (width == 0 && height == 0);
        }

        /// Return as string.
        ALIMER_FORCE_INLINE std::string ToString() const
        {
            return fmt::format("{} {}", width, height);
        }
    };

    using Size = TSize<float>;
    using USize = TSize<uint32>;
}
