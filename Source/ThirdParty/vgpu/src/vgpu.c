//
// Copyright (c) 2019-2020 Amer Koleci.
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

static const vgpu_driver* drivers[] = {
#if defined(VGPU_DRIVER_D3D11)
    &d3d11_driver,
#endif
#if defined(VGPU_DRIVER_VULKAN)
    &vulkan_driver,
#endif
#if defined(VGPU_DRIVER_OPENGL)
    &gl_driver,
#endif
    NULL
};


vgpu_device vgpu_create_device(vgpu_backend_type backend_type, bool debug) {
    if (backend_type == VGPU_BACKEND_TYPE_COUNT) {
        for (uint32_t i = 0; _vgpu_count_of(drivers); i++) {
            if (drivers[i]->is_supported()) {
                return drivers[i]->create_device(debug);
            }
        }
    }
    else {
        for (uint32_t i = 0; _vgpu_count_of(drivers); i++) {
            if (drivers[i]->backend_type == backend_type && drivers[i]->is_supported()) {
                return drivers[i]->create_device(debug);
            }
        }
    }

    return NULL;
}

void vgpu_destroy_device(vgpu_device device) {
    if (device == NULL)
    {
        return;
    }

    device->destroy(device);
}

vgpu_context vgpu_create_context(vgpu_device device, const vgpu_context_info* info) {
    vgpu_assert(device);
    vgpu_assert(info);
    return device->create_context(device->renderer, info);
}

void vgpu_destroy_context(vgpu_device device, vgpu_context context) {
    vgpu_assert(device);
    vgpu_assert(context);
    device->destroy_context(device->renderer, context);
}

void vgpu_begin_frame(vgpu_device device, vgpu_context context) {
    device->begin_frame(device->renderer, context);
}

void vgpu_end_frame(vgpu_device device, vgpu_context context) {
    device->end_frame(device->renderer, context);
}
