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
    class Buffer : public GraphicsResource
    {
        ALIMER_OBJECT(Buffer, GraphicsResource);

    public:
        /// Constructor
        Buffer();
        ~Buffer() override;

        void Create(BufferUsage usage, uint32_t size, uint32_t stride, const void* initialData = nullptr);
        void Destroy() override;

        /// Gets a value indicating whether this buffer has been allocated. 
        ALIMER_FORCE_INLINE bool IsAllocated() const
        {
            return handle.isValid();
        }

        /// Gets buffer usage.
        ALIMER_FORCE_INLINE BufferUsage GetUsage() const
        {
            return desc.usage;
        }

        /// Gets buffer size in bytes.
        ALIMER_FORCE_INLINE uint32_t GetSize() const
        {
            return desc.size;
        }

        /// Gets buffer elements count.
        ALIMER_FORCE_INLINE uint32_t GetStride() const
        {
            return desc.stride;
        }

        /// Gets the number of elements.
        ALIMER_FORCE_INLINE uint32_t GetElementsCount() const
        {
            ALIMER_ASSERT(desc.stride > 0);
            return desc.size / desc.stride;
        }

    private:
        BufferHandle handle{ kInvalidHandleId };
        BufferDescription desc;
    };
}
