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

#define _AGPU_UNUSED(x) do { (void)sizeof(x); } while(0)
#define _AGPU_ALLOC_HANDLE(T) (T*)calloc(1, sizeof(T))

#define _agpu_def(val, def) (((val) == 0) ? (def) : (val))
#define _agpu_def_flt(val, def) (((val) == 0.0f) ? (def) : (val))
#define _agpu_min(a,b) ((a<b)?a:b)
#define _agpu_max(a,b) ((a>b)?a:b)
#define _agpu_clamp(v,v0,v1) ((v<v0)?(v0):((v>v1)?(v1):(v)))
#define GPU_VOIDP_TO_U64(x) (((union { uint64_t u; void* p; }) { .p = x }).u)

typedef struct VGPUDeviceImpl* VGPUDevice;
typedef struct VGPURenderer VGPURenderer;

typedef struct VGPUDeviceImpl {
    /* Opaque pointer for the renderer. */
    VGPURenderer* renderer;

    bool (*init)(VGPUDevice device, const VGpuDeviceDescriptor* descriptor);
    void (*destroy)(VGPUDevice device);

    void (*frame_wait)(VGPURenderer* driverData);
    void (*frame_finish)(VGPURenderer* driverData);

    VGPUBackendType(*getBackend)(void);
    const VGPUDeviceCaps* (*get_caps)(VGPURenderer* driverData);

    AGPUPixelFormat(*get_default_depth_format)(VGPURenderer* driverData);
    AGPUPixelFormat(*get_default_depth_stencil_format)(VGPURenderer* driverData);

    /* Buffer */
    vgpu_buffer* (*buffer_create)(VGPURenderer* driverData, const vgpu_buffer_info* info);
    void (*buffer_destroy)(VGPURenderer* driverData, vgpu_buffer* handle);
    void (*buffer_sub_data)(VGPURenderer* driverData, vgpu_buffer* handle, VGPUDeviceSize offset, VGPUDeviceSize size, const void* pData);

    /* Texture */
    VGPUTexture* (*create_texture)(VGPURenderer* driverData, const VGPUTextureInfo* info);
    void (*destroy_texture)(VGPURenderer* driverData, VGPUTexture* handle);

    /* Shader */
    vgpu_shader (*create_shader)(VGPURenderer* driverData, const vgpu_shader_info* info);
    void (*destroy_shader)(VGPURenderer* driverData, vgpu_shader handle);

    /* Pipeline */
    agpu_pipeline (*create_pipeline)(VGPURenderer* driverData, const vgpu_pipeline_info* info);
    void (*destroy_pipeline)(VGPURenderer* driverData, agpu_pipeline handle);

    /* CommandBuffer */
    void (*cmdBeginRenderPass)(VGPURenderer* driverData, const VGPURenderPassDescriptor* descriptor);
    void (*cmdEndRenderPass)(VGPURenderer* driverData);

    void (*cmdSetPipeline)(VGPURenderer* driverData, agpu_pipeline pipeline);
    void (*cmdSetVertexBuffer)(VGPURenderer* driverData, uint32_t slot, vgpu_buffer* buffer, uint64_t offset);
    void (*cmdSetIndexBuffer)(VGPURenderer* driverData, vgpu_buffer* buffer, uint64_t offset);

    void (*set_uniform_buffer)(VGPURenderer* driverData, uint32_t set, uint32_t binding, vgpu_buffer* handle);
    void (*set_uniform_buffer_data)(VGPURenderer* driverData, uint32_t set, uint32_t binding, const void* data, VGPUDeviceSize size);

    void (*cmdDraw)(VGPURenderer* driverData, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex);
    void (*cmdDrawIndexed)(VGPURenderer* driverData, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex);

} VGPUDeviceImpl;

typedef struct agpu_driver {
    bool (*supported)(void);
    VGPUDeviceImpl* (*create_device)(void);
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

#define ASSIGN_DRIVER_FUNC(func, name) device->func = name##_##func;
#define ASSIGN_DRIVER(name) \
ASSIGN_DRIVER_FUNC(init, name)\
ASSIGN_DRIVER_FUNC(destroy, name)\
ASSIGN_DRIVER_FUNC(frame_wait, name)\
ASSIGN_DRIVER_FUNC(frame_finish, name)\
ASSIGN_DRIVER_FUNC(getBackend, name)\
ASSIGN_DRIVER_FUNC(get_caps, name)\
ASSIGN_DRIVER_FUNC(get_default_depth_format, name)\
ASSIGN_DRIVER_FUNC(get_default_depth_stencil_format, name)\
ASSIGN_DRIVER_FUNC(buffer_create, name)\
ASSIGN_DRIVER_FUNC(buffer_destroy, name)\
ASSIGN_DRIVER_FUNC(buffer_sub_data, name)\
ASSIGN_DRIVER_FUNC(create_texture, name)\
ASSIGN_DRIVER_FUNC(destroy_texture, name)\
ASSIGN_DRIVER_FUNC(create_shader, name)\
ASSIGN_DRIVER_FUNC(destroy_shader, name)\
ASSIGN_DRIVER_FUNC(create_pipeline, name)\
ASSIGN_DRIVER_FUNC(destroy_pipeline, name)\
ASSIGN_DRIVER_FUNC(cmdBeginRenderPass, name)\
ASSIGN_DRIVER_FUNC(cmdEndRenderPass, name)\
ASSIGN_DRIVER_FUNC(cmdSetPipeline, name)\
ASSIGN_DRIVER_FUNC(cmdSetVertexBuffer, name)\
ASSIGN_DRIVER_FUNC(cmdSetIndexBuffer, name)\
ASSIGN_DRIVER_FUNC(set_uniform_buffer, name)\
ASSIGN_DRIVER_FUNC(set_uniform_buffer_data, name)\
ASSIGN_DRIVER_FUNC(cmdDraw, name)\
ASSIGN_DRIVER_FUNC(cmdDrawIndexed, name)
