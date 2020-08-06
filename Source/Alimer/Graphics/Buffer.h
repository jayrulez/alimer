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
#include "Graphics/Backend.h"
#include <EASTL/string_view.h>

namespace alimer
{
    /// 
    class Buffer final : public GraphicsResource
    {
    public:
        /// Constructor
        Buffer(const eastl::string_view& name = "");
        ~Buffer() override;
        void Destroy() override;

        bool Create(BufferUsage usage, uint32_t count, uint32_t stride, const void* data = nullptr);

        template<typename T>
        bool Create(BufferUsage usage, uint32_t count, const T* data)
        {
            return Create(usage, count, sizeof(T), data);
        }

        template<typename T>
        bool Create(BufferUsage usage, const T& data)
        {
            return Create(usage, data.size(), sizeof(typename T::value_type), data.data());
        }

        /// Gets buffer size in bytes.
        uint32_t GetSize() const { return size; }

        /// Gets buffer elements count.
        uint32_t GetElementCount() const { return elementCount; }

        /// Gets size of single element in the buffer.
        uint32_t GetElementSize() const { return elementSize; }

    private:
        bool BackendCreate(const void* data);
        void BackendSetName() override;

        BufferUsage usage{ BufferUsage::None };
        uint32_t size{ 0 };
        uint32_t elementCount{ 0 };
        uint32_t elementSize{ 0 };

        BufferHandle handle{};
#if defined(ALIMER_D3D12) || defined(ALIMER_VULKAN)
        AllocationHandle allocation{};
#endif
    };
}
