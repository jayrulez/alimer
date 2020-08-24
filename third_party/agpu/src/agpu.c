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

#include "agpu_driver.h"
#include <stdio.h>
#include <stdarg.h>

#define VGPU_MAX_LOG_MESSAGE (4096)
static agpu_log_callback s_log_function = NULL;
static void* s_log_user_data = NULL;

void agpu_set_log_callback(agpu_log_callback callback, void* user_data) {
    s_log_function = callback;
    s_log_user_data = user_data;
}

void agpu_log_error(const char* format, ...) {
    if (s_log_function) {
        va_list args;
        va_start(args, format);
        char message[VGPU_MAX_LOG_MESSAGE];
        vsnprintf(message, VGPU_MAX_LOG_MESSAGE, format, args);
        s_log_function(s_log_user_data, AGPU_LOG_LEVEL_ERROR, message);
        va_end(args);
    }
}

void agpu_log_warn(const char* format, ...) {
    if (s_log_function) {
        va_list args;
        va_start(args, format);
        char message[VGPU_MAX_LOG_MESSAGE];
        vsnprintf(message, VGPU_MAX_LOG_MESSAGE, format, args);
        s_log_function(s_log_user_data, AGPU_LOG_LEVEL_ERROR, message);
        va_end(args);
    }
}

void agpu_log_info(const char* format, ...) {
    if (s_log_function) {
        va_list args;
        va_start(args, format);
        char message[VGPU_MAX_LOG_MESSAGE];
        vsnprintf(message, VGPU_MAX_LOG_MESSAGE, format, args);
        s_log_function(s_log_user_data, AGPU_LOG_LEVEL_INFO, message);
        va_end(args);
    }
}

static const agpu_driver* drivers[] = {
#if AGPU_DRIVER_D3D11
    & d3d11_driver,
#endif
#if AGPU_DRIVER_METAL
    & metal_driver,
#endif
#if AGPU_DRIVER_VULKAN && defined(TODO_VK)
    &vulkan_driver,
#endif
#if AGPU_DRIVER_OPENGL
    &gl_driver,
#endif

    NULL
};

static agpu_renderer* gpu_renderer = NULL;

bool agpu_init(const agpu_init_info* info) {
    AGPU_ASSERT(info);

    if (gpu_renderer) {
        return true;
    }

    if (info->backend_type == AGPU_BACKEND_TYPE_DEFAULT) {
        for (uint32_t i = 0; AGPU_COUNT_OF(drivers); i++) {
            if (drivers[i]->is_supported()) {
                gpu_renderer = drivers[i]->create_renderer();
                break;
            }
        }
    }
    else {
        for (uint32_t i = 0; AGPU_COUNT_OF(drivers); i++) {
            if (drivers[i]->backend_type == info->backend_type && drivers[i]->is_supported()) {
                gpu_renderer = drivers[i]->create_renderer();
                break;
            }
        }
    }

    if (!gpu_renderer || !gpu_renderer->init(info)) {
        return false;
    }

    return true;
}

void agpu_shutdown(void) {
    if (gpu_renderer == NULL)
    {
        return;
    }

    gpu_renderer->shutdown();
}

agpu_device_caps agpu_query_caps(void) {
    AGPU_ASSERT(gpu_renderer);
    return gpu_renderer->query_caps();
}

agpu_texture_format_info agpu_query_texture_format_info(agpu_texture_format format) {
    return gpu_renderer->query_texture_format_info(format);
}

/* Context */
static agpu_swapchain_info _agpu_swapchain_info_def(const agpu_swapchain_info* info) {
    agpu_swapchain_info def = *info;
    def.width = _agpu_def(def.width, 1u);
    def.height = _agpu_def(def.height, 1u);
    def.color_format = _agpu_def(def.color_format, AGPU_TEXTURE_FORMAT_BGRA8_UNORM);
    def.depth_stencil_format = _agpu_def(def.depth_stencil_format, AGPU_TEXTURE_FORMAT_UNDEFINED);
    def.vsync = _agpu_def(def.vsync, false);
    def.fullscreen = _agpu_def(def.fullscreen, false);
    return def;
}


agpu_context agpu_create_context(const agpu_context_info* info) {
    AGPU_ASSERT(info);

    agpu_swapchain_info def_swapchain_info;
    agpu_context_info def = *info;
    if (def.swapchain_info) {
        def_swapchain_info = _agpu_swapchain_info_def(def.swapchain_info);
        def.swapchain_info = &def_swapchain_info;
    }

    return gpu_renderer->create_context(&def);
}

void agpu_destroy_context(agpu_context context) {
    gpu_renderer->destroy_context(context);
}

void agpu_set_context(agpu_context context) {
    gpu_renderer->set_context(context);
}

void agpu_begin_frame(void) {
    gpu_renderer->begin_frame();
}

void agpu_end_frame(void) {
    gpu_renderer->end_frame();
}
