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
    /// Defines a two-dimensional floating-point size.
    class ALIMER_API Size
    {
    public:
        /// Specifies the width of the size.
        float width;
        /// Specifies the height of the size.
        float height;

        Size() noexcept : width(0.0f), height(0.0f) {}
        constexpr Size(float width_, float height_) noexcept : width(width_), height(height_) {}
        explicit Size(uint32_t width_, uint32_t height_) noexcept : width(float(width_)), height(float(height_)) {}

        Size(const Size&) = default;
        Size& operator=(const Size&) = default;

        Size(Size&&) = default;
        Size& operator=(Size&&) = default;

        // Comparison operators
        bool operator == (const Size& rhs) const noexcept { return (width == rhs.width) && (height == rhs.height); }
        bool operator != (const Size& rhs) const noexcept { return (width != rhs.width) || (height != rhs.height); }

        /// Return as string.
        String ToString() const;

        // Constants
        static const Size Empty;
    };

    /// Defines a two-dimensional integer size.
    class ALIMER_API SizeI
    {
    public:
        /// Specifies the width of the size.
        int width;
        /// Specifies the height of the size.
        int height;

        SizeI() noexcept : width(0), height(0) {}
        constexpr SizeI(int width_, int height_) noexcept : width(width_), height(height_) {}

        SizeI(const SizeI&) = default;
        SizeI& operator=(const SizeI&) = default;

        SizeI(SizeI&&) = default;
        SizeI& operator=(SizeI&&) = default;

        // Comparison operators
        bool operator == (const SizeI& rhs) const noexcept { return (width == rhs.width) && (height == rhs.height); }
        bool operator != (const SizeI& rhs) const noexcept { return (width != rhs.width) || (height != rhs.height); }

        /// Return as string.
        String ToString() const;
    };
}
