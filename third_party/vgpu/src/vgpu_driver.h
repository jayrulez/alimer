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

#ifndef __VGPU_DRIVER_H__
#define __VGPU_DRIVER_H__

#include "vgpu.h"
#include <string.h> 
#include <float.h>

#define AGPU_UNUSED(x) do { (void)sizeof(x); } while(0)

#define AGPU_DEF(val, def) (((val) == 0) ? (def) : (val))
#define AGPU_DEF_FLOAT(val, def) (((val) == 0.0f) ? (def) : (val))
#define AGPU_MIN(a,b) ((a<b)?a:b)
#define AGPU_MAX(a,b) ((a>b)?a:b)
#define AGPU_CLAMP(v,v0,v1) ((v<v0)?(v0):((v>v1)?(v1):(v)))
#define VGPU_COUNT_OF(x) (sizeof(x) / sizeof(x[0]))

#ifndef VGPU_MALLOC
#   include <stdlib.h>
#   define VGPU_MALLOC(s) malloc(s)
#   define VGPU_FREE(p) free(p)
#   define VGPU_ALLOC(T) (T*)(VGPU_MALLOC(sizeof(T)))
#endif

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

#ifndef AGPU_ASSERT
#   define AGPU_ASSERT(cond) do { \
		if (!(cond)) { \
		    vgpu_log(VGPU_LOG_LEVEL_ERROR, #cond); \
			AGPU_UNREACHABLE(); \
		} \
	} while(0)
#endif

typedef struct agpu_renderer {
    bool (*init)(const char* app_name, const agpu_config* config);
    void (*shutdown)(void);
    bool(*frame_begin)(void);
    void(*frame_finish)(void);

    void (*query_caps)(agpu_caps* caps);

    vgpu_buffer(*buffer_create)(const agpu_buffer_info* info);
    void(*buffer_destroy)(vgpu_buffer handle);

    vgpu_shader(*shader_create)(const vgpu_shader_info* info);
    void(*shader_destroy)(vgpu_shader handle);

    vgpu_texture(*texture_create)(const vgpu_texture_info* info);
    void(*texture_destroy)(vgpu_texture handle);
    uint64_t(*texture_get_native_handle)(vgpu_texture handle);

    vgpu_pipeline(*pipeline_create)(const vgpu_pipeline_info* info);
    void(*pipeline_destroy)(vgpu_pipeline handle);

    /* Commands */
    void(*push_debug_group)(const char* name);
    void(*pop_debug_group)(void);
    void(*insert_debug_marker)(const char* name);
    void(*begin_render_pass)(const agpu_render_pass_info* info);
    void(*end_render_pass)(void);
    void(*bind_pipeline)(vgpu_pipeline handle);
    void(*draw)(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex);
} agpu_renderer;

#define ASSIGN_DRIVER_FUNC(func, name) renderer.func = name##_##func;
#define ASSIGN_DRIVER(name) \
    ASSIGN_DRIVER_FUNC(init, name)\
    ASSIGN_DRIVER_FUNC(shutdown, name)\
    ASSIGN_DRIVER_FUNC(frame_begin, name)\
    ASSIGN_DRIVER_FUNC(frame_finish, name)\
    ASSIGN_DRIVER_FUNC(query_caps, name)\
    ASSIGN_DRIVER_FUNC(buffer_create, name)\
    ASSIGN_DRIVER_FUNC(buffer_destroy, name)\
    ASSIGN_DRIVER_FUNC(shader_create, name)\
    ASSIGN_DRIVER_FUNC(shader_destroy, name)\
    ASSIGN_DRIVER_FUNC(texture_create, name)\
    ASSIGN_DRIVER_FUNC(texture_destroy, name)\
    ASSIGN_DRIVER_FUNC(texture_get_native_handle, name)\
    ASSIGN_DRIVER_FUNC(pipeline_create, name)\
    ASSIGN_DRIVER_FUNC(pipeline_destroy, name)\
    ASSIGN_DRIVER_FUNC(push_debug_group, name)\
    ASSIGN_DRIVER_FUNC(pop_debug_group, name)\
    ASSIGN_DRIVER_FUNC(insert_debug_marker, name)\
    ASSIGN_DRIVER_FUNC(begin_render_pass, name)\
    ASSIGN_DRIVER_FUNC(end_render_pass, name) \
    ASSIGN_DRIVER_FUNC(bind_pipeline, name) \
    ASSIGN_DRIVER_FUNC(draw, name) \

typedef struct agpu_driver
{
    agpu_backend_type backend;
    bool (*is_supported)(void);
    agpu_renderer* (*create_renderer)(void);
} agpu_driver;

//extern agpu_driver d3d12_driver;
//extern agpu_driver D3D11_Driver;
extern agpu_driver vulkan_driver;
//extern agpu_driver metal_driver;
//extern agpu_driver GL_Driver;

#endif /* __VGPU_DRIVER_H__ */
