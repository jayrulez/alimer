//
// Copyright (c) 2020 Amer Koleci.
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

#include "gpu.h"
#include <string.h> /* memset */
#include <float.h> /* FLT_MAX */

#if defined(_MSC_VER) || defined(__MINGW32__)
#   include <malloc.h>
#else
#   include <alloca.h>
#endif

#ifndef VGPU_ASSERT
#   include <assert.h>
#   define AGPU_ASSERT(c) assert(c)
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
#   define AGPU_FREE(p) free(p)
#endif

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

#define _AGPU_UNUSED(x) do { (void)sizeof(x); } while(0)
#define _AGPU_ALLOC_HANDLE(type) ((type*)VGPU_MALLOC(sizeof(type)))

#define _agpu_def(val, def) (((val) == 0) ? (def) : (val))
#define _agpu_def_flt(val, def) (((val) == 0.0f) ? (def) : (val))
#define _agpu_min(a,b) ((a<b)?a:b)
#define _agpu_max(a,b) ((a>b)?a:b)
#define _agpu_clamp(v,v0,v1) ((v<v0)?(v0):((v>v1)?(v1):(v)))
#define GPU_VOIDP_TO_U64(x) (((union { uint64_t u; void* p; }) { .p = x }).u)

typedef struct agpu_renderer {
    bool (*init)(const agpu_config* config);
    void (*shutdown)(void);

    void (*frame_wait)(void);
    void (*frame_finish)(void);

    agpu_backend_type(*query_backend)(void);
    void(*get_limits)(agpu_limits* limits);

    AGPUPixelFormat(*get_default_depth_format)(void);
    AGPUPixelFormat(*get_default_depth_stencil_format)(void);

    /* Buffer */
    agpu_buffer (*create_buffer)(const agpu_buffer_info* info);
    void (*destroy_buffer)(agpu_buffer handle);

    /* Texture */
    agpu_texture (*create_texture)(const agpu_texture_info* info);
    void (*destroy_texture)(agpu_texture handle);

    /* Shader */
    agpu_shader (*create_shader)(const agpu_shader_info* info);
    void (*destroy_shader)(agpu_shader handle);

    /* Pipeline */
    agpu_pipeline (*create_render_pipeline)(const agpu_render_pipeline_info* info);
    void (*destroy_pipeline)(agpu_pipeline handle);

    /* CommandBuffer */
    void (*set_pipeline)(agpu_pipeline pipeline);
    void (*cmdSetVertexBuffer)(uint32_t slot, agpu_buffer buffer, uint64_t offset);
    void (*cmdSetIndexBuffer)(agpu_buffer buffer, uint64_t offset);
    void (*cmdDraw)(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex);
    void (*cmdDrawIndexed)(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex);

} agpu_renderer;

typedef struct agpu_driver {
    bool (*supported)(void);
    agpu_renderer* (*create_renderer)(void);
} agpu_driver;

#if defined(GPU_GL_BACKEND)
extern agpu_driver gl_driver;
#endif

#if defined(GPU_D3D11_BACKEND) && defined(TODO_D3D12)
extern agpu_driver d3d11_driver;
#endif

#if defined(GPU_D3D12_BACKEND) && defined(TODO_D3D12)
extern agpu_driver d3d12_driver;
#endif

#if defined(GPU_VK_BACKEND) && TODO_VK
extern agpu_driver vulkan_driver;
#endif

#define ASSIGN_DRIVER_FUNC(func, name) renderer.func = name##_##func;
#define ASSIGN_DRIVER(name) \
ASSIGN_DRIVER_FUNC(init, name)\
ASSIGN_DRIVER_FUNC(shutdown, name)\
ASSIGN_DRIVER_FUNC(frame_wait, name)\
ASSIGN_DRIVER_FUNC(frame_finish, name)\
ASSIGN_DRIVER_FUNC(query_backend, name)\
ASSIGN_DRIVER_FUNC(get_limits, name)\
ASSIGN_DRIVER_FUNC(get_default_depth_format, name)\
ASSIGN_DRIVER_FUNC(get_default_depth_stencil_format, name)\
ASSIGN_DRIVER_FUNC(create_buffer, name)\
ASSIGN_DRIVER_FUNC(destroy_buffer, name)\
ASSIGN_DRIVER_FUNC(create_texture, name)\
ASSIGN_DRIVER_FUNC(destroy_texture, name)\
ASSIGN_DRIVER_FUNC(create_shader, name)\
ASSIGN_DRIVER_FUNC(destroy_shader, name)\
ASSIGN_DRIVER_FUNC(create_render_pipeline, name)\
ASSIGN_DRIVER_FUNC(destroy_pipeline, name)\
ASSIGN_DRIVER_FUNC(set_pipeline, name)\
ASSIGN_DRIVER_FUNC(cmdSetVertexBuffer, name)\
ASSIGN_DRIVER_FUNC(cmdSetIndexBuffer, name)\
ASSIGN_DRIVER_FUNC(cmdDraw, name)\
ASSIGN_DRIVER_FUNC(cmdDrawIndexed, name)
