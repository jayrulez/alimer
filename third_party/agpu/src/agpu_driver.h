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
    bool (*init)(const char* app_name, const agpu_config* config);
    void (*shutdown)(void);
    bool(*frame_begin)(void);
    void(*frame_finish)(void);

    void (*query_caps)(agpu_caps* caps);

    agpu_buffer(*buffer_create)(const agpu_buffer_info* info);
    void(*buffer_destroy)(agpu_buffer handle);

    agpu_shader(*shader_create)(const agpu_shader_info* info);
    void(*shader_destroy)(agpu_shader handle);

    agpu_texture(*texture_create)(const agpu_texture_info* info);
    void(*texture_destroy)(agpu_texture handle);

    agpu_pipeline(*pipeline_create)(const agpu_pipeline_info* info);
    void(*pipeline_destroy)(agpu_pipeline handle);

    /* Commands */
    void(*push_debug_group)(const char* name);
    void(*pop_debug_group)(void) = 0;
    void(*insert_debug_marker)(const char* name);
    void(*begin_render_pass)(const agpu_render_pass_info* info);
    void(*end_render_pass)(void);
    void(*bind_pipeline)(agpu_pipeline handle);
    void(*draw)(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex);
};

#define ASSIGN_DRIVER_FUNC(func, name) renderer.func = name##_##func;
#define ASSIGN_DRIVER(name) \
    ASSIGN_DRIVER_FUNC(init, name)\
    ASSIGN_DRIVER_FUNC(shutdown, name)\
    ASSIGN_DRIVER_FUNC(frame_begin, name)\
    ASSIGN_DRIVER_FUNC(frame_finish, name)\
    ASSIGN_DRIVER_FUNC(query_caps, name)\
    ASSIGN_DRIVER_FUNC(buffer_create, name)\
    ASSIGN_DRIVER_FUNC(buffer_destroy, name)\
    ASSIGN_DRIVER_FUNC(shader_create, name)\
    ASSIGN_DRIVER_FUNC(shader_destroy, name)\
    ASSIGN_DRIVER_FUNC(texture_create, name)\
    ASSIGN_DRIVER_FUNC(texture_destroy, name)\
    ASSIGN_DRIVER_FUNC(pipeline_create, name)\
    ASSIGN_DRIVER_FUNC(pipeline_destroy, name)\
    ASSIGN_DRIVER_FUNC(push_debug_group, name)\
    ASSIGN_DRIVER_FUNC(pop_debug_group, name)\
    ASSIGN_DRIVER_FUNC(insert_debug_marker, name)\
    ASSIGN_DRIVER_FUNC(begin_render_pass, name)\
    ASSIGN_DRIVER_FUNC(end_render_pass, name) \
    ASSIGN_DRIVER_FUNC(bind_pipeline, name) \
    ASSIGN_DRIVER_FUNC(draw, name) \

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
