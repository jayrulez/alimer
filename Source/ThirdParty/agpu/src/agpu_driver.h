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

#define AGPU_UNUSED(x) do { (void)sizeof(x); } while(0)

#define AGPU_DEF(val, def) (((val) == 0) ? (def) : (val))
#define AGPU_DEF_FLOAT(val, def) (((val) == 0.0f) ? (def) : (val))
#define AGPU_MIN(a,b) ((a<b)?a:b)
#define AGPU_MAX(a,b) ((a>b)?a:b)
#define AGPU_CLAMP(v,v0,v1) ((v<v0)?(v0):((v>v1)?(v1):(v)))
#define AGPU_COUNT_OF(x) (sizeof(x) / sizeof(x[0]))

#if defined(__GNUC__) || defined(__clang__)
#   if defined(__i386__) || defined(__x86_64__)
#       define AGPU_BREAKPOINT() __asm__ __volatile__("int $3\n\t")
#   else
#       define AGPU_BREAKPOINT() ((void)0)
#   endif
#   define AGPU_UNREACHABLE() __builtin_unreachable()

#elif defined(_MSC_VER)
extern void __cdecl __debugbreak(void);
#   define AGPU_BREAKPOINT() __debugbreak()
#   define AGPU_UNREACHABLE() __assume(false)
#else
#   error "Unsupported compiler"
#endif

typedef struct agpu_renderer agpu_renderer;

typedef struct agpu_device_t {
    void (*destroy)(agpu_device device);
    void (*frame_begin)(agpu_renderer* driver_data);
    void (*frame_end)(agpu_renderer* driver_data);

    agpu_device_caps(*query_caps)(agpu_renderer* driver_data);
    agpu_texture_format_info(*query_texture_format_info)(agpu_renderer* driver_data, agpu_texture_format format);

    /* Opaque pointer for the Driver */
    agpu_renderer* driver_data;
} agpu_device_t;

#define ASSIGN_DRIVER_FUNC(func, name) result->func = name##_##func;
#define ASSIGN_DRIVER(name) \
	ASSIGN_DRIVER_FUNC(destroy, name)\
    ASSIGN_DRIVER_FUNC(frame_begin, name)\
    ASSIGN_DRIVER_FUNC(frame_end, name)\
    ASSIGN_DRIVER_FUNC(query_caps, name)\
    ASSIGN_DRIVER_FUNC(query_texture_format_info, name)

typedef struct agpu_driver
{
    agpu_backend_type backend_type;
    bool (*is_supported)(void);
    agpu_device (*create_device)(const agpu_device_info* info);
} agpu_driver;

extern agpu_driver d3d11_driver;
extern agpu_driver vulkan_driver;
extern agpu_driver metal_driver;
extern agpu_driver gl_driver;

#endif /* AGPU_DRIVER_H */
