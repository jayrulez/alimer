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
#include <new>

#ifndef VGPU_ALLOCA
#   include <malloc.h>
#   if defined(_MSC_VER) || defined(__MINGW32__)
#       define VGPU_ALLOCA(type, count) ((type*)(_malloca(sizeof(type) * (count))))
#   else
#       define VGPU_ALLOCA(type, count) ((type*)(alloca(sizeof(type) * (count))))
#   endif
#endif

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

namespace vgpu {
    template <typename T, uint32_t MAX_COUNT>
    class Pool
    {
    public:
        Pool()
            : mem{}
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

        void dealloc(uint32_t index)
        {
            values[index].~T();
            new (&values[index]) int(first_free);
            first_free = index;
        }

        alignas(T) uint8_t mem[sizeof(T) * (MAX_COUNT + 1)];
        T* values;
        int first_free;

        T& operator[](int index) { return values[index]; }
        bool isFull() const { return first_free == -1; }
    };
}


using namespace vgpu;

typedef struct vgpu_context {
    bool (*init)(const PresentationParameters& presentationParameters, InitFlags flags);
    void (*shutdown)(void);
    const Caps* (*getCaps)(void);
    bool (*frame_begin)(void);
    void (*frame_end)(void);

    /* Texture */
    TextureHandle(*createTexture)(const TextureDesc& desc);
    void(*destroyTexture)(TextureHandle handle);
    uint32_t(*texture_get_width)(TextureHandle handle, uint32_t mipLevel);
    uint32_t(*texture_get_height)(TextureHandle handle, uint32_t mipLevel);

    /* Framebuffer */
    vgpu_framebuffer(*framebuffer_create)(const vgpu_framebuffer_info* info);
    void(*framebuffer_destroy)(vgpu_framebuffer handle);
    vgpu_framebuffer(*getDefaultFramebuffer)(void);

    /* Buffer */
    BufferHandle (*createBuffer)(const BufferDesc& desc, const void* pInitData);
    void(*destroyBuffer)(BufferHandle handle);

    /* CommandBuffer */
    void (*insertDebugMarker)(const char* name, CommandList commandList);
    void (*pushDebugGroup)(const char* name, CommandList commandList);
    void (*popDebugGroup)(CommandList commandList);
    void(*beginRenderPass)(const RenderPassDesc* desc, CommandList commandList);
    void(*endRenderPass)(CommandList commandList);

} vgpu_context;

struct vgpu_driver {
    BackendType backendType;
    bool(*isSupported)(void);
    vgpu_context* (*create_context)(void);
};

extern vgpu_driver d3d11_driver;
extern vgpu_driver d3d12_driver;
extern vgpu_driver vulkan_driver;
extern vgpu_driver gl_driver;

#endif /* VGPU_DRIVER_H */
