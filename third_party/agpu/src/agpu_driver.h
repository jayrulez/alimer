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
#include <new>
#include <stdlib.h>
#include <string.h> 
#include <float.h> 

#ifndef AGPU_ASSERT
#   include <assert.h>
#   define AGPU_ASSERT(c) assert(c)
#endif

#define AGPU_UNUSED(x) do { (void)sizeof(x); } while(0)

#define _agpu_def(val, def) (((val) == 0) ? (def) : (val))
#define AGPU_DEF_FLOAT(val, def) (((val) == 0.0f) ? (def) : (val))
#define AGPU_MIN(a,b) ((a<b)?a:b)
#define AGPU_MAX(a,b) ((a>b)?a:b)
#define AGPU_CLAMP(v,v0,v1) ((v<v0)?(v0):((v>v1)?(v1):(v)))
#define AGPU_COUNT_OF(x) (sizeof(x) / sizeof(x[0]))

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

namespace agpu
{
    static constexpr uint16_t kInvalidPoolId = static_cast<uint16_t>(-1);

    template <typename T, uint16_t MAX_COUNT>
    struct Pool
    {
        void init()
        {
            values = (T*)mem;
            for (uint16_t i = 0; i < MAX_COUNT; ++i) {
                new (&values[i]) uint16_t(i + 1);
            }
            new (&values[MAX_COUNT - 1]) uint16_t(-1);
            first_free = 0;
        }

        uint16_t alloc()
        {
            if (first_free == kInvalidPoolId) {
                return kInvalidPoolId;
            }

            const uint16_t id = first_free;
            first_free = *((int*)&values[id]);
            new (&values[id]) T;
            return id;
        }

        void dealloc(uint16_t index)
        {
            values[index].~T();
            new (&values[index]) int(first_free);
            first_free = index;
        }

        alignas(T) uint8_t mem[sizeof(T) * MAX_COUNT];
        T* values;
        uint16_t first_free;

        T& operator[](uint16_t index) { return values[index]; }
        bool isFull() const { return first_free == kInvalidPoolId; }
    };

    struct Renderer {
        bool (*init)(InitFlags flags, void* windowHandle);
        void (*Shutdown)(void);

        Swapchain(*GetPrimarySwapchain)(void);
        FrameOpResult(*BeginFrame)(Swapchain handle);
        FrameOpResult(*EndFrame)(Swapchain handle, EndFrameFlags flags);

        const Caps* (*QueryCaps)(void);

        Swapchain(*CreateSwapchain)(void* windowHandle);
        void(*DestroySwapchain)(Swapchain handle);
        Texture(*GetCurrentTexture)(Swapchain handle);

        BufferHandle(*CreateBuffer)(uint32_t count, uint32_t stride, const void* initialData);
        void(*DestroyBuffer)(BufferHandle handle);

        Texture(*CreateTexture)(uint32_t width, uint32_t height, PixelFormat format, uint32_t mipLevels, intptr_t handle);
        void(*DestroyTexture)(Texture handle);

        /* Commands */
        void (*PushDebugGroup)(const char* name);
        void (*PopDebugGroup)(void) = 0;
        void (*InsertDebugMarker)(const char* name);
        void(*BeginRenderPass)(const RenderPassDescription* renderPass);
        void(*EndRenderPass)(void);
    };

#define ASSIGN_DRIVER_FUNC(func, name) renderer.func = name##_##func;
#define ASSIGN_DRIVER(name) \
    ASSIGN_DRIVER_FUNC(init, name)\
    ASSIGN_DRIVER_FUNC(Shutdown, name)\
	ASSIGN_DRIVER_FUNC(GetPrimarySwapchain, name)\
    ASSIGN_DRIVER_FUNC(BeginFrame, name)\
    ASSIGN_DRIVER_FUNC(BeginFrame, name)\
    ASSIGN_DRIVER_FUNC(EndFrame, name)\
    ASSIGN_DRIVER_FUNC(QueryCaps, name)\
    ASSIGN_DRIVER_FUNC(CreateSwapchain, name)\
    ASSIGN_DRIVER_FUNC(DestroySwapchain, name)\
    ASSIGN_DRIVER_FUNC(GetCurrentTexture, name)\
    ASSIGN_DRIVER_FUNC(CreateBuffer, name)\
    ASSIGN_DRIVER_FUNC(DestroyBuffer, name)\
    ASSIGN_DRIVER_FUNC(CreateTexture, name)\
    ASSIGN_DRIVER_FUNC(DestroyTexture, name)\
    ASSIGN_DRIVER_FUNC(PushDebugGroup, name)\
    ASSIGN_DRIVER_FUNC(PopDebugGroup, name)\
    ASSIGN_DRIVER_FUNC(InsertDebugMarker, name)\
    ASSIGN_DRIVER_FUNC(BeginRenderPass, name)\
    ASSIGN_DRIVER_FUNC(EndRenderPass, name)

    struct Driver
    {
        BackendType backend;
        bool (*isSupported)(void);
        Renderer* (*createRenderer)(void);
    };

    extern Driver D3D12_Driver;
    extern Driver D3D11_Driver;
    extern Driver vulkan_driver;
    extern Driver metal_driver;
    extern Driver GL_Driver;
}


#endif /* AGPU_DRIVER_H */
