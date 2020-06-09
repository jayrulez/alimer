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
#include <atomic>

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

template <typename T, uint32_t MAX_COUNT>
struct VGPUPool
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

class VGPUSpinLock
{
private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
public:
    void lock()
    {
        while (!try_lock()) {}
    }
    bool try_lock()
    {
        return !flag.test_and_set(std::memory_order_acquire);
    }

    void unlock()
    {
        flag.clear(std::memory_order_release);
    }
};

// Fixed size very simple thread safe ring buffer
template <typename T, size_t capacity>
class VGPUThreadSafeRingBuffer
{
public:
    // Push an item to the end if there is free space
    //	Returns true if succesful
    //	Returns false if there is not enough space
    inline bool push_back(const T& item)
    {
        bool result = false;
        lock.lock();
        size_t next = (head + 1) % capacity;
        if (next != tail)
        {
            data[head] = item;
            head = next;
            result = true;
        }
        lock.unlock();
        return result;
    }

    // Get an item if there are any
    //	Returns true if succesful
    //	Returns false if there are no items
    inline bool pop_front(T& item)
    {
        bool result = false;
        lock.lock();
        if (tail != head)
        {
            item = data[tail];
            tail = (tail + 1) % capacity;
            result = true;
        }
        lock.unlock();
        return result;
    }

private:
    T data[capacity];
    size_t head = 0;
    size_t tail = 0;
    VGPUSpinLock lock;
};

typedef struct VGPUGraphicsContext {
    bool (*init)(const VGPUDeviceDescription* desc);
    void (*shutdown)(void);
    bool (*beginFrame)(void);
    void (*endFrame)(void);

    /* Texture */
    vgpu_texture(*texture_create)(const vgpu_texture_info* desc);
    void(*texture_destroy)(vgpu_texture handle);
    uint32_t(*texture_get_width)(vgpu_texture handle, uint32_t mipLevel);
    uint32_t(*texture_get_height)(vgpu_texture handle, uint32_t mipLevel);

    /* Buffer */
    vgpu_buffer(*buffer_create)(const vgpu_buffer_info* info);
    void(*buffer_destroy)(vgpu_buffer handle);

    /* Framebuffer */
    vgpu_framebuffer(*framebuffer_create)(const VGPUFramebufferDescription* desc);
    vgpu_framebuffer(*framebuffer_create_from_window)(const vgpu_swapchain_info* info);
    void(*framebuffer_destroy)(vgpu_framebuffer handle);
    vgpu_framebuffer(*getDefaultFramebuffer)(void);

    /* CommandBuffer */
    VGPUCommandBuffer(*beginCommandBuffer)(const char* name, bool profile);
    void (*insertDebugMarker)(VGPUCommandBuffer commandBuffer, const char* name);
    void (*pushDebugGroup)(VGPUCommandBuffer commandBuffer, const char* name);
    void (*popDebugGroup)(VGPUCommandBuffer commandBuffer);
    void(*beginRenderPass)(VGPUCommandBuffer commandBuffer, const VGPURenderPassBeginDescription* beginDesc);
    void(*endRenderPass)(VGPUCommandBuffer commandBuffer);

} VGPUGraphicsContext;

#define ASSIGN_DRIVER_FUNC(func, name) graphicsContext.func = name##_##func;
#define ASSIGN_DRIVER(name) \
    ASSIGN_DRIVER_FUNC(init, name)\
	ASSIGN_DRIVER_FUNC(shutdown, name)\
    ASSIGN_DRIVER_FUNC(beginFrame, name)\
    ASSIGN_DRIVER_FUNC(endFrame, name)\
    ASSIGN_DRIVER_FUNC(texture_create, name)\
    ASSIGN_DRIVER_FUNC(texture_destroy, name)\
    ASSIGN_DRIVER_FUNC(texture_get_width, name)\
    ASSIGN_DRIVER_FUNC(texture_get_height, name)\
    ASSIGN_DRIVER_FUNC(buffer_create, name)\
    ASSIGN_DRIVER_FUNC(buffer_destroy, name)\
    ASSIGN_DRIVER_FUNC(framebuffer_create, name)\
    ASSIGN_DRIVER_FUNC(framebuffer_create_from_window, name)\
    ASSIGN_DRIVER_FUNC(framebuffer_destroy, name)\
    ASSIGN_DRIVER_FUNC(getDefaultFramebuffer, name)\
    ASSIGN_DRIVER_FUNC(beginCommandBuffer, name)\
    ASSIGN_DRIVER_FUNC(insertDebugMarker, name)\
    ASSIGN_DRIVER_FUNC(pushDebugGroup, name)\
    ASSIGN_DRIVER_FUNC(popDebugGroup, name)\
    ASSIGN_DRIVER_FUNC(beginRenderPass, name)\
    ASSIGN_DRIVER_FUNC(endRenderPass, name)

typedef struct vgpu_driver {
    VGPUBackendType backendType;
    bool(*isSupported)(void);
    VGPUGraphicsContext* (*createContext)(void);
} vgpu_driver;

extern vgpu_driver d3d11_driver;
extern vgpu_driver d3d12_driver;
extern vgpu_driver vulkan_driver;
extern vgpu_driver gl_driver;

#endif /* VGPU_DRIVER_H */
