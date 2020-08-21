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

namespace Alimer
{
    /// A 3D rectangular region for the viewport clipping.
    class ALIMER_API Viewport
    {
    public:
        /// The x coordinate of the upper-left corner of the viewport.
        float x;
        /// The y coordinate of the upper-left corner of the viewport.
        float y;
        /// The width of the viewport, in pixels.
        float width;
        /// The width of the viewport, in pixels.
        float height;
        /// The z coordinate of the near clipping plane of the viewport.
        float minDepth;
        /// The z coordinate of the far clipping plane of the viewport.
        float maxDepth;

        /// Constructor.
        Viewport() noexcept : x(0.0f), y(0.0f), width(0.0f), height(0.0f), minDepth(0.0f), maxDepth(1.0f) {}
        constexpr Viewport(float x_, float y_, float width_, float ih, float iminz = 0.f, float imaxz = 1.f) noexcept
            : x(x_), y(y_), width(width_), height(ih), minDepth(iminz), maxDepth(imaxz) {}

        Viewport(const Viewport&) = default;
        Viewport& operator=(const Viewport&) = default;

        Viewport(Viewport&&) = default;
        Viewport& operator=(Viewport&&) = default;

        // Comparison operators
        bool operator == (const Viewport& rhs) const noexcept
        {
            return (x == rhs.x && y == rhs.y && width == rhs.width && height == rhs.height && minDepth == rhs.minDepth && maxDepth == rhs.maxDepth);
        }

        bool operator != (const Viewport& rhs) const noexcept
        {
            return (x != rhs.x || y != rhs.y || width != rhs.width || height != rhs.height || minDepth != rhs.minDepth || maxDepth != rhs.maxDepth);
        }

        // Viewport operations
        float AspectRatio() const;
    };
}
