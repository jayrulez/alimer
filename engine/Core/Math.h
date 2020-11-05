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
#include "Math/MathHelper.h"

namespace alimer
{
    /// 2D Vector; 32 bit floating point components
    struct ALIMER_API Float2
    {
        /// Specifies the x-component of the vector.
        float x;
        /// Specifies the y-component of the vector.
        float y;

        /// Constructor.
        Float2() noexcept
            : x(0.0f)
            , y(0.0f)
        {
        }

        constexpr explicit Float2(float value) noexcept
            : x(value)
            , y(value)
        {
        }

        constexpr Float2(float x_, float y_) noexcept
            : x(x_)
            , y(y_)
        {
        }

        explicit Float2(_In_reads_(2) const float* pArray) noexcept
            : x(pArray[0])
            , y(pArray[1])
        {
        }

        Float2(const Float2&) = default;
        Float2& operator=(const Float2&) = default;

        Float2(Float2&&) = default;
        Float2& operator=(Float2&&) = default;

        // Comparison operators
        bool operator==(const Float2& rhs) const noexcept { return x == rhs.x && y == rhs.y; }

        bool operator!=(const Float2& rhs) const noexcept { return x != rhs.x || y != rhs.y; }

        /// Return float data.
        const float* Data() const { return &x; }

        /// Return as string.
        std::string ToString() const;

        // Constants
        static const Float2 Zero;
        static const Float2 One;
        static const Float2 UnitX;
        static const Float2 UnitY;
    };

    /// 2D Vector; 32 bit signed integer components
    struct ALIMER_API Int2
    {
        /// Specifies the x-component of the vector.
        int32_t x;
        /// Specifies the y-component of the vector.
        int32_t y;

        Int2() = default;
        constexpr explicit Int2(int32_t value) noexcept
            : x(value)
            , y(value)
        {
        }
        constexpr Int2(int32_t _x, int32_t _y) noexcept
            : x(_x)
            , y(_y)
        {
        }
        explicit Int2(_In_reads_(2) const int32_t* pArray) noexcept
            : x(pArray[0])
            , y(pArray[1])
        {
        }

        Int2(const Int2&) = default;
        Int2& operator=(const Int2&) = default;

        Int2(Int2&&) = default;
        Int2& operator=(Int2&&) = default;

        // Constants
        static const Int2 Zero;
        static const Int2 One;
        static const Int2 UnitX;
        static const Int2 UnitY;
    };

    /// 2D Vector; 32 bit unsigned integer components
    struct ALIMER_API UInt2
    {
        /// Specifies the x-component of the vector.
        uint32_t x;
        /// Specifies the y-component of the vector.
        uint32_t y;

        UInt2() noexcept
            : x(0)
            , y(0)
        {
        }

        UInt2(const UInt2&) = default;
        UInt2& operator=(const UInt2&) = default;

        UInt2(UInt2&&) = default;
        UInt2& operator=(UInt2&&) = default;

        constexpr UInt2(uint32_t _x, uint32_t _y) noexcept
            : x(_x)
            , y(_y)
        {
        }
        explicit UInt2(_In_reads_(2) const uint32_t* pArray) noexcept
            : x(pArray[0])
            , y(pArray[1])
        {
        }

        static const UInt2 Zero;
        static const UInt2 One;
        static const UInt2 UnitX;
        static const UInt2 UnitY;
    };

    /// 3D Vector; 32 bit floating point components
    struct ALIMER_API Float3
    {
        /// Specifies the x-component of the vector.
        float x;
        /// Specifies the y-component of the vector.
        float y;
        /// Specifies the z-component of the vector.
        float z;

        /// Constructor.
        Float3() noexcept
            : x(0.0f)
            , y(0.0f)
            , z(0.0f)
        {
        }
        constexpr explicit Float3(float value) noexcept
            : x(value)
            , y(value)
            , z(value)
        {
        }
        constexpr Float3(float x_, float y_, float z_) noexcept
            : x(x_)
            , y(y_)
            , z(z_)
        {
        }
        explicit Float3(_In_reads_(3) const float* data) noexcept
            : x(data[0])
            , y(data[1])
            , z(data[2])
        {
        }
        constexpr Float3(const Float2& vector, float z_) noexcept
            : x(vector.x)
            , y(vector.y)
            , z(z_)
        {
        }

        Float3(const Float3&) = default;
        Float3& operator=(const Float3&) = default;

        Float3(Float3&&) = default;
        Float3& operator=(Float3&&) = default;

        // Comparison operators
        bool operator==(const Float3& rhs) const noexcept { return x == rhs.x && y == rhs.y && z == rhs.z; }
        bool operator!=(const Float3& rhs) const noexcept { return x != rhs.x || y != rhs.y || z != rhs.z; }

        /// Return float data.
        const float* Data() const { return &x; }

        /// Return as string.
        std::string ToString() const;

        // Constants
        static const Float3 Zero;
        static const Float3 One;
        static const Float3 UnitX;
        static const Float3 UnitY;
        static const Float3 UnitZ;
        static const Float3 Up;
        static const Float3 Down;
        static const Float3 Right;
        static const Float3 Left;
        static const Float3 Forward;
        static const Float3 Backward;
    };

    /// 4D Vector; 32 bit floating point components
    struct ALIMER_API Float4
    {
        /// Specifies the x-component of the vector.
        float x;
        /// Specifies the y-component of the vector.
        float y;
        /// Specifies the z-component of the vector.
        float z;
        /// Specifies the w-component of the vector.
        float w;

        /// Constructor.
        Float4() noexcept
            : x(0.0f)
            , y(0.0f)
            , z(0.0f)
            , w(0.0f)
        {
        }
        constexpr explicit Float4(float value) noexcept
            : x(value)
            , y(value)
            , z(value)
            , w(value)
        {
        }
        constexpr Float4(float x_, float y_, float z_, float w_) noexcept
            : x(x_)
            , y(y_)
            , z(z_)
            , w(w_)
        {
        }
        explicit Float4(_In_reads_(4) const float* data) noexcept
            : x(data[0])
            , y(data[1])
            , z(data[2])
            , w(data[3])
        {
        }
        constexpr Float4(const Float2& vector, float z_, float w_) noexcept
            : x(vector.x)
            , y(vector.y)
            , z(z_)
            , w(w_)
        {
        }
        constexpr Float4(const Float3& vector, float w_) noexcept
            : x(vector.x)
            , y(vector.y)
            , z(vector.z)
            , w(w_)
        {
        }

        Float4(const Float4&) = default;
        Float4& operator=(const Float4&) = default;

        Float4(Float4&&) = default;
        Float4& operator=(Float4&&) = default;

        // Comparison operators
        bool operator==(const Float4& rhs) const noexcept { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }
        bool operator!=(const Float4& rhs) const noexcept { return x != rhs.x || y != rhs.y || z != rhs.z || w != rhs.w; }

        /// Return float data.
        const float* Data() const { return &x; }

        /// Return as string.
        std::string ToString() const;

        // Constants
        static const Float4 Zero;
        static const Float4 One;
        static const Float4 UnitX;
        static const Float4 UnitY;
        static const Float4 UnitZ;
        static const Float4 UnitW;
    };
}
