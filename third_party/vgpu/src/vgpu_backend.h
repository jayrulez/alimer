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

#include <vgpu/vgpu.h>
#include <string.h>
#include <new>

#if defined(_MSC_VER) || defined(__MINGW32__)
#   include <malloc.h>
#else
#   include <alloca.h>
#endif

#ifndef __cplusplus
#  define nullptr ((void*)0)
#endif

#if !defined(NDEBUG) || defined(DEBUG) || defined(_DEBUG)
#   define VGPU_DEBUG 
#endif

#ifndef _VGPU_PRIVATE
#   if defined(__GNUC__)
#       define _VGPU_PRIVATE __attribute__((unused)) static
#   else
#       define _VGPU_PRIVATE static
#   endif
#endif

#ifndef VGPU_ASSERT
#   include <assert.h>
#   define VGPU_ASSERT(c) assert(c)
#endif

#ifndef VGPU_MALLOC
#   include <stdlib.h>
#   define VGPU_MALLOC(s) malloc(s)
#   define VGPU_FREE(p) free(p)
#endif

#if defined(__GNUC__) || defined(__clang__)
#   if defined(__i386__) || defined(__x86_64__)
#       define _VGPU_BREAKPOINT() __asm__ __volatile__("int $3\n\t")
#   else
#       define _VGPU_BREAKPOINT() ((void)0)
#   endif
#   define _VGPU_UNREACHABLE() __builtin_unreachable()

#elif defined(_MSC_VER)
extern void __cdecl __debugbreak(void);
#   define _VGPU_BREAKPOINT() __debugbreak()
#   define _VGPU_UNREACHABLE() __assume(false)
#else
#   error "Unsupported compiler"
#endif

#define _VGPU_UNUSED(x) do { (void)sizeof(x); } while(0)

#if defined(_MSC_VER) || defined(__MINGW32__)
#   define _vgpu_alloca(type, count) ((type*)(_malloca(sizeof(type) * (count))))
#else
#   define _vgpu_alloca(type, count) ((type*)(alloca(sizeof(type) * (count))))
#endif

#define _vgpu_def(val, def) (((val) == 0) ? (def) : (val))
#define _vgpu_min(a,b) ((a<b)?a:b)
#define _vgpu_max(a,b) ((a>b)?a:b)

#define VGPU_ALLOC_HANDLE(type) ((type*)VGPU_MALLOC(sizeof(type)))
#define VGPU_ALLOCN(type, count) ((type*)VGPU_MALLOC(sizeof(type) * count))
#define GPU_UNUSED(x) do { (void)sizeof(x); } while(0)
#define GPU_THROW(s) vgpu_log(VGPU_LOG_LEVEL_ERROR, s);
#define GPU_CHECK(c, s) if (!(c)) { GPU_THROW(s); }

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

typedef struct vgpu_renderer {

    bool (*init)(const char* application_name, const vgpu_desc* desc);
    void (*shutdown)(void);
    void (*wait_idle)(void);
    void (*begin_frame)(void);
    void (*end_frame)(void);

    vgpu_context* (*create_context)(vgpu_renderer* driver_data, const vgpu_context_desc* desc);
    void (*destroy_context)(vgpu_renderer* driver_data, vgpu_context* context);
    void (*set_context)(vgpu_renderer* driver_data, vgpu_context* context);

    vgpu_backend(*get_backend)(void);
    vgpu_features (*query_features)(void);
    vgpu_limits (*query_limits)(void);

    vgpu_buffer* (*create_buffer)(vgpu_renderer* driver_data, const vgpu_buffer_desc* desc);
    void (*destroy_buffer)(vgpu_renderer* driver_data, vgpu_buffer* driver_buffer);

    vgpu_texture* (*create_texture)(vgpu_renderer* driver_data, const vgpu_texture_desc* desc);
    void (*destroy_texture)(vgpu_renderer* driver_data, vgpu_texture* driver_texture);
} vgpu_renderer;

#if defined(VGPU_D3D12_BACKEND)
extern int vgpu_d3d12_supported(void);
extern vgpu_renderer* vgpu_create_d3d12_backend(void);
#endif

#define ASSIGN_DRIVER_FUNC(func, name) render_api.func = name##_##func;
#define ASSIGN_DRIVER(name)\
ASSIGN_DRIVER_FUNC(init, name)\
ASSIGN_DRIVER_FUNC(shutdown, name)\
ASSIGN_DRIVER_FUNC(wait_idle, name)\
ASSIGN_DRIVER_FUNC(begin_frame, name)\
ASSIGN_DRIVER_FUNC(end_frame, name)\
ASSIGN_DRIVER_FUNC(get_backend, name)

