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

#ifndef AGPU_DRIVER_H
#define AGPU_DRIVER_H

#include "agpu.h"

#ifndef AGPU_MALLOC
#   define AGPU_MALLOC(s) malloc(s)
#   define AGPU_FREE(p) free(p)
#endif

#define AGPU_ALLOC(T) ((T*) AGPU_MALLOC(sizeof(T)))

#ifndef AGPU_ASSERT
#   include <assert.h>
#   define AGPU_ASSERT(c) assert(c)
#endif

#define AGPU_UNUSED(x) do { (void)sizeof(x); } while(0)

#define AGPU_DEF(val, def) (((val) == 0) ? (def) : (val))
#define AGPU_DEF_FLOAT(val, def) (((val) == 0.0f) ? (def) : (val))
#define AGPU_MIN(a,b) ((a<b)?a:b)
#define AGPU_MAX(a,b) ((a>b)?a:b)
#define AGPU_CLAMP(v,v0,v1) ((v<v0)?(v0):((v>v1)?(v1):(v)))
#define AGPU_COUNT_OF(x) (sizeof(x) / sizeof(x[0]))

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

typedef struct AGPU_Renderer AGPU_Renderer;

struct AGPUDevice {
    void (*DestroyDevice)(AGPUDevice* device);

    /* Opaque pointer for the Driver */
    AGPU_Renderer* driverData;
};

#define ASSIGN_DRIVER_FUNC(func, name) result->func = name##_##func;
#define ASSIGN_DRIVER(name) \
	ASSIGN_DRIVER_FUNC(DestroyDevice, name)

typedef struct AGPU_Driver
{
    AGPUBackendType backendType;
    bool (*IsSupported)(void);
    AGPUDevice* (*CreateDevice)(void);
} AGPU_Driver;

extern AGPU_Driver D3D11Driver;
extern AGPU_Driver VulkanDriver;
extern AGPU_Driver MetalDriver;
extern AGPU_Driver OpenGLDriver;

#endif /* AGPU_DRIVER_H */
