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

#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable:4244) // Conversion from 'double' to 'float'
#   pragma warning(disable:4702) // unreachable code
#endif

namespace alimer
{
    static constexpr float M_EPSILON = 0.000001f;

    template <typename T> inline T pi() { return T(3.1415926535897932384626433832795028841971); }
    template <typename T> inline T half_pi() { return T(0.5) * pi<T>(); }

    template<typename T> inline T abs(T v) { return std::abs(v); }
    template<typename T> inline T min(T a, T b) { return (a < b) ? a : b; }
    template<typename T> inline T max(T a, T b) { return (a < b) ? b : a; }
    template<typename T> inline T clamp(T arg, T lo, T hi) { return (arg < lo) ? lo : (arg < hi) ? arg : hi; }
    template <typename T> T sign(T v) { return v < T(0) ? T(-1) : (v > T(0) ? T(1) : T(0)); }
    template <typename T> T sin(T v) { return std::sin(v); }
    template <typename T> T cos(T v) { return std::cos(v); }
    template <typename T> T tan(T v) { return std::tan(v); }
    template <typename T> T asin(T v) { return std::asin(v); }
    template <typename T> T acos(T v) { return std::acos(v); }
    template <typename T> T atan(T v) { return std::atan(v); }
    template <typename T> T log2(T v) { return std::log2(v); }
    template <typename T> T log10(T v) { return std::log10(v); }
    template <typename T> T log(T v) { return std::log(v); }
    template <typename T> T exp2(T v) { return std::exp2(v); }
    template <typename T> T exp(T v) { return std::exp(v); }
    template <typename T> T pow(T a, T b) { return std::pow(a, b); }
    template <typename T> T radians(T a) { return a * (T(pi<T>() / T(180))); }
    template <typename T> T degrees(T a) { return a * (T(180) / pi<T>()); }

    /// Check whether two floating point values are equal within accuracy.
    template <typename T> inline bool Equals(T lhs, T rhs, T eps = M_EPSILON) { return lhs + eps >= rhs && lhs - eps <= rhs; }

    /// Linear interpolation between two values.
    template <typename T, typename U> inline T lerp(T lhs, T rhs, U t) { return lhs * (1.0 - t) + rhs * t; }

    /// Inverse linear interpolation between two values.
    template <typename T> inline T inverse_lerp(T lhs, T rhs, T x) { return (x - lhs) / (rhs - lhs); }

    /// Check whether a floating point value is NaN.
    template <class T> inline bool is_naN(T value) { return std::isnan(value); }

    /// Check whether a floating point value is positive or negative infinity.
    template <class T> inline bool is_inf(T value) { return std::isinf(value); }


    template <typename T> inline bool IsPowerOfTwo(T value)
    {
        return 0 == (value & (value - 1));
    }

    inline uint32_t AlignTo(uint32_t value, uint32_t alignment)
    {
        ALIMER_ASSERT(alignment > 0);
        return ((value + alignment - 1) / alignment) * alignment;
    }

    inline uint64_t AlignTo(uint64_t value, uint64_t alignment)
    {
        ALIMER_ASSERT(alignment > 0);
        return ((value + alignment - 1) / alignment) * alignment;
    }

    template <typename T> struct tvec2;
    template <typename T> struct tvec3;
    template <typename T> struct tmat2;
    template <typename T> struct tmat3;
    template <typename T> struct tmat4;

    template<typename T>
    struct tvec2
    {
        static constexpr size_t SIZE = 2;

        union {
            T data[SIZE];
            struct { T x, y; };
            struct { T s, t; };
            struct { T r, g; };
        };

        tvec2() = default;
        tvec2(const tvec2&) = default;

        constexpr tvec2(T value) noexcept : x(value), y(value) {}

        template <typename U>
        constexpr tvec2(const tvec2<U>& u) noexcept : x(T(u.x)), y(T(u.y)) {}

        constexpr tvec2(T x_, T y_) noexcept : x(x_), y(y_) {}

        // array access
        inline constexpr T const& operator[](size_t i) const noexcept {
            ALIMER_ASSERT(i < SIZE);
            return data[i];
        }

        inline constexpr T& operator[](size_t i) noexcept {
            ALIMER_ASSERT(i < SIZE);
            return data[i];
        }

        inline tvec2 xx() const;
        inline tvec2 xy() const;
        inline tvec2 yx() const;
        inline tvec2 yy() const;
    };

    template <typename T>
    struct tvec3 final
    {
        static constexpr size_t SIZE = 3;

        union
        {
            T data[SIZE];
            struct
            {
                T x, y, z;
            };
        };

        tvec3() = default;
        tvec3(const tvec3&) = default;
        tvec3& operator=(const tvec3&) = default;
        tvec3(tvec3&&) = default;
        tvec3& operator=(tvec3&&) = default;

        constexpr explicit tvec3(T v) noexcept
        {
            x = v;
            y = v;
            z = v;
        }

        constexpr tvec3(T x_, T y_, T z_) noexcept
        {
            x = x_;
            y = y_;
            z = z_;
        }

        template <typename U>
        constexpr tvec3(const tvec3<U>& u) noexcept
        {
            x = T(u.x);
            y = T(u.y);
            z = T(u.z);
        }

        /*inline tvec3(const tvec2<T>& v, T z_) noexcept
        {
            x = v.x;
            y = v.y;
            z = z_;
        }*/

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
    
    template <typename T>
    struct tmat2
    {
        tmat2() = default;

        explicit inline tmat2(T v)
        {
            vec[0] = tvec2<T>(v, T(0));
            vec[1] = tvec2<T>(T(0), v);
        }

        inline tmat2(const tvec2<T>& a, const tvec2<T>& b)
        {
            vec[0] = a;
            vec[1] = b;
        }

        // array access
        inline constexpr tvec2<T> const& operator[](size_t i) const noexcept {
            return vec[i];
        }

        inline constexpr tvec2<T>& operator[](size_t i) noexcept {
            return vec[i];
        }

    private:
        tvec2<T> vec[2];
    };

    template <typename T>
    struct tmat3
    {
        tmat3() = default;

        explicit inline tmat3(T v)
        {
            vec[0] = tvec3<T>(v, T(0), T(0));
            vec[1] = tvec3<T>(T(0), v, T(0));
            vec[2] = tvec3<T>(T(0), T(0), v);
        }

        inline tmat3(const tvec3<T>& a, const tvec3<T>& b, const tvec3<T>& c)
        {
            vec[0] = a;
            vec[1] = b;
            vec[2] = c;
        }

        explicit inline tmat3(const tmat4<T>& m)
        {
            for (int col = 0; col < 3; col++)
                for (int row = 0; row < 3; row++)
                    vec[col][row] = m[col][row];
        }

        // array access
        inline constexpr tvec3<T> const& operator[](size_t i) const noexcept {
            return vec[i];
        }

        inline constexpr tvec3<T>& operator[](size_t i) noexcept {
            return vec[i];
        }

    private:
        tvec3<T> vec[3];
    };

    using uint = uint32_t;
    using float2 = tvec2<float>;
    using float3 = tvec3<float>;
    using float2x2 = tmat2<float>;
    using float3x3 = tmat3<float>;
    using float4x4 = tmat4<float>;

    using double2 = tvec2<double>;
    using double3 = tvec3<double>;
    using double2x2 = tmat2<double>;
    using double3x3 = tmat3<double>;
    using double4x4 = tmat4<double>;

    using int2 = tvec2<int32_t>;
    using int3 = tvec3<int32_t>;
    using uint3 = tvec3<uint32_t>;

    using ushort2 = tvec2<uint16_t>;
    using ushort3 = tvec3<uint16_t>;
    using short3 = tvec3<int16_t>;

    using ubyte2 = tvec2<uint8_t>;
    using ubyte3 = tvec3<uint8_t>;
    using byte2 = tvec2<int8_t>;
    using byte3 = tvec3<int8_t>;

    using bool2 = tvec2<bool>;
    using bool3 = tvec3<bool>;

    // select
    template <typename T> inline T select(T a, T b, bool lerp) {
        return lerp ? b : a;
    }

    template <typename T> inline tvec2<T> select(const tvec2<T>& a, const tvec2<T>& b, const tvec2<bool>& lerp) {
        return tvec2<T>(lerp.x ? b.x : a.x, lerp.y ? b.y : a.y);
    }

    template <typename T> inline tvec3<T> select(const tvec3<T>& a, const tvec3<T>& b, const tvec3<bool>& lerp) {
        return tvec3<T>(lerp.x ? b.x : a.x, lerp.y ? b.y : a.y, lerp.z ? b.z : a.z);
    }

    // smoothstep
    template <typename T> inline T smoothstep(const T& lo, const T& hi, T val) {
        val = clamp((val - lo) / (hi - lo), T(0.0f), T(1.0f));
        return val * val * (3.0f - 2.0f * val);
    }

    /* Implementation */
#define MATH_IMPL_SWIZZLE(ret_type, self_type, swiz, ...) template <typename T> t##ret_type<T> t##self_type<T>::swiz() const { return t##ret_type<T>(__VA_ARGS__); }

    // vec2
    MATH_IMPL_SWIZZLE(vec2, vec2, xx, x, x);
    MATH_IMPL_SWIZZLE(vec2, vec2, xy, x, y);
    MATH_IMPL_SWIZZLE(vec2, vec2, yx, y, x);
    MATH_IMPL_SWIZZLE(vec2, vec2, yy, y, y);

#undef MATH_IMPL_SWIZZLE
}

#ifdef _MSC_VER
#   pragma warning(pop)
#endif
