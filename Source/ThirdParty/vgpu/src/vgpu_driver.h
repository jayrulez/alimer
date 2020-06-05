//
// Copyright (c) 2019-2020 Amer Koleci.
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

#ifndef VGPU_DRIVER_H
#define VGPU_DRIVER_H

#include "vgpu.h"
#include <string.h> /* memset */
#include <float.h> /* FLT_MAX */

#ifndef VGPU_ASSERT
#   include <assert.h>
#   define VGPU_ASSERT(c) assert(c)
#endif

#if defined(__GNUC__) || defined(__clang__)
#   if defined(__i386__) || defined(__x86_64__)
#       define VGPU_BREAKPOINT() __asm__ __volatile__("int $3\n\t")
#   else
#       define VGPU_BREAKPOINT() ((void)0)
#   endif
#   define VGPU_UNREACHABLE() __builtin_unreachable()

#elif defined(_MSC_VER)
extern void __cdecl __debugbreak(void);
#   define VGPU_BREAKPOINT() __debugbreak()
#   define VGPU_UNREACHABLE() __assume(false)
#else
#   error "Unsupported compiler"
#endif

#define VGPU_UNUSED(x) do { (void)sizeof(x); } while(0)

#define _vgpu_def(val, def) (((val) == 0) ? (def) : (val))
#define _vgpu_def_flt(val, def) (((val) == 0.0f) ? (def) : (val))
#define _vgpu_min(a,b) ((a<b)?a:b)
#define _vgpu_max(a,b) ((a>b)?a:b)
#define _vgpu_clamp(v,v0,v1) ((v<v0)?(v0):((v>v1)?(v1):(v)))
#define _vgpu_count_of(x) (sizeof(x) / sizeof(x[0]))

extern const vgpu_allocation_callbacks* vgpu_alloc_cb;
extern void* vgpu_allocation_user_data;

#define VGPU_ALLOC(type)     ((type*) vgpu_alloc_cb->allocate_memory(vgpu_allocation_user_data, sizeof(type)))
#define VGPU_ALLOCN(type, n) ((type*) vgpu_alloc_cb->allocate_memory(vgpu_allocation_user_data, sizeof(type) * n))
#define VGPU_FREE(ptr)       (vgpu_alloc_cb->free_memory(vgpu_allocation_user_data, (void*)(ptr)))

typedef struct vgpu_renderer_t* vgpu_renderer;

typedef struct vgpu_device_t {
    void (*destroy)(vgpu_device device);
    void(*begin_frame)(vgpu_renderer driver_data);
    void(*present_frame)(vgpu_renderer driver_data);

    /* Opaque pointer for the implementation */
    vgpu_renderer renderer;
} vgpu_device_t;

#define ASSIGN_DRIVER_FUNC(func, name) result->func = name##_##func;
#define ASSIGN_DRIVER(name) \
	ASSIGN_DRIVER_FUNC(destroy, name)\
    ASSIGN_DRIVER_FUNC(begin_frame, name)\
    ASSIGN_DRIVER_FUNC(present_frame, name)

typedef struct vgpu_driver {
    vgpu_backend_type backend_type;
    bool(*is_supported)(void);
    vgpu_device(*create_device)(const vgpu_device_desc* desc);
} vgpu_driver;

extern vgpu_driver d3d11_driver;
extern vgpu_driver vulkan_driver;
extern vgpu_driver gl_driver;

#endif /* VGPU_DRIVER_H */
