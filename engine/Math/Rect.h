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
    /// Defines a floating-point rectangle.
    class ALIMER_API Rect final
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

        constexpr Rect() noexcept : x(0.0f), y(0.0f), width(0.0f), height(0.0f) {}
        constexpr Rect(float x_, float y_, float width_, float height_) noexcept : x(x_), y(y_), width(width_), height(height_) {}
        constexpr Rect(float width_, float height_) noexcept : x(0.0f), y(0.0f), width(width_), height(height_) {}
        constexpr Rect(const Size& size) noexcept : x(0.0f), y(0.0f), width(size.width), height(size.height) {}

        Rect(const Rect&) = default;
        Rect& operator=(const Rect&) = default;

        Rect(Rect&&) = default;
        Rect& operator=(Rect&&) = default;

        /// Test for equality with another rect with epsilon.
        bool Equals(const Rect& rhs, float eps = M_EPSILON) const {
            return alimer::Equals(x, rhs.x)
                && alimer::Equals(y, rhs.y)
                && alimer::Equals(width, rhs.width)
                && alimer::Equals(height, rhs.height);
        }

        /// Gets a value that indicates whether the rectangle is empty.
        bool IsEmpty() const;

        /// Return float data.
        const float* Data() const { return &x; }

        /// Return as string.
        String ToString() const;
    };

    /// Defines an integer rectangle.
    class ALIMER_API RectI final
    {
    public:
        /// Specifies the x-coordinate of the rectangle.
        int x;
        /// Specifies the y-coordinate of the rectangle.
        int y;
        /// Specifies the width of the rectangle.
        int width;
        /// Specifies the height of the rectangle.
        int height;

        constexpr RectI() noexcept : x(0), y(0), width(0), height(0) {}
        constexpr RectI(int x_, int y_, int width_, int height_) noexcept : x(x_), y(y_), width(width_), height(height_) {}
        constexpr RectI(int width_, int height_) noexcept : x(0), y(0), width(width_), height(height_) {}
        constexpr RectI(const SizeI& size) noexcept : x(0), y(0), width(size.width), height(size.height) {}

        RectI(const RectI&) = default;
        RectI& operator=(const RectI&) = default;

        RectI(RectI&&) = default;
        RectI& operator=(RectI&&) = default;

        /// Gets a value that indicates whether the rectangle is empty.
        bool IsEmpty() const;

        /// Return integer data.
        const int* Data() const { return &x; }

        /// Return as string.
        String ToString() const;
    };
} 
