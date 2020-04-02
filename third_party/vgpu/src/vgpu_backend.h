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

#pragma once

#include <vgpu/vgpu.h>
#include <string.h>

#if defined(_MSC_VER) || defined(__MINGW32__)
#   include <malloc.h>
#else
#   include <alloca.h>
#endif

#ifndef __cplusplus
#  define nullptr ((void*)0)
#endif

#ifndef _AGPU_PRIVATE
#   if defined(__GNUC__)
#       define _AGPU_PRIVATE __attribute__((unused)) static
#   else
#       define _AGPU_PRIVATE static
#   endif
#endif

#ifndef VGPU_ASSERT
#   include <assert.h>
#   define VGPU_ASSERT(c) assert(c)
#endif

#ifndef AGPU_MALLOC
#   include <stdlib.h>
#   define VGPU_MALLOC(s) malloc(s)
#   define VGPU_FREE(p) free(p)
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
#   define _vgpu_alloca(type, count) ((type*)(_alloca(sizeof(type) * (count))))
#else
#   define _vgpu_alloca(type, count) ((type*)(alloca(sizeof(type) * (count))))
#endif

#define _vgpu_def(val, def) (((val) == 0) ? (def) : (val))
#define _vgpu_min(a,b) ((a<b)?a:b)
#define _vgpu_max(a,b) ((a>b)?a:b)

#define VGPU_ALLOC_HANDLE(type) ((type*)VGPU_MALLOC(sizeof(type)))
#define VGPU_ALLOCN(type, count) ((type*)VGPU_MALLOC(sizeof(type) * count))
#define GPU_UNUSED(x) do { (void)sizeof(x); } while(0)
#define GPU_THROW(s) vgpu_log(s, VGPU_LOG_LEVEL_ERROR);
#define GPU_CHECK(c, s) if (!(c)) { GPU_THROW(s); }

typedef struct agpu_renderer agpu_renderer;

typedef struct agpu_device {
    /* Opaque pointer for the renderer. */
    agpu_renderer* renderer;

    void (*destroy_device)(agpu_device* device);
    void (*wait_idle)(agpu_renderer* renderer);
    void (*begin_frame)(agpu_renderer* renderer);
    void (*end_frame)(agpu_renderer* renderer);

    vgpu_context* (*create_context)(agpu_renderer* driver_data, const vgpu_context_desc* desc);
    void (*destroy_context)(agpu_renderer* driver_data, vgpu_context* context);
    void (*set_context)(agpu_renderer* driver_data, vgpu_context* context);

    vgpu_backend(*query_backend)();
    agpu_features (*query_features)();
    agpu_limits (*query_limits)();

    vgpu_buffer* (*create_buffer)(agpu_renderer* renderer, const vgpu_buffer_desc* desc);
    void (*destroy_buffer)(agpu_renderer* renderer, vgpu_buffer* buffer);
} agpu_device;


typedef struct agpu_driver {
    vgpu_backend backend;
    agpu_device* (*create_device)(const char* application_name, const agpu_desc* desc);
} agpu_driver;

extern agpu_driver vulkan_driver;

#define ASSIGN_DRIVER_FUNC(func, name) device->func = name##_##func;
#define ASSIGN_DRIVER(name)\
ASSIGN_DRIVER_FUNC(destroy_device, name)\
ASSIGN_DRIVER_FUNC(wait_idle, name)\
ASSIGN_DRIVER_FUNC(begin_frame, name)\
ASSIGN_DRIVER_FUNC(end_frame, name)\
ASSIGN_DRIVER_FUNC(create_context, name)\
ASSIGN_DRIVER_FUNC(destroy_context, name)\
ASSIGN_DRIVER_FUNC(set_context, name)\
ASSIGN_DRIVER_FUNC(create_buffer, name)\
ASSIGN_DRIVER_FUNC(destroy_buffer, name)\
ASSIGN_DRIVER_FUNC(query_backend, name)\
ASSIGN_DRIVER_FUNC(query_features, name)\
ASSIGN_DRIVER_FUNC(query_limits, name)

