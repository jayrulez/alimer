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

#include "graphics/GraphicsResource.h"

namespace alimer
{
    enum class BufferUsage : uint32_t
    {
        None = 0,
        MapRead = 1 << 0,
        MapWrite = 1 << 1,
        CopySrc = 1 << 2,
        CopyDst = 1 << 3,
        Index = 1 << 4,
        Vertex = 1 << 5,
        Uniform = 1 << 6,
        Storage = 1 << 7,
        Indirect = 1 << 8,
    };
    ALIMER_DEFINE_ENUM_BITWISE_OPERATORS(BufferUsage);

    /// Describes a Graphics buffer.
    struct BufferDesc
    {
        const char* name = nullptr;
        BufferUsage usage;
        uint64_t size;
    };

    class GraphicsBuffer : public GraphicsResource
    {
        virtual BufferDesc GetDesc() const = 0;
    };
} 
