#pragma once

#include "engine/array.h"
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

namespace Alimer
{
    namespace gpu
    {
        template <typename T, uint32_t MAX_COUNT>
        struct Pool
        {
            void init()
            {
                values = (T*)mem;
                for (int i = 0; i < MAX_COUNT; ++i) {
                    new (NewPlaceholder(), &values[i]) int(i + 1);
                }
                new (NewPlaceholder(), &values[MAX_COUNT - 1]) int(-1);
                first_free = 0;
            }

            int alloc()
            {
                if (first_free == -1) return -1;

                const int id = first_free;
                first_free = *((int*)&values[id]);
                new (NewPlaceholder(), &values[id]) T;
                return id;
            }

            void dealloc(uint32_t idx)
            {
                values[idx].~T();
                new (NewPlaceholder(), &values[idx]) int(first_free);
                first_free = idx;
            }

            alignas(T) uint8_t mem[sizeof(T) * MAX_COUNT];
            T* values;
            int first_free;

            T& operator[](int index) { return values[index]; }
            bool isFull() const { return first_free == -1; }
        };


        struct Renderer
        {
            bool (*init)(const Configuration& config, IAllocator& allocator);
            void (*shutdown)(void);
        };
    }
}
