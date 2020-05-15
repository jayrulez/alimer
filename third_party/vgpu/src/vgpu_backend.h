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

#pragma once

#include "vgpu/vgpu.h"
#include <stdbool.h>
#include <string.h> /* memset */

#if defined(_MSC_VER) || defined(__MINGW32__)
#   include <malloc.h>
#else
#   include <alloca.h>
#endif

#ifndef VGPU_ASSERT
#   include <assert.h>
#   define VGPU_ASSERT(c) assert(c)
#endif

#ifndef VGPU_ALLOCA
#   if defined(_MSC_VER) || defined(__MINGW32__)
#       define VGPU_ALLOCA(type, count) ((type*)(_malloca(sizeof(type) * (count))))
#   else
#       define VGPU_ALLOCA(type, count) ((type*)(alloca(sizeof(type) * (count))))
#   endif
#endif

#ifndef VGPU_MALLOC
#   include <stdlib.h>
#   define VGPU_MALLOC(s) malloc(s)
#   define VGPU_FREE(p) free(p)
#endif

#define VGPU_CHECK(c, s) if (!(c)) { vgpu_log(VGPU_LOG_LEVEL_ERROR, s); _VGPU_BREAKPOINT(); }

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

#if defined( __clang__ )
#   define VGPU_UNREACHABLE() __builtin_unreachable()
#   define VGPU_THREADLOCAL _Thread_local
#elif defined(__GNUC__)
#   define VGPU_UNREACHABLE() __builtin_unreachable()
#   define VGPU_THREADLOCAL __thread
#elif defined(_MSC_VER)
#   define VGPU_UNREACHABLE() __assume(false)
#   define VGPU_THREADLOCAL __declspec(thread)
#else
#   define VGPU_UNREACHABLE()((void)0)
#   define VGPU_THREADLOCAL
#endif

#define _VGPU_UNUSED(x) do { (void)sizeof(x); } while(0)
#define _VGPU_ALLOC_HANDLE(type) ((type*)(calloc(1, sizeof(type))))

#define _vgpu_def(val, def) (((val) == 0) ? (def) : (val))
#define _vgpu_def_flt(val, def) (((val) == 0.0f) ? (def) : (val))
#define _vgpu_min(a,b) ((a<b)?a:b)
#define _vgpu_max(a,b) ((a>b)?a:b)
#define _vgpu_clamp(v,v0,v1) ((v<v0)?(v0):((v>v1)?(v1):(v)))

typedef struct vgpu_renderer {
    vgpu_bool(*init)(const vgpu_config* config);
    void (*destroy)(void);
    vgpu_backend_type(*get_backend)(void);
    vgpu_caps(*get_caps)(void);
    VGPUTextureFormat(*GetDefaultDepthFormat)(void);
    VGPUTextureFormat(*GetDefaultDepthStencilFormat)(void);

    void (*begin_frame)(void);
    void (*end_frame)(void);

    /* Texture */
    vgpu_texture(*create_texture)(const VGPUTextureDescriptor* desc);
    void (*destroy_texture)(vgpu_texture handle);

    /* Buffer */
    vgpu_buffer(*create_buffer)(const vgpu_buffer_info* info);
    void (*destroy_buffer)(vgpu_buffer handle);

    /* Sampler */
    vgpu_sampler(*create_sampler)(const vgpu_sampler_info* info);
    void (*destroy_sampler)(vgpu_sampler handle);

    /* Shader */
    vgpu_shader(*create_shader)(const vgpu_shader_info* info);
    void (*destroy_shader)(vgpu_shader handle);

    /* Commands */
    void (*cmdBeginRenderPass)(const VGPURenderPassDescriptor* descriptor);
    void (*cmdEndRenderPass)(void);
} vgpu_renderer;

typedef struct vgpu_driver {
    vgpu_bool (*supported)(void);
    vgpu_renderer* (*init_renderer)(void);
} vgpu_driver;

#if defined(VGPU_BACKEND_OPENGL)
extern vgpu_driver gl_driver;
#endif

/* d3d11 */
#if defined(VGPU_BACKEND_D3D11)
extern vgpu_driver d3d11_driver;
#endif

/* d3d12 */
#if defined(VGPU_BACKEND_D3D12)
extern vgpu_driver d3d12_driver;
#endif

/* vulkan */
#if defined(VGPU_BACKEND_VK)
extern vgpu_driver vk_driver;
#endif

#define ASSIGN_DRIVER_FUNC(func, name) renderer.func = name##_##func;
#define ASSIGN_DRIVER(name) \
ASSIGN_DRIVER_FUNC(init, name)\
ASSIGN_DRIVER_FUNC(destroy, name)\
ASSIGN_DRIVER_FUNC(get_backend, name)\
ASSIGN_DRIVER_FUNC(get_caps, name)\
ASSIGN_DRIVER_FUNC(GetDefaultDepthFormat, name)\
ASSIGN_DRIVER_FUNC(GetDefaultDepthStencilFormat, name)\
ASSIGN_DRIVER_FUNC(begin_frame, name)\
ASSIGN_DRIVER_FUNC(end_frame, name)\
ASSIGN_DRIVER_FUNC(create_texture, name)\
ASSIGN_DRIVER_FUNC(destroy_texture, name)\
ASSIGN_DRIVER_FUNC(create_buffer, name)\
ASSIGN_DRIVER_FUNC(destroy_buffer, name)\
ASSIGN_DRIVER_FUNC(create_sampler, name)\
ASSIGN_DRIVER_FUNC(destroy_sampler, name)\
ASSIGN_DRIVER_FUNC(create_shader, name)\
ASSIGN_DRIVER_FUNC(destroy_shader, name)
