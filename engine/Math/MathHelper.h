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

#include "Core/Assert.h"
#include <cmath>
#include <cstdlib>
#include <limits>

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4244) // Conversion from 'double' to 'float'
    #pragma warning(disable : 4702) // unreachable code
#endif

namespace alimer
{
    static constexpr float Pi = 3.1415926535897932f;
    static constexpr float TwoPi = 6.28318530718f;
    static constexpr float PiOver2 = 1.57079632679f;
    static constexpr float PiOver4 = 0.78539816339f;
    static constexpr float HalfPi = PiOver2;

    /// Check whether two floating point values are equal within accuracy.
    template <typename T> inline bool Equals(T lhs, T rhs, T eps) { return lhs + eps >= rhs && lhs - eps <= rhs; }

    template <class T> inline bool Equals(T lhs, T rhs)
    {
        return lhs + std::numeric_limits<T>::epsilon() >= rhs && lhs - std::numeric_limits<T>::epsilon() <= rhs;
    }

    template <typename T> inline T Sign(T v) { return v < T(0) ? T(-1) : (v > T(0) ? T(1) : T(0)); }

    template <typename T> inline T sin(T v) { return std::sin(v); }
    template <typename T> inline T cos(T v) { return std::cos(v); }

    /// Return tangent of an angle in degrees.
    template <class T> inline T Tan(T angle) { return std::tan(angle); }

    template <typename T> inline T asin(T v) { return std::asin(v); }
    template <typename T> inline T acos(T v) { return std::acos(v); }
    template <typename T> inline T atan(T v) { return std::atan(v); }
    template <typename T> inline T log2(T v) { return std::log2(v); }
    template <typename T> inline T log10(T v) { return std::log10(v); }
    template <typename T> inline T log(T v) { return std::log(v); }
    template <typename T> inline T exp2(T v) { return std::exp2(v); }
    template <typename T> inline T exp(T v) { return std::exp(v); }
    template <typename T> inline T pow(T a, T b) { return std::pow(a, b); }
    template <typename T> inline T ToRadians(T degrees) { return degrees * (Pi / 180.0f); }
    template <typename T> inline T ToDegrees(T radians) { return radians * (180.0f / Pi); }

    /// Linear interpolation between two values.
    template <typename T, typename U> inline T lerp(T lhs, T rhs, U t) { return lhs * (1.0 - t) + rhs * t; }

    /// Inverse linear interpolation between two values.
    template <typename T> inline T inverse_lerp(T lhs, T rhs, T x) { return (x - lhs) / (rhs - lhs); }

    /// Check whether a floating point value is NaN.
    template <class T> inline bool IsNaN(T value) { return std::isnan(value); }

    /// Check whether a floating point value is positive or negative infinity.
    template <class T> inline bool IsInf(T value) { return std::isinf(value); }

    template <typename T> constexpr bool IsPowerOfTwo(T value) { return !(value & (value - 1)) && value; }

    /// Round up to next power of two.
    constexpr uint32_t NextPowerOfTwo(uint32_t value)
    {
        // http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
        --value;
        value |= value >> 1u;
        value |= value >> 2u;
        value |= value >> 4u;
        value |= value >> 8u;
        value |= value >> 16u;
        return ++value;
    }

    /// Round up or down to the closest power of two.
    inline uint32_t ClosestPowerOfTwo(uint32_t value)
    {
        const uint32_t next = NextPowerOfTwo(value);
        const uint32_t prev = next >> 1u;
        return (value - prev) > (next - value) ? next : prev;
    }

    constexpr uint32_t AlignTo(uint32_t value, uint32_t alignment)
    {
        ALIMER_ASSERT(alignment > 0);
        return ((value + alignment - 1) / alignment) * alignment;
    }

    constexpr uint64_t AlignTo(uint64_t value, uint64_t alignment)
    {
        ALIMER_ASSERT(alignment > 0);
        return ((value + alignment - 1) / alignment) * alignment;
    }

    /// Return a representation of the specified floating-point value as a single format bit layout.
    constexpr unsigned FloatToRawIntBits(float value)
    {
        unsigned u = *((unsigned*)&value);
        return u;
    }
}

#ifdef _MSC_VER
    #pragma warning(pop)
#endif
