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

#include "math/Rect.h"

namespace alimer
{
    /// Class specifying the dimensions of a viewport.
    class ALIMER_API Viewport
    {
    public:
        /// Specifies the x-coordinate of the viewport.
        float x;
        /// Specifies the y-component of the viewport.
        float y;
        /// Specifies the width of the viewport.
        float width;
        /// Specifies the height of the viewport.
        float height;
        /// Specifies the minimum depth of the viewport. Ranges between 0 and 1.
        float minDepth;
        /// Specifies the maximum depth of the viewport. Ranges between 0 and 1.
        float maxDepth;

        Viewport() noexcept : x(0.0f), y(0.0f), width(0.0f), height(0.0f), minDepth(0.0f), maxDepth(1.0f) {}
        constexpr Viewport(float x_, float y_, float width_, float height_, float minDepth_ = 0.0f, float maxDepth_ = 1.0f) noexcept
            : x(x_), y(y_), width(width_), height(height_), minDepth(minDepth_), maxDepth(maxDepth_) {}
        explicit Viewport(const Rect& rect) noexcept
            : x(rect.x), y(rect.y), width(rect.width), height(rect.height), minDepth(0.0f), maxDepth(1.0f) {}

        explicit Viewport(const RectI& rect) noexcept
            : x(float(rect.x)), y(float(rect.y)), width(float(rect.width)), height(float(rect.height)), minDepth(0.0f), maxDepth(1.0f) {}

        Viewport(const Viewport&) = default;
        Viewport& operator=(const Viewport&) = default;

        Viewport(Viewport&&) = default;
        Viewport& operator=(Viewport&&) = default;

        // Assignment operators
        Viewport& operator= (const Rect& rect) noexcept
        {
            x = rect.x;
            y = rect.y;
            width = rect.width;
            height = rect.height;
            minDepth = 0.0f; maxDepth = 1.0f;
            return *this;
        }

        Viewport& operator= (const RectI& rect) noexcept
        {
            x = float(rect.x);
            y = float(rect.y);
            width = float(rect.width);
            height = float(rect.height);
            minDepth = 0.0f; maxDepth = 1.0f;
            return *this;
        }

        /// Test for equality with another vector without epsilon.
        bool operator ==(const Viewport& rhs) const {
            return x == rhs.x &&
                y == rhs.y &&
                width == rhs.width &&
                height == rhs.height &&
                minDepth == rhs.minDepth &&
                maxDepth == rhs.maxDepth; }

        /// Test for inequality with another vector without epsilon.
        bool operator !=(const Viewport& rhs) const {
            return x != rhs.x ||
                y != rhs.y ||
                width != rhs.width ||
                height != rhs.height ||
                minDepth != rhs.minDepth ||
                maxDepth != rhs.maxDepth;
        }

        // Viewport operations
        float AspectRatio() const noexcept;

        /// Return float data.
        const float* Data() const { return &x; }
    };
}
