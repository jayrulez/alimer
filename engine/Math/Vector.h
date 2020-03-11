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
#include "Diagnostics/Assert.h"
#include <EASTL/string.h>

namespace alimer
{
    /// Class specifying a two-dimensional Vector.
    class ALIMER_API Vector2
    {
    public:
        /// Specifies the x-component of the vector.
        float x;
        /// Specifies the y-component of the vector.
        float y;

        /// Constructor.
        Vector2() noexcept : x(0.0f), y(0.0f) {}

        constexpr explicit Vector2(float value) noexcept : x(value), y(value) {}
        constexpr Vector2(float x_, float y_) noexcept : x(x_), y(y_) {}
        explicit Vector2(const float* pArray) noexcept : x(pArray[0]), y(pArray[1]) {}

        Vector2(const Vector2&) = default;
        Vector2& operator=(const Vector2&) = default;
        Vector2(Vector2&&) = default;
        Vector2& operator=(Vector2&&) = default;

        /// Test for equality with another vector without epsilon.
        bool operator ==(const Vector2& rhs) const { return x == rhs.x && y == rhs.y; }

        /// Test for inequality with another vector without epsilon.
        bool operator !=(const Vector2& rhs) const { return x != rhs.x || y != rhs.y; }

        /// Add a vector.
        Vector2 operator +(const Vector2& rhs) const { return Vector2(x + rhs.x, y + rhs.y); }

        /// Return negation.
        Vector2 operator -() const { return Vector2(-x, -y); }

        /// Subtract a vector.
        Vector2 operator -(const Vector2& rhs) const { return Vector2(x - rhs.x, y - rhs.y); }

        /// Multiply with a scalar.
        Vector2 operator *(float rhs) const { return Vector2(x * rhs, y * rhs); }

        /// Multiply with a vector.
        Vector2 operator *(const Vector2& rhs) const { return Vector2(x * rhs.x, y * rhs.y); }

        /// Divide by a scalar.
        Vector2 operator /(float rhs) const { return Vector2(x / rhs, y / rhs); }

        /// Divide by a vector.
        Vector2 operator /(const Vector2& rhs) const { return Vector2(x / rhs.x, y / rhs.y); }

        /// Add-assign a vector.
        Vector2& operator +=(const Vector2& rhs)
        {
            x += rhs.x;
            y += rhs.y;
            return *this;
        }

        /// Subtract-assign a vector.
        Vector2& operator -=(const Vector2& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            return *this;
        }

        /// Multiply-assign a scalar.
        Vector2& operator *=(float rhs)
        {
            x *= rhs;
            y *= rhs;
            return *this;
        }

        /// Multiply-assign a vector.
        Vector2& operator *=(const Vector2& rhs)
        {
            x *= rhs.x;
            y *= rhs.y;
            return *this;
        }

        /// Divide-assign a scalar.
        Vector2& operator /=(float rhs)
        {
            float invRhs = 1.0f / rhs;
            x *= invRhs;
            y *= invRhs;
            return *this;
        }

        /// Divide-assign a vector.
        Vector2& operator /=(const Vector2& rhs)
        {
            x /= rhs.x;
            y /= rhs.y;
            return *this;
        }

        /// Normalize to unit length.
        void Normalize()
        {
            float lenSquared = LengthSquared();
            if (!alimer::Equals(lenSquared, 1.0f) && lenSquared > 0.0f)
            {
                float invLen = 1.0f / sqrtf(lenSquared);
                x *= invLen;
                y *= invLen;
            }
        }

        void Normalize(Vector2* result) const noexcept;

        /// Return length.
        float Length() const { return sqrtf(x * x + y * y); }

        /// Return squared length.
        float LengthSquared() const { return x * x + y * y; }

        /// Calculate dot product.
        float Dot(const Vector2& rhs) const noexcept { return x * rhs.x + y * rhs.y; }
        static float Cross(const Vector2& lhs, const Vector2& rhs);

        /// Test for equality with another vector with epsilon.
        bool Equals(const Vector2& rhs, float eps = M_EPSILON) const { return alimer::Equals(x, rhs.x, eps) && alimer::Equals(y, rhs.y, eps); }

        /// Return float data.
        const float* Data() const { return &x; }

        /// Return as string.
        eastl::string ToString() const;

        // Constants
        static const Vector2 Zero;
        static const Vector2 One;
        static const Vector2 UnitX;
        static const Vector2 UnitY;
    };

    /// Class specifying a three-dimensional Vector.
    template <typename T>
    class tvec3 final
    {
    public:
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

        /// Return float data.
        const T* Data() const { return &x; }

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

    /// Class specifying a four-dimensional Vector.
    class ALIMER_API Vector4
    {
    public:
        /// Specifies the x-component of the vector.
        float x;
        /// Specifies the y-component of the vector.
        float y;
        /// Specifies the z-component of the vector.
        float z;
        /// Specifies the w-component of the vector.
        float w;

        /// Constructor.
        Vector4() noexcept
            : x(0.0f)
            , y(0.0f)
            , z(0.0f)
            , w(0.0f)
        {

        }

        /// Return float data.
        const float* Data() const { return &x; }

        /// Return as string.
        eastl::string ToString() const;
    };

    using float3 = tvec3<float>;
    using dvec3 = tvec3<double>;
}
