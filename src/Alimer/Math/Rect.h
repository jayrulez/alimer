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
#include <EASTL/string.h>

namespace Alimer
{
    /// Class specifying a two-dimensional floating-point rectangle.
    class ALIMER_API Rect
    {
    public:
        /// Specifies the x-coordinate of the rectangle.
        float x;
        /// Specifies the y-coordinate of the rectangle.
        float y;
        /// Specifies the width of the rectangle.
        float width;
        /// Specifies the height of the rectangle.
        float height;

        /// Constructor.
        Rect() noexcept : x(0.0f), y(0.0f), width(0.0f), height(0.0f) {}
        constexpr Rect(float x_, float y_, float width_, float height_) noexcept : x(x_), y(y_), width(width_), height(height_) {}

        Rect(const Rect&) = default;
        Rect& operator=(const Rect&) = default;

        Rect(Rect&&) = default;
        Rect& operator=(Rect&&) = default;
    };

    /// Class specifying a two-dimensional integer rectangle.
    class ALIMER_API RectI
    {
    public:
        /// Specifies the x-coordinate of the rectangle.
        int32_t x;
        /// Specifies the y-coordinate of the rectangle.
        int32_t y;
        /// Specifies the width of the rectangle.
        int32_t width;
        /// Specifies the height of the rectangle.
        int32_t height;

        /// Constructor.
        RectI() noexcept : x(0), y(0), width(0), height(0) {}
        constexpr RectI(int32_t x_, int32_t y_, int32_t width_, int32_t height_) noexcept : x(x_), y(y_), width(width_), height(height_) {}

        RectI(const RectI&) = default;
        RectI& operator=(const RectI&) = default;

        RectI(RectI&&) = default;
        RectI& operator=(RectI&&) = default;
    };
} 
