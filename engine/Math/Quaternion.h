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

#include "Core/Math.h"

namespace Alimer
{
    /// Class specifying a four-dimensional quaternion.
    struct ALIMER_API Quaternion
    {
        /// Specifies the x-component of the quaternion.
        float x;
        /// Specifies the y-component of the quaternion.
        float y;
        /// Specifies the z-component of the quaternion.
        float z;
        /// Specifies the w-component of the quaternion.
        float w;

        /// Constructor.
        Quaternion() noexcept : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
        constexpr Quaternion(float x_, float y_, float z_, float w_) noexcept : x(x_), y(y_), z(z_), w(w_) {}
        explicit Quaternion(_In_reads_(4) const float* data) noexcept : x(data[0]), y(data[1]), z(data[2]), w(data[3]) {}

        Quaternion(const Quaternion&) = default;
        Quaternion& operator=(const Quaternion&) = default;

        Quaternion(Quaternion&&) = default;
        Quaternion& operator=(Quaternion&&) = default;

        // Comparison operators
        bool operator == (const Quaternion& rhs) const noexcept { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }
        bool operator != (const Quaternion& rhs) const noexcept { return x != rhs.x || y != rhs.y || z != rhs.z || w != rhs.w; }

        /// Return float data.
        const float* Data() const { return &x; }

        /// Return as string.
        eastl::string ToString() const;

        // Constants
        static const Quaternion Zero;
        static const Quaternion Identity;
    };
}
