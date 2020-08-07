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

#include "Graphics/GraphicsResource.h"
#include <EASTL/string_view.h>

namespace alimer
{
    /// 
    class Buffer : public RefCounted
    {
    public:
        /// Constructor
        Buffer(const BufferDescription& desc);
        virtual ~Buffer() = default;

        /// Gets buffer usage.
        BufferUsage GetUsage() const { return usage; }

        /// Gets buffer size in bytes.
        uint32_t GetSize() const { return size; }

        /// Gets buffer elements count.
        uint32_t GetStride() const { return stride; }

        /// Gets size of single element in the buffer.
        uint32_t GetElementSize() const { return size / stride; }

    protected:
        BufferUsage usage;
        uint32_t size;
        uint32_t stride;
    };
}
