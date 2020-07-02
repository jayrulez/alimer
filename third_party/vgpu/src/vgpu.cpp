//
// Copyright (c) 2020 Amer Koleci and contributors.
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

#include "vgpu_driver.h"

/* Drivers */
static const vgpu_driver* drivers[] = {
#if defined(VGPU_DRIVER_D3D12)
    &d3d12_driver,
#endif
    nullptr
};

static vgpu_renderer* s_gpu_renderer = nullptr;

static vgpu_config _vgpu_config_defaults(const vgpu_config* desc) {
    vgpu_config def = *desc;
    def.swapchain.color_format = _vgpu_def(desc->swapchain.color_format, VGPU_TEXTURE_FORMAT_BGRA8_UNORM);
    def.swapchain.depth_stencil_format = _vgpu_def(desc->swapchain.depth_stencil_format, VGPU_TEXTURE_FORMAT_BGRA8_UNORM);
    return def;
}

bool vgpu_init(const vgpu_config* config) {
    if (s_gpu_renderer) {
        return true;
    }

    if (config->preferred_backend == VGPU_BACKEND_TYPE_DEFAULT) {
        for (uint32_t i = 0; _vgpu_count_of(drivers); i++) {
            if (drivers[i]->is_supported()) {
                s_gpu_renderer = drivers[i]->init_renderer();
                break;
            }
        }
    }
    else {
        for (uint32_t i = 0; _vgpu_count_of(drivers); i++) {
            if (drivers[i]->backendType == config->preferred_backend && drivers[i]->is_supported()) {
                s_gpu_renderer = drivers[i]->init_renderer();
                break;
            }
        }
    }

    if (!s_gpu_renderer->init(config)) {
        s_gpu_renderer = nullptr;
        return false;
    }

    return true;
}

void vgpu_shutdown(void) {
    if (s_gpu_renderer == nullptr) {
        return;
    }

    s_gpu_renderer->shutdown();
    s_gpu_renderer = nullptr;
}

void vgpu_begin_frame(void) {
    s_gpu_renderer->begin_frame();
}

void vgpu_end_frame(void) {
    s_gpu_renderer->end_frame();
}
