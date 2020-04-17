//
// Copyright (c) 2020 Amer Koleci and contributors.
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

#include "graphics/Types.h"
#include "math/Color.h"

namespace alimer
{
    /* Handle Pool */
    template <typename T, uint32_t MAX_COUNT>
    struct Pool
    {
        void init()
        {
            values = (T*)mem;
            for (int i = 0; i < MAX_COUNT; ++i) {
                new (&values[i]) int(i + 1);
            }
            new (&values[MAX_COUNT - 1]) int(-1);
            first_free = 0;
        }

        int alloc()
        {
            if (first_free == -1) return -1;

            const int id = first_free;
            first_free = *((int*)&values[id]);
            new (&values[id]) T;
            return id;
        }

        void dealloc(uint32_t idx)
        {
            values[idx].~T();
            *((int*)&values[idx]) = first_free;
            first_free = idx;
        }

        alignas(T) uint8_t mem[sizeof(T) * MAX_COUNT];
        T* values;
        int first_free;

        T& operator[](int idx) { return values[idx]; }
        bool isFull() const { return first_free == -1; }

        static constexpr uint32_t CAPACITY = MAX_COUNT;
    };

    class GpuCommandBuffer
    {
    public:
        /// Destructor.
        virtual ~GpuCommandBuffer() = default;

        virtual void BeginMarker(const char* name) = 0;
        virtual void EndMarker() = 0;
        virtual void Flush(bool wait) = 0;

        virtual void BeginRenderPass(GPUTexture texture, const Color& clearColor) = 0;
        virtual void EndRenderPass() = 0;
    };

    class GraphicsImpl
    {
    public:
        GraphicsImpl() = default;

        /// Destructor.
        virtual ~GraphicsImpl() = default;

        GraphicsImpl(const GraphicsImpl&) = delete;
        GraphicsImpl& operator=(const GraphicsImpl&) = delete;
        GraphicsImpl(GraphicsImpl&&) = delete;
        GraphicsImpl& operator=(GraphicsImpl&&) = delete;

        virtual bool Init() = 0;
        virtual void WaitForIdle() = 0;

        /* CommandBuffer */
        virtual GpuCommandBuffer* CreateCommandBuffer(QueueType family) = 0;
        virtual void DestroyCommandBuffer(GpuCommandBuffer* handle) = 0;
    };
}
