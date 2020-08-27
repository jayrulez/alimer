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

#include "Graphics/GPUResource.h"

namespace Alimer
{
    /// 
    class GPUBuffer final : public GPUResource
    {
        ALIMER_OBJECT(GPUBuffer, GPUResource);

    public:
        /// Constructor
        GPUBuffer(const std::string_view& name);

        bool Init(GPUBufferUsage usage, uint64_t size, uint64_t stride, const void* initialData);

        /// Gets a value indicating whether this buffer has been allocated. 
        bool IsAllocated() const;

        /// Gets buffer usage.
        ALIMER_FORCE_INLINE GPUBufferUsage GetUsage() const
        {
            return _usage;
        }

        /// Gets buffer size in bytes.
        ALIMER_FORCE_INLINE uint64_t GetSize() const
        {
            return _size;
        }

        /// Gets buffer elements count.
        ALIMER_FORCE_INLINE uint32_t GetStride() const
        {
            return _stride;
        }

        /// Gets the number of elements.
        ALIMER_FORCE_INLINE uint32_t GetElementsCount() const
        {
            ALIMER_ASSERT(_stride > 0);
            return _size / _stride;
        }

    private:
        gpu::BufferHandle handle;
        GPUBufferUsage _usage{ GPUBufferUsage::None };
        uint64_t _size{ 0 };
        uint32_t _stride{ 0 };
    };
}
