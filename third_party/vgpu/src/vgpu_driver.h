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

extern const vgpu_allocation_callbacks* vgpu_alloc_cb;
extern void* vgpu_allocation_user_data;

#define VGPU_ALLOC(T)     ((T*) vgpu_alloc_cb->allocate_memory(vgpu_allocation_user_data, sizeof(T)))
#define VGPU_FREE(ptr)       (vgpu_alloc_cb->free_memory(vgpu_allocation_user_data, (void*)(ptr)))
#define VGPU_ALLOC_HANDLE(T) ((T*) vgpu_alloc_cb->allocate_cleared_memory(vgpu_allocation_user_data, sizeof(T)))

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

typedef struct vgpu_context {
    bool (*init)(const vgpu_config* config);
    void (*shutdown)(void);
    bool (*frame_begin)(void);
    void (*frame_end)(void);

    /* Texture */
    vgpu_texture(*texture_create)(const vgpu_texture_info* desc);
    void(*texture_destroy)(vgpu_texture handle);
    uint32_t(*texture_get_width)(vgpu_texture handle, uint32_t mipLevel);
    uint32_t(*texture_get_height)(vgpu_texture handle, uint32_t mipLevel);

    /* Framebuffer */
    vgpu_framebuffer(*framebuffer_create)(const vgpu_framebuffer_info* info);
    void(*framebuffer_destroy)(vgpu_framebuffer handle);
    vgpu_framebuffer(*getDefaultFramebuffer)(void);

    /* Buffer */
    vgpu_buffer(*buffer_create)(const vgpu_buffer_info* info);
    void(*buffer_destroy)(vgpu_buffer handle);

    /* Swapchain */
    vgpu_swapchain(*swapchain_create)(uintptr_t window_handle, vgpu_texture_format color_format, vgpu_texture_format depth_stencil_format);
    void(*swapchain_destroy)(vgpu_swapchain handle);
    void(*swapchain_resize)(vgpu_swapchain handle, uint32_t width, uint32_t height);
    void(*swapchain_present)(vgpu_swapchain handle);

    /* CommandBuffer */
    void (*insertDebugMarker)(const char* name);
    void (*pushDebugGroup)(const char* name);
    void (*popDebugGroup)(void);
    void(*beginRenderPass)(vgpu_framebuffer framebuffer);
    void(*render_finish)(void);

} vgpu_context;

#define ASSIGN_DRIVER_FUNC(func, name) context.func = name##_##func;
#define ASSIGN_DRIVER(name) \
ASSIGN_DRIVER_FUNC(init, name)\
ASSIGN_DRIVER_FUNC(shutdown, name)\
ASSIGN_DRIVER_FUNC(frame_begin, name)\
ASSIGN_DRIVER_FUNC(frame_end, name)\
ASSIGN_DRIVER_FUNC(texture_create, name)\
ASSIGN_DRIVER_FUNC(texture_destroy, name)\
ASSIGN_DRIVER_FUNC(texture_get_width, name)\
ASSIGN_DRIVER_FUNC(texture_get_height, name)\
ASSIGN_DRIVER_FUNC(framebuffer_create, name)\
ASSIGN_DRIVER_FUNC(framebuffer_destroy, name)\
ASSIGN_DRIVER_FUNC(swapchain_create, name)\
ASSIGN_DRIVER_FUNC(swapchain_destroy, name)\
ASSIGN_DRIVER_FUNC(swapchain_resize, name)\
ASSIGN_DRIVER_FUNC(swapchain_present, name)\
ASSIGN_DRIVER_FUNC(insertDebugMarker, name)\
ASSIGN_DRIVER_FUNC(pushDebugGroup, name)\
ASSIGN_DRIVER_FUNC(popDebugGroup, name)\
ASSIGN_DRIVER_FUNC(beginRenderPass, name)\
ASSIGN_DRIVER_FUNC(render_finish, name)

typedef struct vgpu_driver {
    vgpu_backend_type backendType;
    bool(*is_supported)(void);
    vgpu_context* (*create_context)(void);
} vgpu_driver;

extern vgpu_driver d3d11_driver;
extern vgpu_driver d3d12_driver;
extern vgpu_driver vulkan_driver;
extern vgpu_driver gl_driver;

#endif /* VGPU_DRIVER_H */
