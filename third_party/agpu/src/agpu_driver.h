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
    template<typename T>
    struct Array
    {
        uint32_t Size;
        uint32_t capacity;
        T* data;

        constexpr Array() { Size = capacity = 0; data = nullptr; }

        constexpr Array(const Array<T>& src) { Size = capacity = 0; data = nullptr; operator=(src); }
        inline Array<T>& operator=(const Array<T>& src)
        {
            Clear();
            Resize(src.Size);
            memcpy(data, src.data, (size_t)Size * sizeof(T));
            return *this;
        }

        inline ~Array()
        {
            if (data)
                free(data);
        }

        inline bool     IsEmpty() const { return Size == 0; }
        inline uint32_t MemorySize() const { return Size * (uint32_t)sizeof(T); }
        inline uint32_t Capacity() const { return capacity; }
        inline T& operator[](uint32_t i) { AGPU_ASSERT(i < Size); return data[i]; }
        inline const T& operator[](uint32_t i) const { AGPU_ASSERT(i < Size); return data[i]; }

        inline void Clear()
        {
            if (data) {
                Size = capacity = 0; free(data);
                data = nullptr;
            }
        }

        constexpr uint32_t getGrowCapacity(uint32_t size) const
        {
            uint32_t newCapacity = capacity ? (capacity + capacity / 2) : 8;
            return newCapacity > size ? newCapacity : size;
        }

        inline void Resize(uint32_t newSize)
        {
            if (newSize > capacity)
            {
                Reserve(getGrowCapacity(newSize));
            }

            Size = newSize;
        }

        inline void Reserve(uint32_t newCapacity)
        {
            if (newCapacity <= capacity)
                return;

            T* new_data = (T*)malloc((size_t)newCapacity * sizeof(T));
            if (data) {
                memcpy(new_data, data, (size_t)Size * sizeof(T));
                free(data);
            }

            data = new_data;
            capacity = newCapacity;
        }

        inline void Add(const T& v)
        {
            if (Size == capacity)
            {
                Reserve(getGrowCapacity(Size + 1));
            }

            memcpy(&data[Size], &v, sizeof(v));
            Size++;
        }
    };
}

struct agpu_renderer {
    bool (*init)(agpu_init_flags flags, const agpu_swapchain_info* swapchain_info);
    void (*shutdown)(void);
    bool(*frame_begin)(agpu_swapchain handle);
    void(*frame_finish)(agpu_swapchain handle);

    void (*query_caps)(agpu_caps* caps);

    agpu_swapchain(*create_swapchain)(const agpu_swapchain_info* info);
    void(*destroy_swapchain)(agpu_swapchain handle);
    agpu_swapchain(*get_main_swapchain)(void);
    agpu_texture(*get_current_texture)(agpu_swapchain handle);

    agpu_buffer(*createBuffer)(const agpu_buffer_info* info);
    void(*destroyBuffer)(agpu_buffer handle);

    agpu_texture(*create_texture)(const agpu_texture_info* info);
    void(*destroy_texture)(agpu_texture handle);

    /* Commands */
    void(*PushDebugGroup)(const char* name);
    void(*PopDebugGroup)(void) = 0;
    void(*InsertDebugMarker)(const char* name);
    void(*begin_render_pass)(const agpu_render_pass_info* info);
    void(*end_render_pass)(void);
};

#define ASSIGN_DRIVER_FUNC(func, name) renderer.func = name##_##func;
#define ASSIGN_DRIVER(name) \
    ASSIGN_DRIVER_FUNC(init, name)\
    ASSIGN_DRIVER_FUNC(shutdown, name)\
    ASSIGN_DRIVER_FUNC(frame_begin, name)\
    ASSIGN_DRIVER_FUNC(frame_finish, name)\
    ASSIGN_DRIVER_FUNC(query_caps, name)\
    ASSIGN_DRIVER_FUNC(create_swapchain, name)\
    ASSIGN_DRIVER_FUNC(destroy_swapchain, name)\
    ASSIGN_DRIVER_FUNC(get_main_swapchain, name)\
    ASSIGN_DRIVER_FUNC(get_current_texture, name)\
    ASSIGN_DRIVER_FUNC(createBuffer, name)\
    ASSIGN_DRIVER_FUNC(destroyBuffer, name)\
    ASSIGN_DRIVER_FUNC(create_texture, name)\
    ASSIGN_DRIVER_FUNC(destroy_texture, name)\
    ASSIGN_DRIVER_FUNC(PushDebugGroup, name)\
    ASSIGN_DRIVER_FUNC(PopDebugGroup, name)\
    ASSIGN_DRIVER_FUNC(InsertDebugMarker, name)\
    ASSIGN_DRIVER_FUNC(begin_render_pass, name)\
    ASSIGN_DRIVER_FUNC(end_render_pass, name)

struct agpu_driver
{
    agpu_backend_type backend;
    bool (*is_supported)(void);
    agpu_renderer* (*create_renderer)(void);
};

extern agpu_driver d3d12_driver;
extern agpu_driver D3D11_Driver;
extern agpu_driver vulkan_driver;
extern agpu_driver metal_driver;
extern agpu_driver GL_Driver;

#endif /* AGPU_DRIVER_H */
