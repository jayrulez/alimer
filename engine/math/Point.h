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
#include <string>

namespace alimer
{
    /// Class specifying a two-dimensional point.
    class ALIMER_API Point2D
    {
    public:
        /// Specifies the x-coordinate of the point.
        int32_t x;
        /// Specifies the y-coordinate of the point.
        int32_t y;

        /// Constructor.
        Point2D() noexcept : x(0), y(0) {}
    };

    /// Class specifying a three-dimensional point.
    class ALIMER_API Point3D
    {
    public:
        /// Specifies the x-coordinate of the point.
        int32_t x;
        /// Specifies the y-coordinate of the point.
        int32_t y;
        /// Specifies the z-coordinate of the point.
        int32_t z;

        /// Constructor.
        Point3D() noexcept : x(0), y(0), z(0) {}
    };
} 
