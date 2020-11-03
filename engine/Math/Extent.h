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

namespace alimer
{
    struct Extent2D
    {
        /// Specifies the width of the extent.
        uint32_t width;

        /// Specifies the height of the extent.
        uint32_t height;

        constexpr Extent2D() : width(0), height(0) {}
        constexpr Extent2D(uint32_t value) noexcept : width(value), height(value) {}
        constexpr Extent2D(uint32_t width_, uint32_t height_) noexcept : width(width_), height(height_) {}

        Extent2D(const Extent2D&) = default;
        Extent2D& operator=(const Extent2D&) = default;

        Extent2D(Extent2D&&) = default;
        Extent2D& operator=(Extent2D&&) = default;

        bool operator!=(const Extent2D& other) const
        {
            return width != other.width || height != other.height;
        }

        bool operator==(const Extent2D& other) const
        {
            return width == other.width && height == other.height;
        }

        /// Return as string.
        std::string ToString() const;

        // Constants
        static const Extent2D Empty;
    };

    struct Extent3D
    {
        /// Specifies the width of the extent.
        uint32_t width;

        /// Specifies the height of the extent.
        uint32_t height;

        /// Specifies the depth of the extent.
        uint32_t depth;

        constexpr Extent3D() : width(0), height(0), depth(0) {}
        constexpr Extent3D(uint32_t value) noexcept : width(value), height(value), depth(value) {}
        constexpr Extent3D(uint32_t width_, uint32_t height_, uint32_t depth_) : width(width_), height(height_), depth(depth_) {}
        constexpr Extent3D(const Extent2D& extent2d, uint32_t depth_) : width(extent2d.width), height(extent2d.height), depth(depth_) {}

        Extent3D(const Extent3D&) = default;
        Extent3D& operator=(const Extent3D&) = default;

        Extent3D(Extent3D&&) = default;
        Extent3D& operator=(Extent3D&&) = default;

        bool operator!=(const Extent3D& other) const
        {
            return width != other.width || height != other.height || depth != other.depth;
        }

        bool operator==(const Extent3D& other) const
        {
            return width == other.width && height == other.height && depth == other.depth;
        }

        /// Return as string.
        std::string ToString() const;

         // Constants
        static const Extent3D Empty;
    };
}
