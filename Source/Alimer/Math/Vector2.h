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
#include "Core/String.h"

namespace alimer
{
    /// 2D Vector; 32 bit floating point components
    struct ALIMER_API Vector2
    {
        /// Specifies the x-component of the vector.
        float x;
        /// Specifies the y-component of the vector.
        float y;

        /// Constructor.
        Vector2() noexcept : x(0.0f), y(0.0f) {}
        constexpr explicit Vector2(float value) noexcept : x(value), y(value) {}
        constexpr Vector2(float x_, float y_) noexcept : x(x_), y(y_) {}
        explicit Vector2(_In_reads_(2) const float* data) noexcept : x(data[0]), y(data[1]) {}

        Vector2(const Vector2&) = default;
        Vector2& operator=(const Vector2&) = default;
        Vector2(Vector2&&) = default;
        Vector2& operator=(Vector2&&) = default;

        // Comparison operators
        bool operator == (const Vector2& rhs) const noexcept { return x == rhs.x && y == rhs.y; }
        bool operator != (const Vector2& rhs) const noexcept { return x != rhs.x || y != rhs.y; }

        /// Return float data.
        const float* Data() const { return &x; }

        /// Return as string.
        String ToString() const;

        // Constants
        static const Vector2 Zero;
        static const Vector2 One;
        static const Vector2 UnitX;
        static const Vector2 UnitY;
    };

    // 2D Vector; 32 bit unsigned integer components
    struct UInt2
    {
        uint32_t x;
        uint32_t y;

        UInt2() noexcept : x(0), y(0) {}
        constexpr UInt2(uint32_t x_, uint32_t y_) noexcept : x(x_), y(y_) {}
        explicit UInt2(_In_reads_(2) const uint32_t* data) noexcept : x(data[0]), y(data[1]) {}

        UInt2(const UInt2&) = default;
        UInt2& operator=(const UInt2&) = default;
        UInt2(UInt2&&) = default;
        UInt2& operator=(UInt2&&) = default;

        // Constants
        static const UInt2 Zero;
        static const UInt2 One;
        static const UInt2 UnitX;
        static const UInt2 UnitY;
    };
}
