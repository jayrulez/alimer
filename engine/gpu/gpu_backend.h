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

#ifdef __cplusplus
#include <new>
template <typename T, uint32_t MAX_COUNT>
struct Pool
{
    void init()
    {
        values = (T*)mem;
        for (int i = 0; i < MAX_COUNT; ++i) {
            new (&values[i]) int(i + 1);
        }
        new (&values[MAX_COUNT - 1]) int(-1);
        first_free = 0;
    }

    int alloc()
    {
        if (first_free == -1) return -1;

        const int id = first_free;
        first_free = *((int*)&values[id]);
        new (&values[id]) T;
        return id;
    }

    void dealloc(uint32_t idx)
    {
        values[idx].~T();
        new (&values[idx]) int(first_free);
        first_free = idx;
    }


    alignas(T) uint8_t mem[sizeof(T) * MAX_COUNT];
    T* values;
    int first_free;

    T& operator[](int idx) { return values[idx]; }
    bool isFull() const { return first_free == -1; }
};
#endif

typedef struct gpu_renderer gpu_renderer;

typedef struct GPUDeviceImpl {
    /* Opaque pointer for the renderer. */
    gpu_renderer* renderer;

    void (*destroyDevice)(GPUDevice device);

    void (*beginFrame)(gpu_renderer* driverData);
    void (*presentFrame)(gpu_renderer* driverData);
    void (*waitForGPU)(gpu_renderer* driverData);

    GPUDeviceCapabilities(*query_caps)(gpu_renderer* driverData);

    //GPUTextureFormat(*getPreferredSwapChainFormat)(gpu_renderer* driverData, GPUSurface surface);
    AGPUPixelFormat(*getDefaultDepthFormat)(gpu_renderer* driverData);
    AGPUPixelFormat(*getDefaultDepthStencilFormat)(gpu_renderer* driverData);

    /* Texture */
    AGPUTexture(*createTexture)(gpu_renderer* driverData, const AGPUTextureDescriptor* descriptor);
    void (*destroyTexture)(gpu_renderer* driverData, AGPUTexture handle);


#if TODO
    /* Buffer */
    vgpu_buffer(*create_buffer)(VGPURenderer* driverData, const vgpu_buffer_desc* desc);
    void (*destroy_buffer)(VGPURenderer* driverData, vgpu_buffer handle);


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

typedef struct gpu_driver {
    bool (*supported)(void);
    GPUDevice(*create_device)(const agpu_device_info* info);
} gpu_driver;


#if defined(GPU_D3D11_BACKEND)
extern gpu_driver d3d11_driver;
#endif

#if defined(GPU_D3D12_BACKEND) && defined(TODO_D3D12)
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
ASSIGN_DRIVER_FUNC(beginFrame, name)\
ASSIGN_DRIVER_FUNC(presentFrame, name)\
ASSIGN_DRIVER_FUNC(waitForGPU, name)\
ASSIGN_DRIVER_FUNC(query_caps, name)\
ASSIGN_DRIVER_FUNC(getDefaultDepthFormat, name)\
ASSIGN_DRIVER_FUNC(getDefaultDepthStencilFormat, name)\
ASSIGN_DRIVER_FUNC(createTexture, name)\
ASSIGN_DRIVER_FUNC(destroyTexture, name)
