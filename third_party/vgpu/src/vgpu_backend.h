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

#pragma once

#include "vgpu/vgpu.h"
#include <string.h> /* memset */
#include <new>

#if defined(_MSC_VER) || defined(__MINGW32__)
#   include <malloc.h>
#else
#   include <alloca.h>
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

#define VGPU_CHECK(c, s) if (!(c)) { vgpu_log(VGPU_LOG_LEVEL_ERROR, s); _VGPU_BREAKPOINT(); }

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

namespace vgpu
{
    template <typename T, uint32_t MAX_COUNT>
    struct Pool
    {
        void init()
        {
            values = (T*)mem;
            for (int i = 0; i < MAX_COUNT + 1; ++i) {
                new (&values[i]) int(i + 1);
            }
            new (&values[MAX_COUNT]) int(-1);
            first_free = 1;
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

        alignas(T) uint8_t mem[sizeof(T) * (MAX_COUNT + 1)];
        T* values;
        int first_free;

        T& operator[](int idx) { return values[idx]; }
        bool isFull() const { return first_free == -1; }
    };

    using Hash = uint64_t;

    class Hasher
    {
    public:
        explicit Hasher(Hash h_)
            : h(h_)
        {
        }

        Hasher() = default;

        template <typename T>
        inline void data(const T* data_, size_t size)
        {
            size /= sizeof(*data_);
            for (size_t i = 0; i < size; i++)
                h = (h * 0x100000001b3ull) ^ data_[i];
        }

        inline void u32(uint32_t value)
        {
            h = (h * 0x100000001b3ull) ^ value;
        }

        inline void s32(int32_t value)
        {
            u32(uint32_t(value));
        }

        inline void f32(float value)
        {
            union
            {
                float f32;
                uint32_t u32;
            } u;
            u.f32 = value;
            u32(u.u32);
        }

        inline void u64(uint64_t value)
        {
            u32(value & 0xffffffffu);
            u32(value >> 32);
        }

        template <typename T>
        inline void pointer(T* ptr)
        {
            u64(reinterpret_cast<uintptr_t>(ptr));
        }

        inline void string(const char* str)
        {
            char c;
            u32(0xff);
            while ((c = *str++) != '\0')
                u32(uint8_t(c));
        }

        inline Hash get() const
        {
            return h;
        }

    private:
        Hash h = 0xcbf29ce484222325ull;
    };
}

struct vgpu_renderer {
    bool (*init)(const vgpu_config* config);
    void (*destroy)(void);
    vgpu_backend_type(*get_backend)(void);
    vgpu_caps(*get_caps)(void);
    VGPUTextureFormat(*GetDefaultDepthFormat)(void);
    VGPUTextureFormat(*GetDefaultDepthStencilFormat)(void);

    void (*begin_frame)(void);
    void (*end_frame)(void);

    /* Texture */
    VGPUTexture(*create_texture)(const VGPUTextureDescriptor* desc);
    void (*destroy_texture)(VGPUTexture handle);

    /* Buffer */
    VGPUBuffer(*bufferCreate)(const VGPUBufferDescriptor* desc);
    void (*bufferDestroy)(VGPUBuffer handle);

    /* Sampler */
    VGPUSampler(*samplerCreate)(const VGPUSamplerDescriptor* desc);
    void (*samplerDestroy)(VGPUSampler handle);

    /* Commands */
    void (*cmdBeginRenderPass)(const VGPURenderPassDescriptor* descriptor);
    void (*cmdEndRenderPass)(void);
};

/* d3d11 */
extern bool vgpu_d3d11_supported(void);
extern vgpu_renderer* vgpu_d3d11_create_device(void);

/* d3d12 */
extern bool vgpu_d3d12_supported(void);
extern vgpu_renderer* vgpu_d3d12_create_device(void);

/* vulkan */
extern bool vgpu_vk_supported(void);
extern vgpu_renderer* vgpu_vk_create_device(void);

/* opengl */
extern bool vgpu_opengl_supported(void);
extern vgpu_renderer* vgpu_opengl_create_device(void);

#define ASSIGN_DRIVER_FUNC(func, name) renderer.func = name##_##func;
#define ASSIGN_DRIVER(name) \
ASSIGN_DRIVER_FUNC(init, name)\
ASSIGN_DRIVER_FUNC(destroy, name)\
ASSIGN_DRIVER_FUNC(get_backend, name)\
ASSIGN_DRIVER_FUNC(get_caps, name)\
ASSIGN_DRIVER_FUNC(GetDefaultDepthFormat, name)\
ASSIGN_DRIVER_FUNC(GetDefaultDepthStencilFormat, name)\
ASSIGN_DRIVER_FUNC(begin_frame, name)\
ASSIGN_DRIVER_FUNC(end_frame, name)\
ASSIGN_DRIVER_FUNC(create_texture, name)\
ASSIGN_DRIVER_FUNC(destroy_texture, name)
