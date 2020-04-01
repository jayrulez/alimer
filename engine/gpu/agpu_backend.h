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

#include "agpu.h"
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

#ifndef AGPU_ASSERT
#   include <assert.h>
#   define AGPU_ASSERT(c) assert(c)
#endif

#ifndef AGPU_MALLOC
#   include <stdlib.h>
#   define AGPU_MALLOC(s) malloc(s)
#   define AGPU_FREE(p) free(p)
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
#   define gpu_alloca(type, count) ((type*)(_alloca(sizeof(type) * (count))))
#   define gpu_alloca(type, count) ((type*)(_alloca(sizeof(type) * (count))))
#   define gpu_alloca(type, count) ((type*)(_alloca(sizeof(type) * (count))))
#else
#   define gpu_alloca(type, count) ((type*)(alloca(sizeof(type) * (count))))
#endif

#define _gpu_def(val, def) (((val) == 0) ? (def) : (val))
#define _gpu_min(a,b) ((a<b)?a:b)
#define _gpu_max(a,b) ((a>b)?a:b)
#define GPU_UNUSED(x) do { (void)sizeof(x); } while(0)
#define GPU_THROW(s) agpu_log(s, AGPU_LOG_LEVEL_ERROR);
#define GPU_CHECK(c, s) if (!(c)) { GPU_THROW(s); }

typedef struct agpu_renderer agpu_renderer;

typedef struct agpu_device {
    /* Opaque pointer for the renderer. */
    agpu_renderer* renderer;

    void (*destroy_device)(agpu_device* device);
    void (*wait_idle)(agpu_renderer* renderer);
    void (*begin_frame)(agpu_renderer* renderer);
    void (*end_frame)(agpu_renderer* renderer);
    agpu_backend (*query_backend)();
    agpu_features (*query_features)();
    agpu_limits (*query_limits)();

    agpu_buffer* (*create_buffer)(agpu_renderer* renderer, const agpu_buffer_desc* desc);
    void (*destroy_buffer)(agpu_renderer* renderer, agpu_buffer* buffer);
} agpu_device;


typedef struct agpu_driver {
    agpu_backend backend;
    agpu_device* (*create_device)(const char* application_name, const agpu_desc* desc);
} agpu_driver;

extern agpu_driver vulkan_driver;

#define ASSIGN_DRIVER_FUNC(func, name) device->func = name##_##func;
#define ASSIGN_DRIVER(name) \
ASSIGN_DRIVER_FUNC(destroy_device, name) \
ASSIGN_DRIVER_FUNC(wait_idle, name) \
ASSIGN_DRIVER_FUNC(begin_frame, name) \
ASSIGN_DRIVER_FUNC(end_frame, name) \
ASSIGN_DRIVER_FUNC(create_buffer, name) \
ASSIGN_DRIVER_FUNC(destroy_buffer, name)\
ASSIGN_DRIVER_FUNC(query_backend, name)\
ASSIGN_DRIVER_FUNC(query_features, name)\
ASSIGN_DRIVER_FUNC(query_limits, name)
