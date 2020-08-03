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

#include "agpu_driver.h"
#include <stdio.h>
#include <stdarg.h>

 /* Drivers */
static const agpu_driver* drivers[] = {
#if defined(AGPU_DRIVER_OPENGL)
    &gl_driver,
#endif
    NULL
};

static agpu_renderer* s_gpu_renderer = NULL;

bool agpu_init(const agpu_config* config) {
    if (s_gpu_renderer) {
        return true;
    }

    GPU_ASSERT(config);

    if (config->backend == AGPU_BACKEND_DEFAULT) {
        for (uint32_t i = 0; _AGPU_COUNTOF(drivers); i++) {
            if (drivers[i]->is_supported()) {
                s_gpu_renderer = drivers[i]->init_renderer();
                break;
            }
        }
    }
    else {
        for (uint32_t i = 0; _AGPU_COUNTOF(drivers); i++) {
            if (drivers[i]->backend == config->backend && drivers[i]->is_supported()) {
                s_gpu_renderer = drivers[i]->init_renderer();
                break;
            }
        }
    }

    if (!s_gpu_renderer) {
        return false;
    }

    if (!s_gpu_renderer->init(config)) {
        s_gpu_renderer = NULL;
        return false;
    }

    return true;
}

void agpu_shutdown(void) {
    if (s_gpu_renderer) {
        s_gpu_renderer->shutdown();
        s_gpu_renderer = NULL;
    }
}

void agpu_frame_begin(void) {
    s_gpu_renderer->frame_begin();
}

void agpu_frame_end(void) {
    s_gpu_renderer->frame_end();
}

agpu_buffer agpu_create_buffer(const agpu_buffer_info* info) {
    GPU_ASSERT(s_gpu_renderer);
    GPU_ASSERT(info);

    return s_gpu_renderer->create_buffer(info);
}

void agpu_destroy_buffer(agpu_buffer buffer) {
    GPU_ASSERT(s_gpu_renderer);
    GPU_ASSERT(buffer);

    s_gpu_renderer->destroy_buffer(buffer);
}

/* Internal log functions */
#define AGPU_LOG_MAX_MESSAGE_SIZE 1024

void agpu_log_info(const char* fmt, ...) {
    if (s_gpu_renderer->log) {
        char msg[AGPU_LOG_MAX_MESSAGE_SIZE];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(msg, sizeof(msg), fmt, ap);
        va_end(ap);
        s_gpu_renderer->log(AGPU_LOG_LEVEL_INFO, msg);
    }
}

void agpu_log_warn(const char* fmt, ...) {
    if (s_gpu_renderer->log) {
        char msg[AGPU_LOG_MAX_MESSAGE_SIZE];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(msg, sizeof(msg), fmt, ap);
        va_end(ap);
        s_gpu_renderer->log(AGPU_LOG_LEVEL_WARN, msg);
    }
}

void agpu_log_error(const char* fmt, ...) {
    if (s_gpu_renderer->log) {
        char msg[AGPU_LOG_MAX_MESSAGE_SIZE];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(msg, sizeof(msg), fmt, ap);
        va_end(ap);
        s_gpu_renderer->log(AGPU_LOG_LEVEL_ERROR, msg);
    }
}
