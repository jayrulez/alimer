//
// Copyright (c) 2020 Amer Koleci.
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

#ifndef GPU_ASSERT
#   include <assert.h>
#   define GPU_ASSERT(c) assert(c)
#endif

#ifndef GPU_MALLOC
#   include <stdlib.h>
#   define GPU_MALLOC(s) malloc(s)
#   define _AGPU_FREE(p) free(p)
#   define _AGPU_ALLOC_HANDLE(T) (T*)calloc(1, sizeof(T))
#endif

#define _AGPU_UNUSED(x) do { (void)sizeof(x); } while(0)
#define _AGPU_COUNTOF(x) (sizeof(x) / sizeof(x[0]))

typedef struct agpu_renderer {
    bool (*init)(const agpu_config* config);
    void (*shutdown)(void);

    void (*log)(agpu_log_level level, const char* msg);

    void (*frame_begin)(void);
    void (*frame_end)(void);

    agpu_buffer(*create_buffer)(const agpu_buffer_info* info);
    void (*destroy_buffer)(agpu_buffer handle);
} agpu_renderer;

typedef struct agpu_driver {
    agpu_backend backend;
    bool (*is_supported)(void);
    agpu_renderer* (*init_renderer)(void);
} agpu_driver;

extern void agpu_log_info(const char* fmt, ...);
extern void agpu_log_warn(const char* fmt, ...);
extern void agpu_log_error(const char* fmt, ...);

extern agpu_driver gl_driver;
