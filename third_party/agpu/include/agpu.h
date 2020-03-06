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

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#if AGPU_BUILD_SHARED
#   if defined(_MSC_VER)
#       ifdef VGPU_EXPORTS
#           define _AGPU_API_DECL __declspec(dllexport)
#       else
#           define _AGPU_API_DECL __declspec(dllimport)
#       endif
#   else
#       define _AGPU_API_DECL __attribute__((visibility("default")))
#   endif
#else
#   define _AGPU_API_DECL
#endif

#ifdef __cplusplus
#    define AGPU_API extern "C" _AGPU_API_DECL
#else
#    define AGPU_API extern _AGPU_API_DECL
#endif

typedef struct gpu_buffer_t* gpu_buffer;

typedef enum agpu_backend {
    AGPU_BACKEND_DEFAULT = 0,
    AGPU_BACKEND_NULL,
    AGPU_BACKEND_VULKAN,
    AGPU_BACKEND_DIRECT3D12,
    AGPU_BACKEND_DIRECT3D11,
    AGPU_BACKEND_OPENGL,
    AGPU_BACKEND_COUNT
} agpu_backend;

typedef struct agpu_swapchain_desc {
    uint32_t width;
    uint32_t height;
    void* native_handle;
} agpu_swapchain_desc;

typedef struct gpu_buffer_desc {
    uint64_t size;
    uint32_t usage;
    const char* name;
} gpu_buffer_desc;

typedef struct agpu_config {
    agpu_backend preferred_backend;
    bool debug;
    void* (*get_gl_proc_address)(const char*);
    void (*callback)(void* context, const char* message, int level);
    void* context;
    const agpu_swapchain_desc* swapchain_desc;
} agpu_config;

AGPU_API bool agpu_is_backend_supported(agpu_backend backend);
AGPU_API agpu_backend agpu_get_default_platform_backend();

AGPU_API bool agpu_init(const agpu_config* config);
AGPU_API void agpu_shutdown(void);
AGPU_API void agpu_wait_idle(void);
AGPU_API void agpu_begin_frame(void);
AGPU_API void agpu_end_frame(void);

AGPU_API bool gpu_create_buffer(const gpu_buffer_desc* desc, gpu_buffer* result);
AGPU_API void gpu_destroy_buffer(gpu_buffer buffer);
