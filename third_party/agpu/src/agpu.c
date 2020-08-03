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
        for (uint32_t i = 0; GPU_COUNTOF(drivers); i++) {
            if (drivers[i]->is_supported()) {
                s_gpu_renderer = drivers[i]->init_renderer();
                break;
            }
        }
    }
    else {
        for (uint32_t i = 0; GPU_COUNTOF(drivers); i++) {
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
