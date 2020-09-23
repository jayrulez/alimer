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

#ifndef AGPU_DRIVER_H
#define AGPU_DRIVER_H

#include "agpu.h"
#include <new>
#include <stdlib.h>
#include <string.h> 
#include <float.h> 

#ifndef AGPU_ASSERT
#   include <assert.h>
#   define AGPU_ASSERT(c) assert(c)
#endif

#define AGPU_UNUSED(x) do { (void)sizeof(x); } while(0)

#define _agpu_def(val, def) (((val) == 0) ? (def) : (val))
#define AGPU_DEF_FLOAT(val, def) (((val) == 0.0f) ? (def) : (val))
#define AGPU_MIN(a,b) ((a<b)?a:b)
#define AGPU_MAX(a,b) ((a>b)?a:b)
#define AGPU_CLAMP(v,v0,v1) ((v<v0)?(v0):((v>v1)?(v1):(v)))
#define AGPU_COUNT_OF(x) (sizeof(x) / sizeof(x[0]))

#if defined(__clang__)
#   define AGPU_THREADLOCAL _Thread_local
#   define AGPU_UNREACHABLE() __builtin_unreachable()
#   define AGPU_DEBUG_BREAK() __builtin_trap()
#elif defined(__GNUC__)
#   define AGPU_THREADLOCAL __thread
#   define AGPU_UNREACHABLE() __builtin_unreachable()
#   define AGPU_DEBUG_BREAK() __builtin_trap()
#elif defined(_MSC_VER)
extern void __cdecl __debugbreak(void);
#   define AGPU_THREADLOCAL __declspec(thread)
#   define AGPU_UNREACHABLE() __assume(false)
#   define AGPU_DEBUG_BREAK() __debugbreak()
#else
#   define AGPU_THREADLOCAL
#endif

namespace agpu
{
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
            new (&values[idx]) int(first_free);
            first_free = idx;
        }

        alignas(T) uint8_t mem[sizeof(T) * MAX_COUNT];
        T* values;
        int first_free;

        T& operator[](int idx) { return values[idx]; }
        bool isFull() const { return first_free == -1; }
    };

    struct agpu_renderer {
        bool (*init)(InitFlags flags, const PresentationParameters* presentationParameters);
        void (*shutdown)(void);
        void (*resize)(uint32_t width, uint32_t height);
        bool (*beginFrame)(void);
        void (*endFrame)(void);

        const Caps* (*QueryCaps)(void);

        RenderPassHandle(*CreateRenderPass)(const PassDescription& description);
        void(*DestroyRenderPass)(RenderPassHandle handle);

        BufferHandle(*CreateBuffer)(uint32_t count, uint32_t stride, const void* initialData);
        void(*DestroyBuffer)(BufferHandle handle);

        /* Commands */
        void (*PushDebugGroup)(const char* name);
        void (*PopDebugGroup)(void) = 0;
        void (*InsertDebugMarker)(const char* name);
    };

#define ASSIGN_DRIVER_FUNC(func, name) renderer.func = name##_##func;
#define ASSIGN_DRIVER(name) \
    ASSIGN_DRIVER_FUNC(init, name)\
	ASSIGN_DRIVER_FUNC(shutdown, name)\
    ASSIGN_DRIVER_FUNC(resize, name)\
    ASSIGN_DRIVER_FUNC(beginFrame, name)\
    ASSIGN_DRIVER_FUNC(endFrame, name)\
    ASSIGN_DRIVER_FUNC(QueryCaps, name)\
    ASSIGN_DRIVER_FUNC(CreateRenderPass, name)\
    ASSIGN_DRIVER_FUNC(DestroyRenderPass, name)\
    ASSIGN_DRIVER_FUNC(CreateBuffer, name)\
    ASSIGN_DRIVER_FUNC(DestroyBuffer, name)\
    ASSIGN_DRIVER_FUNC(PushDebugGroup, name)\
    ASSIGN_DRIVER_FUNC(PopDebugGroup, name)\
    ASSIGN_DRIVER_FUNC(InsertDebugMarker, name)

    struct Driver
    {
        BackendType backend;
        bool (*isSupported)(void);
        agpu_renderer* (*createRenderer)(void);
    };

    extern Driver D3D12_Driver;
    extern Driver D3D11_Driver;
    extern Driver vulkan_driver;
    extern Driver metal_driver;
    extern Driver GL_Driver;
}


#endif /* AGPU_DRIVER_H */
