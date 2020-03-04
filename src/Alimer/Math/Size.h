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
#include <EASTL/string.h>

namespace Alimer
{
    /// Class specifying a two-dimensional size.
    class ALIMER_API Size2D
    {
    public:
        /// Specifies the width of the size.
        uint32_t width;
        /// Specifies the height of the size.
        uint32_t height;

        /// Constructor.
        Size2D() noexcept
            : width(0)
            , height(0)
        {
        }
    };

    /// Class specifying a three-dimensional size.
    class ALIMER_API Size3D
    {
    public:
        /// Specifies the width of the size.
        uint32_t width;
        /// Specifies the height of the size.
        uint32_t height;
        /// Specifies the depth; of the size.
        uint32_t depth;

        /// Constructor.
        Size3D() noexcept
            : width(0)
            , height(0)
            , depth(0)
        {
        }
    };
} 
