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
#include <cmath>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4244) // Conversion from 'double' to 'float'
#pragma warning(disable:4702) // unreachable code
#endif

namespace alimer {
    static constexpr float M_EPSILON = 0.000001f;

    template <typename T> inline T pi() { return T(3.1415926535897932384626433832795028841971); }
    template <typename T> inline T half_pi() { return T(0.5) * pi<T>(); }
    template <typename T> inline T one_over_root_two() { return T(0.7071067811865476); }

    // min, max, clamp
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

    /// Check whether a floating point value is NaN.
    template <typename T> inline bool is_nan(T value) { return isnan(value); }

    /// Check whether a floating point value is positive or negative infinity
    template <typename T> inline bool is_inf(T value) { return isinf(value); }

    template <typename T> struct tvec2;
    template <typename T> struct tvec3;
    template <typename T> struct tvec4;
    template <typename T> struct tmat2;
    template <typename T> struct tmat3;
    template <typename T> struct tmat4;

    template <typename T>
    struct tvec2
    {
        static constexpr size_t SIZE = 2;

        union
        {
            T data[2];
            struct
            {
                T x, y;
            };
        };

        constexpr tvec2() = default;
        constexpr tvec2(const tvec2&) = default;

        template <typename U>
        explicit constexpr tvec2(const tvec2 <U>& u)
        {
            x = T(u.x);
            y = T(u.y);
        }

        constexpr tvec2(T x_, T y_)
        {
            x = x_;
            y = y_;
        }

        inline constexpr T const& operator[](size_t i) const noexcept {
            assert(i < SIZE);
            return v[i];
        }

        inline constexpr T& operator[](size_t i) noexcept {
            assert(i < SIZE);
            return v[i];
        }

        inline constexpr tvec2 xx() const { return tvec2(x, x); }
        inline constexpr tvec2 xy() const { return tvec2(x, y); }
        inline constexpr tvec2 yx() const { return tvec2(y, x); }
        inline constexpr tvec2 yy() const { return tvec2(y, y); }
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
    struct tvec4
    {
    public:
        static constexpr size_t SIZE = 4;

        union
        {
            T data[SIZE];
            struct
            {
                T x, y, z, w;
            };
        };

        tvec4() = default;
        tvec4(const tvec4&) = default;

        template <typename U>
        explicit inline tvec4(const tvec4<U>& u)
        {
            x = T(u.x);
            y = T(u.y);
            z = T(u.z);
            w = T(u.w);
        }

        inline tvec4(const tvec2<T>& a, const tvec2<T>& b)
        {
            x = a.x;
            y = a.y;
            z = b.x;
            w = b.y;
        }

        inline tvec4(const tvec3<T>& a, T b)
        {
            x = a.x;
            y = a.y;
            z = a.z;
            w = b;
        }

        inline tvec4(T a, const tvec3<T>& b)
        {
            x = a;
            y = b.x;
            z = b.y;
            w = b.z;
        }

        inline tvec4(const tvec2<T>& a, T b, T c)
        {
            x = a.x;
            y = a.y;
            z = b;
            w = c;
        }

        inline tvec4(T a, const tvec2<T>& b, T c)
        {
            x = a;
            y = b.x;
            z = b.y;
            w = c;
        }

        inline tvec4(T a, T b, const tvec2<T>& c)
        {
            x = a;
            y = b;
            z = c.x;
            w = c.y;
        }

        explicit inline tvec4(T v)
        {
            x = v;
            y = v;
            z = v;
            w = v;
        }

        inline tvec4(T x_, T y_, T z_, T w_)
        {
            x = x_;
            y = y_;
            z = z_;
            w = w_;
        }

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

    using uint = uint32_t;
    using float2 = tvec2<float>;
    using float3 = tvec3<float>;
    using float4 = tvec4<float>;
    using float2x2 = tmat2<float>;
    using float3x3 = tmat3<float>;
    using float4x4 = tmat4<float>;

    using double2 = tvec2<double>;
    using double3 = tvec3<double>;
    using double4 = tvec4<double>;
    using double2x2 = tmat2<double>;
    using double3x3 = tmat3<double>;
    using double4x4 = tmat4<double>;

    using int2 = tvec2<int32_t>;
    using int3 = tvec3<int32_t>;
    using int4 = tvec4<int32_t>;
    using uint2 = tvec2<uint32_t>;
    using uint3 = tvec3<uint32_t>;
    using uint4 = tvec4<uint32_t>;

    using ushort2 = tvec2<uint16_t>;
    using ushort3 = tvec3<uint16_t>;
    using ushort4 = tvec4<uint16_t>;
    using short2 = tvec2<int16_t>;
    using short3 = tvec3<int16_t>;
    using short4 = tvec4<int16_t>;

    //using half2 = tvec2<half>;
    using ubyte2 = tvec2<uint8_t>;
    using ubyte3 = tvec3<uint8_t>;
    using ubyte4 = tvec4<uint8_t>;
    using byte2 = tvec2<int8_t>;
    using byte3 = tvec3<int8_t>;
    using byte4 = tvec4<int8_t>;

    using bool2 = tvec2<bool>;
    using bool3 = tvec3<bool>;
    using bool4 = tvec4<bool>;
}  // namespace alimer

#ifdef _MSC_VER
#pragma warning(pop)
#endif
