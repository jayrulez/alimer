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
#include <stdlib.h>
#include <string.h> 
#include <float.h> 

#ifndef AGPU_ASSERT
#   include <assert.h>
#   define AGPU_ASSERT(c) assert(c)
#endif

static inline void* agpu_malloc(size_t size)
{
    void* ptr;
    if (!(ptr = malloc(size)))
        agpu_log_error("Out of memory.");
    return ptr;
}

static inline void* agpu_realloc(void* ptr, size_t size)
{
    if (!(ptr = realloc(ptr, size)))
        agpu_log_error("Out of memory.");
    return ptr;
}

static inline void* agpu_calloc(size_t count, size_t size)
{
    void* ptr;
    AGPU_ASSERT(count <= ~(size_t)0 / size);
    if (!(ptr = calloc(count, size)))
        agpu_log_error("Out of memory.");
    return ptr;
}

static inline void agpu_free(void* ptr)
{
    free(ptr);
}

#define AGPU_ALLOC_HANDLE(T) ((T*)agpu_calloc(1, sizeof(T)))
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

typedef struct agpu_renderer {
    bool (*init)(const agpu_init_info* info);
    void (*shutdown)(void);

    /* context */
    agpu_context(*create_context)(const agpu_context_info* info);
    void (*destroy_context)(agpu_context context);
    void (*set_context)(agpu_context context);
    void (*begin_frame)(void);
    void (*end_frame)(void);

    agpu_device_caps(*query_caps)(void);
    agpu_texture_format_info(*query_texture_format_info)(agpu_texture_format format);
} agpu_renderer;

#define ASSIGN_DRIVER_FUNC(func, name) renderer.func = name##_##func;
#define ASSIGN_DRIVER(name) \
    ASSIGN_DRIVER_FUNC(init, name)\
	ASSIGN_DRIVER_FUNC(shutdown, name)\
    ASSIGN_DRIVER_FUNC(create_context, name)\
    ASSIGN_DRIVER_FUNC(destroy_context, name)\
    ASSIGN_DRIVER_FUNC(set_context, name)\
    ASSIGN_DRIVER_FUNC(begin_frame, name)\
    ASSIGN_DRIVER_FUNC(end_frame, name)\
    ASSIGN_DRIVER_FUNC(query_caps, name)\
    ASSIGN_DRIVER_FUNC(query_texture_format_info, name)

typedef struct agpu_driver
{
    agpu_backend_type backend_type;
    bool (*is_supported)(void);
    agpu_renderer* (*create_renderer)(void);
} agpu_driver;

extern agpu_driver d3d11_driver;
extern agpu_driver vulkan_driver;
extern agpu_driver metal_driver;
extern agpu_driver gl_driver;

#endif /* AGPU_DRIVER_H */
