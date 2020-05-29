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

#ifndef vgpu_assert
#   include <assert.h>
#   define vgpu_assert(c) assert(c)
#endif

#ifndef vgpu_malloc
#   include <stdlib.h>
#   define vgpu_malloc(s) malloc(s)
#   define vgpu_free(p) free(p)
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

#define _vgpu_def(val, def) (((val) == 0) ? (def) : (val))
#define _vgpu_def_flt(val, def) (((val) == 0.0f) ? (def) : (val))
#define _vgpu_min(a,b) ((a<b)?a:b)
#define _vgpu_max(a,b) ((a>b)?a:b)
#define _vgpu_clamp(v,v0,v1) ((v<v0)?(v0):((v>v1)?(v1):(v)))
#define _vgpu_count_of(x) (sizeof(x) / sizeof(x[0]))

typedef struct vgpu_renderer_t* vgpu_renderer;

typedef struct vgpu_device_t {
    void (*destroy)(vgpu_device device);

    /* Context */
    vgpu_context(*create_context)(vgpu_renderer driver_data, const vgpu_context_info* info);
    void(*destroy_context)(vgpu_renderer driver_data, vgpu_context context);
    void(*begin_frame)(vgpu_renderer driver_data, vgpu_context context);
    void(*end_frame)(vgpu_renderer driver_data, vgpu_context context);

    /* Opaque pointer for the implementation */
    vgpu_renderer renderer;
} vgpu_device_t;

#define ASSIGN_DRIVER_FUNC(func, name) result->func = name##_##func;
#define ASSIGN_DRIVER(name) \
	ASSIGN_DRIVER_FUNC(destroy, name)\
    ASSIGN_DRIVER_FUNC(create_context, name)\
    ASSIGN_DRIVER_FUNC(destroy_context, name)\
    ASSIGN_DRIVER_FUNC(begin_frame, name)\
    ASSIGN_DRIVER_FUNC(end_frame, name)

typedef struct vgpu_driver {
    vgpu_backend_type backend_type;
    bool(*is_supported)(void);
    vgpu_device(*create_device)(bool validation);
} vgpu_driver;

extern vgpu_driver d3d11_driver;
extern vgpu_driver vulkan_driver;
extern vgpu_driver gl_driver;

#endif /* VGPU_DRIVER_H */
