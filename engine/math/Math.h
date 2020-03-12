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

#include "Core/Preprocessor.h"
#include <stdlib.h>
#include <math.h>
#include <limits>
#include <type_traits>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4244) // Conversion from 'double' to 'float'
#pragma warning(disable:4702) // unreachable code
#endif

namespace alimer
{
    static constexpr float M_PI = 3.14159265358979323846264338327950288f;
    static constexpr float M_EPSILON = 0.000001f;
    static constexpr float M_DEGTORAD = M_PI / 180.0f;
    static constexpr float M_DEGTORAD_2 = M_PI / 360.0f;    // M_DEGTORAD / 2.0f
    static constexpr float M_RADTODEG = 1.0f / M_DEGTORAD;

    /// Check whether two floating point values are equal within accuracy.
    template <typename T>
    inline bool Equals(T lhs, T rhs, T eps = M_EPSILON) { return lhs + eps >= rhs && lhs - eps <= rhs; }

    /// Linear interpolation between two values.
    template <typename T, typename U>
    inline T Lerp(T lhs, T rhs, U t) { return lhs * (1.0 - t) + rhs * t; }

    /// Inverse linear interpolation between two values.
    template <typename T>
    inline T InverseLerp(T lhs, T rhs, T x) { return (x - lhs) / (rhs - lhs); }

    /// Return the smaller of two values.
    template <typename T, typename U>
    inline T Min(T lhs, U rhs) { return lhs < rhs ? lhs : rhs; }

    /// Return the larger of two values.
    template <typename T, typename U>
    inline T Max(T lhs, U rhs) { return lhs > rhs ? lhs : rhs; }

    /// Return absolute value of a value
    template <typename T>
    inline T Abs(T value) { return value >= 0.0 ? value : -value; }

    /// Return the sign of a float (-1, 0 or 1.)
    template <typename T>
    inline T Sign(T value) { return value > 0.0 ? 1.0 : (value < 0.0 ? -1.0 : 0.0); }

    /// Convert degrees to radians.
    template <typename T>
    inline T ToRadians(const T degrees) { return M_DEGTORAD * degrees; }

    /// Convert radians to degrees.
    template <typename T>
    inline T ToDegrees(const T radians) { return M_RADTODEG * radians; }

    /// Return a representation of the specified floating-point value as a single format bit layout.
    inline unsigned FloatToRawIntBits(float value)
    {
        unsigned u = *((unsigned*)&value);
        return u;
    }

    /// Check whether a floating point value is NaN.
    template <typename T> inline bool IsNaN(T value) { return isnan(value); }

    /// Check whether a floating point value is positive or negative infinity
    template <typename T> inline bool IsInf(T value) { return isinf(value); }

    /// Clamp a number to a range.
    template <typename T> inline T Clamp(T value, T min, T max)
    {
        if (value < min)
            return min;
        else if (value > max)
            return max;
        else
            return value;
    }

    /// Smoothly damp between values.
    template <typename T> inline T SmoothStep(T lhs, T rhs, T t)
    {
        t = Clamp((t - lhs) / (rhs - lhs), T(0.0), T(1.0)); // Saturate t
        return t * t * (3.0 - 2.0 * t);
    }

    template <typename T> inline T AlignUpWithMask(T value, size_t mask)
    {
        return (T)(((size_t)value + mask) & ~mask);
    }

    template <typename T> inline T AlignDownWithMask(T value, size_t mask)
    {
        return (T)((size_t)value & ~mask);
    }

    template <typename T> inline T AlignUp(T value, size_t alignment)
    {
        return AlignUpWithMask(value, alignment - 1);
    }

    template <typename T> inline T AlignDown(T value, size_t alignment)
    {
        return AlignDownWithMask(value, alignment - 1);
    }

    template <typename T> inline bool IsAligned(T value, size_t alignment)
    {
        return 0 == ((size_t)value & (alignment - 1));
    }

    template <typename T> inline T DivideByMultiple(T value, size_t alignment)
    {
        return (T)((value + alignment - 1) / alignment);
    }

    template <typename T> inline bool IsPowerOfTwo(T value)
    {
        return 0 == (value & (value - 1));
    }

    template <typename T> inline bool IsDivisible(T value, T divisor)
    {
        return (value / divisor) * divisor == value;
    }

} 

#ifdef _MSC_VER
#pragma warning(pop)
#endif
