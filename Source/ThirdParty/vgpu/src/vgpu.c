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
#include <stdlib.h>

static const vgpu_driver* drivers[] = {
#if defined(VGPU_DRIVER_D3D11)
    &d3d11_driver,
#endif
#if defined(VGPU_DRIVER_D3D12)
    &d3d12_driver,
#endif
#if defined(VGPU_DRIVER_VULKAN)
    &vulkan_driver,
#endif
#if defined(VGPU_DRIVER_OPENGL)
    &gl_driver,
#endif
    NULL
};

static void vgpu_log_default_callback(void* user_data, vgpu_log_level level, const char* message) {

}

void* vgpu_default_allocate_memory(void* user_data, size_t size) {
    VGPU_UNUSED(user_data);
    return malloc(size);
}

void vgpu_default_free_memory(void* user_data, void* ptr) {
    VGPU_UNUSED(user_data);
    free(ptr);
}


const vgpu_allocation_callbacks vgpu_default_alloc_cb = {
    NULL,
    vgpu_default_allocate_memory,
    vgpu_default_free_memory
};

static vgpu_PFN_log vgpu_log_callback = vgpu_log_default_callback;
static void* vgpu_log_user_data = NULL;
const vgpu_allocation_callbacks* vgpu_alloc_cb = &vgpu_default_alloc_cb;
void* vgpu_allocation_user_data = NULL;

void vgpu_log_set_log_callback(vgpu_PFN_log callback, void* user_data) {
    vgpu_log_callback = callback;
    vgpu_log_user_data = user_data;
}

void vgpu_set_allocation_callbacks(const vgpu_allocation_callbacks* callbacks) {
    if (callbacks == NULL) {
        vgpu_alloc_cb = &vgpu_default_alloc_cb;
    }
    else {
        vgpu_alloc_cb = callbacks;
    }
}

static vgpu_device_desc _vgpu_device_desc_defaults(const vgpu_device_desc* desc) {
    vgpu_device_desc def = *desc;
    def.swapchain.width = _vgpu_def(desc->swapchain.width, 1u);
    def.swapchain.height = _vgpu_def(desc->swapchain.height, 1u);
    def.swapchain.color_format = _vgpu_def(desc->swapchain.color_format, VGPU_TEXTURE_FORMAT_BGRA8_UNORM);
    def.swapchain.depth_stencil_format = _vgpu_def(desc->swapchain.depth_stencil_format, VGPU_TEXTURE_FORMAT_UNDEFINED);
    def.swapchain.sample_count = _vgpu_def(desc->swapchain.sample_count, VGPU_SAMPLE_COUNT_1);
    def.swapchain.fullscreen = _vgpu_def(desc->swapchain.fullscreen, false);
    def.swapchain.present_interval = _vgpu_def(desc->swapchain.present_interval, VGPU_PRESENT_INTERVAL_DEFAULT);
    return def;
}

vgpu_device vgpu_create_device(vgpu_backend_type backend_type, const vgpu_device_desc* desc) {
    VGPU_ASSERT(desc);
    vgpu_device_desc desc_def = _vgpu_device_desc_defaults(desc);
    if (backend_type == VGPU_BACKEND_TYPE_COUNT) {
        for (uint32_t i = 0; _vgpu_count_of(drivers); i++) {
            if (drivers[i]->is_supported()) {
                return drivers[i]->create_device(&desc_def);
            }
        }
    }
    else {
        for (uint32_t i = 0; _vgpu_count_of(drivers); i++) {
            if (drivers[i]->backend_type == backend_type && drivers[i]->is_supported()) {
                return drivers[i]->create_device(&desc_def);
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

void vgpu_begin_frame(vgpu_device device) {
    device->begin_frame(device->renderer);
}

void vgpu_present_frame(vgpu_device device) {
    device->present_frame(device->renderer);
}

static vgpu_texture_desc _vgpu_texture_desc_defaults(const vgpu_texture_desc* desc) {
    vgpu_texture_desc def = *desc;
    def.type = _vgpu_def(desc->type, VGPU_TEXTURE_TYPE_2D);
    def.format = _vgpu_def(desc->format, VGPU_TEXTURE_FORMAT_RGBA8_UNORM);
    def.width = _vgpu_def(desc->width, 1u);
    def.height = _vgpu_def(desc->height, 1u);
    def.depth_or_layers = _vgpu_def(desc->depth_or_layers, 1u);
    def.mip_levels = _vgpu_def(desc->mip_levels, 1u);
    def.sample_count = _vgpu_def(desc->sample_count, VGPU_SAMPLE_COUNT_1);
    return def;
}

vgpu_texture vgpu_create_texture(vgpu_device device, const vgpu_texture_desc* desc) {
    VGPU_ASSERT(device);
    VGPU_ASSERT(desc);

    vgpu_texture_desc desc_def = _vgpu_texture_desc_defaults(desc);
    return device->create_texture(device->renderer, &desc_def);
}

void vgpu_destroy_texture(vgpu_device device, vgpu_texture texture) {
    VGPU_ASSERT(device);
    VGPU_ASSERT(texture);

    device->destroy_texture(device->renderer, texture);
}
