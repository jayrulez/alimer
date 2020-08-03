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

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
    /* Handles */
    typedef struct agpu_texture_t* agpu_texture;

    typedef enum agpu_backend {
        AGPU_BACKEND_DEFAULT = 0,
        AGPU_BACKEND_OPENGL = 1,
        AGPU_BACKEND_D3D11 = 2,
        _AGPU_BACKEND_D3D11_FORCE_U32 = 0x7FFFFFFF
    } agpu_backend;

    typedef struct agpu_config {
        agpu_backend backend;
        bool debug;
        void (*callback)(void* context, const char* message, int level);
        void* context;
        void* (*gl_get_proc_address)(const char*);
    } agpu_config;

    bool agpu_init(const agpu_config* config);
    void agpu_shutdown(void);
    void agpu_frame_begin(void);
    void agpu_frame_end(void);

#ifdef __cplusplus
}
#endif
