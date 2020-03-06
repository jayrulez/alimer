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
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define COUNTOF(x) (sizeof(x) / sizeof(x[0]))
#define _AGPU_ALLOC(type) ((type*) malloc(sizeof(type)))
#define _AGPU_ALLOCN(type, n) ((type*) malloc(sizeof(type) * n))
#define _AGPU_FREE(p) free(p)
#define _GPU_ALLOC_HANDLE(type) ((type##_t*) calloc(1, sizeof(type##_t)))

typedef struct agpu_renderer {
    agpu_backend(*get_backend)(void);
    bool (*initialize)(const agpu_config* config);
    void (*shutdown)(void);
    void (*wait_idle)(void);
    void (*commit_frame)(void);
} agpu_renderer;


bool agpu_gl_supported(void);
agpu_renderer* agpu_create_gl_backend(void);
bool agpu_vk_supported(void);
agpu_renderer* agpu_create_vk_backend(void);
