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

#ifndef _VGPU_DRIVER_H
#define _VGPU_DRIVER_H

#include "vgpu.h"

#ifndef VGPU_ASSERT
#   include <assert.h>
#   define VGPU_ASSERT(c) assert(c)
#endif

extern const vgpu_allocation_callbacks* vgpu_alloc_cb;
extern void* vgpu_allocation_user_data;

#define VGPU_ALLOC(T)     ((T*) vgpu_alloc_cb->allocate(vgpu_allocation_user_data, sizeof(T)))
#define VGPU_FREE(ptr)       (vgpu_alloc_cb->free(vgpu_allocation_user_data, (void*)(ptr)))
#define VGPU_ALLOC_HANDLE(T) ((T*) vgpu_alloc_cb->allocate_cleared(vgpu_allocation_user_data, sizeof(T)))

#ifndef VGPU_ALLOCA
#   include <malloc.h>
#   if defined(_MSC_VER) || defined(__MINGW32__)
#       define VGPU_ALLOCA(type, count) ((type*)(_malloca(sizeof(type) * (count))))
#   else
#       define VGPU_ALLOCA(type, count) ((type*)(alloca(sizeof(type) * (count))))
#   endif
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

typedef struct vgpu_renderer {
    bool (*init)(const vgpu_init_info* config);
    void (*shutdown)(void);
    vgpu_caps(*query_caps)(void);
    void (*begin_frame)(void);
    void (*end_frame)(void);

    vgpu_texture(*texture_create)(const vgpu_texture_info* info);
    void(*texture_destroy)(vgpu_texture handle);
    vgpu_texture_info(*query_texture_info)(vgpu_texture handle);

    vgpu_texture(*get_backbuffer_texture)(void);
    void (*begin_pass)(const vgpu_pass_begin_info* info);
    void (*end_pass)(void);
} vgpu_renderer;

typedef struct vgpu_driver {
    vgpu_backend_type backendType;
    bool(*is_supported)(void);
    vgpu_renderer* (*init_renderer)(void);
} vgpu_driver;

extern vgpu_driver d3d11_driver;
extern vgpu_driver d3d12_driver;
extern vgpu_driver vulkan_driver;

#endif /* _VGPU_DRIVER_H */
