#pragma once

#include "gpu.h"

#include <string.h> /* memset */
#include <float.h>  /* FLT_MAX */

#define _gpu_def(val, def) (((val) == 0) ? (def) : (val))
#define _gpu_def_flt(val, def) (((val) == 0.0f) ? (def) : (val))
#define _gpu_min(a,b) ((a<b)?a:b)
#define _gpu_max(a,b) ((a>b)?a:b)
#define _gpu_clamp(v,v0,v1) ((v<v0)?(v0):((v>v1)?(v1):(v)))

#ifndef GPU_ASSERT
#   include <assert.h>
#   define GPU_ASSERT(c) assert(c)
#endif

#ifndef GPU_MALLOC
#   include <stdlib.h>
#   define GPU_MALLOC(s) malloc(s)
#   define GPU_FREE(p) free(p)
#endif

#ifndef GPU_STACK_ALLOC
#include <malloc.h>
#if defined(_MSC_VER)
#define GPU_STACK_ALLOC(T, N) (T*)_malloca(N * sizeof(T));
#else
#define GPU_STACK_ALLOC(T, N) (T*)alloca(N * sizeof(T));
#endif
#endif

#if defined(__GNUC__) || defined(__clang__)
#   if defined(__i386__) || defined(__x86_64__)
#       define GPU_BREAKPOINT() __asm__ __volatile__("int $3\n\t")
#   else
#       define GPU_BREAKPOINT() ((void)0)
#   endif
#   define GPU_UNREACHABLE() __builtin_unreachable()

#elif defined(_MSC_VER)
extern void __cdecl __debugbreak(void);
#   define GPU_BREAKPOINT() __debugbreak()
#   define GPU_UNREACHABLE() __assume(false)
#else
#   error "Unsupported compiler"
#endif

#define GPU_COUNTOF(x) (sizeof(x) / sizeof(x[0]))

typedef struct GPUBackendContext GPUBackendContext;
typedef struct GPU_Renderer GPU_Renderer;

struct GPUContext {
    GPUDevice* device;
    GPUBackendContext* backend;
};

struct GPUDevice {
    void (*Destroy)(GPUDevice *device);

    void (*BeginFrame)(GPU_Renderer* driverData);
    void (*EndFrame)(GPU_Renderer* driverData);

    gpu_features (*GetFeatures)(GPU_Renderer* driverData);
    gpu_limits(*GetLimits)(GPU_Renderer* driverData);

    GPUBackendContext* (*CreateContext)(GPU_Renderer* driverData, const GPUSwapChainDescriptor* descriptor);
    void (*DestroyContext)(GPU_Renderer* driverData, GPUBackendContext* handle);
    bool(*ResizeContext)(GPU_Renderer* driverData, GPUBackendContext* handle, uint32_t width, uint32_t height);

    GPU_Renderer* driverData;
};

typedef struct GPUDriver
{
    GPUBackendType backendType;
    bool (*IsSupported)(void);
    void (*GetDrawableSize)(void* window, uint32_t* width, uint32_t* height);
    GPUDevice* (*CreateDevice)(bool debug, const GPUSwapChainDescriptor* descriptor);
} GPUDriver;

#define ASSIGN_DRIVER_FUNC(func, name) device->func = name##_##func;
#define ASSIGN_DRIVER(name) \
ASSIGN_DRIVER_FUNC(Destroy, name)\
ASSIGN_DRIVER_FUNC(BeginFrame, name)\
ASSIGN_DRIVER_FUNC(EndFrame, name)\
ASSIGN_DRIVER_FUNC(GetFeatures, name)\
ASSIGN_DRIVER_FUNC(GetLimits, name)\
ASSIGN_DRIVER_FUNC(CreateContext, name)\
ASSIGN_DRIVER_FUNC(DestroyContext, name)\
ASSIGN_DRIVER_FUNC(ResizeContext, name)

extern GPUDriver VulkanDriver;
extern GPUDriver D3D11Driver;
