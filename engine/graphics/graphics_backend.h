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

#include "graphics/graphics.h"
#include <malloc.h>

namespace alimer
{
    namespace graphics
    {
        void LogError(const char* format, ...);
        void LogWarn(const char* format, ...);
        void LogInfo(const char* format, ...);
        void LogDebug(const char* format, ...);


        template <typename T> ALIMER_FORCEINLINE T* StackAlloc(size_t N) {
#if defined(_MSC_VER)
            return (T*)_malloca(N * sizeof(T));
#else
            return (T*)alloca(N * sizeof(T));
#endif
        }

        template <typename T, uint32_t MAX_COUNT>
        class Pool
        {
        public:
            Pool()
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
                new (&values[idx]) int(first_free);
                first_free = idx;
            }

            alignas(T) uint8_t mem[sizeof(T) * MAX_COUNT];
            T* values;
            int first_free;

            T& operator[](int index) { return values[index]; }
            bool isFull() const { return first_free == -1; }
        };

        struct Renderer
        {
            bool (*Init)(const Configuration& config);
            void (*Shutdown)(void);

            ContextHandle(*CreateContext)(const ContextInfo& info);
            void(*DestroyContext)(ContextHandle handle);
            bool(*ResizeContext)(ContextHandle handle, uint32_t width, uint32_t height);

            bool(*BeginFrame)(ContextHandle handle);
            void(*EndFrame)(ContextHandle handle);
            void(*BeginRenderPass)(ContextHandle handle, const Color& clearColor, float clearDepth, uint8_t clearStencil);
            void(*EndRenderPass)(ContextHandle handle);

            /* Texture */
            TextureHandle(*CreateTexture)(const TextureInfo& info);
            void(*DestroyTexture)(TextureHandle handle);
        };
    }
}
