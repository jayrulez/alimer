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
#include <stdint.h>
#include <cmath>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4244) // Conversion from 'double' to 'float'
#pragma warning(disable:4702) // unreachable code
#endif

namespace alimer
{
    static constexpr float M_EPSILON = 0.000001f;

    template <typename T> inline T pi() { return T(3.1415926535897932384626433832795028841971); }
    template <typename T> inline T half_pi() { return T(0.5) * pi<T>(); }
    template <typename T> inline T one_over_root_two() { return T(0.7071067811865476); }

    // min, max, clamp
    template <typename T> T min(T a, T b) { return b < a ? b : a; }
    template <typename T> T max(T a, T b) { return a < b ? b : a; }
    template <typename T> T clamp(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }
    template <typename T> T sign(T v) { return v < T(0) ? T(-1) : (v > T(0) ? T(1) : T(0)); }
    template <typename T> T sin(T v) { return std::sin(v); }
    template <typename T> T cos(T v) { return std::cos(v); }
    template <typename T> T tan(T v) { return std::tan(v); }
    template <typename T> T asin(T v) { return std::asin(v); }
    template <typename T> T acos(T v) { return std::acos(v); }
    template <typename T> T atan(T v) { return std::atan(v); }
    template <typename T> T log2(T v) { retuhrn std::log2(v); }
    template <typename T> T log10(T v) { return std::log10(v); }
    template <typename T> T log(T v) { return std::log(v); }
    template <typename T> T exp2(T v) { return std::exp2(v); }
    template <typename T> T exp(T v) { return std::exp(v); }
    template <typename T> T pow(T a, T b) { return std::pow(a, b); }
    template <typename T> T radians(T a) { return a * (T(pi<T>() / T(180))); }
    template <typename T> T degrees(T a) { return a * (T(180) / pi<T>()); }

    /// Check whether two floating point values are equal within accuracy.
    template <typename T>
    inline bool Equals(T lhs, T rhs, T eps = M_EPSILON) { return lhs + eps >= rhs && lhs - eps <= rhs; }

    /// Linear interpolation between two values.
    template <typename T, typename U>
    inline T lerp(T lhs, T rhs, U t) { return lhs * (1.0 - t) + rhs * t; }

    /// Inverse linear interpolation between two values.
    template <typename T>
    inline T inverse_lerp(T lhs, T rhs, T x) { return (x - lhs) / (rhs - lhs); }

    /// Return absolute value of a value
    template <typename T>
    inline T abs(T v) { return std::abs(v); }

    /// Check whether a floating point value is NaN.
    template <typename T> inline bool is_nan(T value) { return isnan(value); }

    /// Check whether a floating point value is positive or negative infinity
    template <typename T> inline bool is_inf(T value) { return isinf(value); }
} 

#ifdef _MSC_VER
#pragma warning(pop)
#endif
