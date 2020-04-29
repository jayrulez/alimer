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

#ifndef __cplusplus
#  define nullptr ((void*)0)
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
#define GPU_VOIDP_TO_U64(x) (((union { uint64_t u; void* p; }) { .p = x }).u)

typedef struct gpu_renderer gpu_renderer;

typedef struct GPUDeviceImpl {
    /* Opaque pointer for the renderer. */
    gpu_renderer* renderer;

    void (*destroyDevice)(GPUDevice device);
    GPUDeviceCapabilities(*query_caps)(gpu_renderer* driverData);
    VGPURenderPass (*get_default_render_pass)(gpu_renderer* driverData);

    GPUTextureFormat(*getPreferredSwapChainFormat)(gpu_renderer* driverData, GPUSurface surface);
    GPUTextureFormat(*getDefaultDepthFormat)(gpu_renderer* driverData);
    GPUTextureFormat(*getDefaultDepthStencilFormat)(gpu_renderer* driverData);

    void (*wait_idle)(gpu_renderer* driverData);
    void (*begin_frame)(gpu_renderer* driverData);
    void (*end_frame)(gpu_renderer* driverData);

    GPUSwapChain(*createSwapChain)(gpu_renderer* driverData, GPUSurface surface, const GPUSwapChainDescriptor* desc);
    void (*destroySwapChain)(gpu_renderer* driverData, GPUSwapChain handle);

#if TODO
    /* Buffer */
    vgpu_buffer(*create_buffer)(VGPURenderer* driverData, const vgpu_buffer_desc* desc);
    void (*destroy_buffer)(VGPURenderer* driverData, vgpu_buffer handle);

    /* Texture */
    vgpu_texture(*create_texture)(VGPURenderer* driverData, const vgpu_texture_desc* desc);
    void (*destroy_texture)(VGPURenderer* driverData, vgpu_texture handle);
    vgpu_texture_desc(*query_texture_desc)(vgpu_texture handle);

    /* Sampler */
    vgpu_sampler(*samplerCreate)(VGPURenderer* driver_data, const vgpu_sampler_desc* desc);
    void (*samplerDestroy)(VGPURenderer* driver_data, vgpu_sampler handle);

    /* RenderPass */
    VGPURenderPass(*renderPassCreate)(VGPURenderer* driver_data, const VGPURenderPassDescriptor* descriptor);
    void (*renderPassDestroy)(VGPURenderer* driver_data, VGPURenderPass handle);
    void (*renderPassGetExtent)(VGPURenderer* driver_data, VGPURenderPass handle, uint32_t* width, uint32_t* height);
    void (*render_pass_set_color_clear_value)(VGPURenderPass handle, uint32_t attachment_index, const float colorRGBA[4]);
    void (*render_pass_set_depth_stencil_clear_value)(VGPURenderPass handle, float depth, uint8_t stencil);

    /* Shader */
    vgpu_shader(*create_shader)(VGPURenderer* driver_data, const vgpu_shader_desc* desc);
    void (*destroy_shader)(VGPURenderer* driver_data, vgpu_shader handle);

    /* Pipeline */
    vgpu_pipeline(*create_render_pipeline)(VGPURenderer* driver_data, const vgpu_render_pipeline_desc* desc);
    vgpu_pipeline(*create_compute_pipeline)(VGPURenderer* driver_data, const VgpuComputePipelineDescriptor* desc);
    void (*destroy_pipeline)(VGPURenderer* driver_data, vgpu_pipeline handle);

    /* Commands */
    void (*cmdBeginRenderPass)(VGPURenderer* driver_data, VGPURenderPass handle);
    void (*cmdEndRenderPass)(VGPURenderer* driver_data);
#endif // TODO

} GPUDeviceImpl;

typedef struct gpu_backend_surface gpu_backend_surface;

typedef struct GPUSurfaceImpl {
    uint64_t surface;
#if defined(GPU_D3D12_BACKEND)
    gpu_backend_surface* d3d12;
#endif

#if defined(GPU_D3D11_BACKEND)
    gpu_backend_surface* d3d11;
#endif
} GPUSurfaceImpl;

typedef struct gpu_driver {
    bool (*supported)(void);
    bool (*init)(const GPUInitConfig* config);
    void (*shutdown)(void);

    gpu_backend_surface* (*create_surface_from_windows_hwnd)(void* hinstance, void* hwnd);
    GPUDevice (*createDevice)(const GPUDeviceDescriptor* desc);
} gpu_driver;


#if defined(GPU_D3D11_BACKEND)
extern gpu_driver d3d11_driver;
#endif

#if defined(GPU_D3D12_BACKEND)
extern gpu_driver d3d12_driver;
#endif

#if defined(GPU_VK_BACKEND) && TODO_VK
extern gpu_driver vulkan_driver;
extern bool gpu_vk_supported(void);
extern gpu_renderer* vk_gpu_create_renderer(void);
#endif

#define ASSIGN_DRIVER_FUNC(func, name) device->func = name##_##func;
#define ASSIGN_DRIVER(name) \
ASSIGN_DRIVER_FUNC(destroyDevice, name)\
ASSIGN_DRIVER_FUNC(query_caps, name)\
ASSIGN_DRIVER_FUNC(get_default_render_pass, name)\
ASSIGN_DRIVER_FUNC(getPreferredSwapChainFormat, name)\
ASSIGN_DRIVER_FUNC(getDefaultDepthFormat, name)\
ASSIGN_DRIVER_FUNC(getDefaultDepthStencilFormat, name)\
ASSIGN_DRIVER_FUNC(wait_idle, name)\
ASSIGN_DRIVER_FUNC(begin_frame, name)\
ASSIGN_DRIVER_FUNC(end_frame, name)\
ASSIGN_DRIVER_FUNC(createSwapChain, name)\
ASSIGN_DRIVER_FUNC(destroySwapChain, name)
