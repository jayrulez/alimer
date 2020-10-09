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

#include "vgpu_driver.h"
#include <stdio.h>
#include <stdarg.h>

/* Logging */
static vgpu_log_callback s_log_function = NULL;
static void* s_log_user_data = NULL;

void vgpu_set_log_callback(vgpu_log_callback callback, void* user_data) {
    s_log_function = callback;
    s_log_user_data = user_data;
}

void vgpu_log(vgpu_log_level level, const char* format, ...) {
    if (s_log_function) {
        va_list args;
        va_start(args, format);
        char message[VGPU_MAX_LOG_MESSAGE_LENGTH];
        vsnprintf(message, VGPU_MAX_LOG_MESSAGE_LENGTH, format, args);
        s_log_function(s_log_user_data, level, message);
        va_end(args);
    }
}

static const agpu_driver* drivers[] = {
#if AGPU_DRIVER_D3D12 && defined(TODO_D3D12)
    &d3d12_driver,
#endif
#if AGPU_DRIVER_D3D11
    & D3D11_Driver,
#endif
#if AGPU_DRIVER_METAL
    & metal_driver,
#endif
#if VGPU_DRIVER_VULKAN 
    &vulkan_driver,
#endif
#if AGPU_DRIVER_OPENGL
    & GL_Driver,
#endif
    NULL
};

static agpu_config _agpu_config_defaults(const agpu_config* config) {
    agpu_config def = *config;
    def.swapchain_info.color_format = AGPU_DEF(def.swapchain_info.color_format, AGPU_TEXTURE_FORMAT_BGRA8_UNORM);
    def.swapchain_info.depth_stencil_format = AGPU_DEF(def.swapchain_info.depth_stencil_format, AGPU_TEXTURE_FORMAT_INVALID);
    def.swapchain_info.sample_count = AGPU_DEF(def.swapchain_info.sample_count, 1u);
    return def;
};

static agpu_backend_type s_backend = VGPU_BACKEND_TYPE_COUNT;
static agpu_renderer* renderer = NULL;

bool agpu_set_preferred_backend(agpu_backend_type backend)
{
    if (renderer != NULL)
        return false;

    s_backend = backend;
    return true;
}

bool vgpu_init(const char* app_name, const agpu_config* config)
{
    AGPU_ASSERT(config);

    if (renderer) {
        return true;
    }

    if (s_backend == VGPU_BACKEND_TYPE_COUNT)
    {
        for (uint32_t i = 0; i < VGPU_COUNT_OF(drivers); i++)
        {
            if (!drivers[i])
                break;

            if (drivers[i]->is_supported()) {
                renderer = drivers[i]->create_renderer();
                break;
            }
        }
    }
    else
    {
        for (uint32_t i = 0; i < VGPU_COUNT_OF(drivers); i++)
        {
            if (!drivers[i])
                break;

            if (drivers[i]->backend == s_backend && drivers[i]->is_supported())
            {
                renderer = drivers[i]->create_renderer();
                break;
            }
        }
    }

    agpu_config config_def = _agpu_config_defaults(config);
    if (!renderer || !renderer->init(app_name, &config_def)) {
        return false;
    }

    return true;
}

void vgpu_shutdown(void) {
    if (renderer == NULL)
        return;

    renderer->shutdown();
    renderer = NULL;
}

void vgpu_query_caps(agpu_caps* caps)
{
    AGPU_ASSERT(caps);
    AGPU_ASSERT(renderer);

    renderer->query_caps(caps);
}

bool vgpu_begin_frame(void)
{
    return renderer->frame_begin();
}

void vgpu_end_frame(void)
{
    renderer->frame_finish();
}

/* Buffer */
vgpu_buffer vgpu_create_buffer(const agpu_buffer_info* info) {
    AGPU_ASSERT(info);
    AGPU_ASSERT(info->size > 0);

    if (info->usage == AGPU_BUFFER_USAGE_IMMUTABLE && !info->data)
    {
        vgpu_log(VGPU_LOG_LEVEL_ERROR, "Cannot create immutable buffer without data");
        return NULL;
    }

    return renderer->buffer_create(info);
}

void vgpu_destroy_buffer(vgpu_buffer buffer) {
    AGPU_ASSERT(buffer);

    renderer->buffer_destroy(buffer);
}

/* Shader */
vgpu_shader vgpu_create_shader(const vgpu_shader_info* info) {
    AGPU_ASSERT(info);
    return renderer->shader_create(info);
}

void vgpu_destroy_shader(vgpu_shader shader) {
    AGPU_ASSERT(shader);
    renderer->shader_destroy(shader);
}

/* Texture */
static vgpu_texture_info _vgpu_texture_info_defaults(const vgpu_texture_info* info) {
    vgpu_texture_info def = *info;
    def.type = AGPU_DEF(def.type, AGPU_TEXTURE_TYPE_2D);
    def.format = AGPU_DEF(def.format, AGPU_TEXTURE_FORMAT_RGBA8_UNORM);
    def.depth = AGPU_DEF(def.depth, 1);
    def.mipmaps = AGPU_DEF(def.mipmaps, 1);
    return def;
};

vgpu_texture vgpu_create_texture(const vgpu_texture_info* info)
{
    AGPU_ASSERT(info);
    vgpu_texture_info def = _vgpu_texture_info_defaults(info);
    return renderer->texture_create(&def);
}

void vgpu_destroy_texture(vgpu_texture handle) {
    renderer->texture_destroy(handle);
}

uint64_t vgpu_texture_get_native_handle(vgpu_texture handle) {
    return renderer->texture_get_native_handle(handle);
}

/* Pipeline */
vgpu_pipeline vgpu_create_pipeline(const vgpu_pipeline_info* info) {
    AGPU_ASSERT(info);
    return renderer->pipeline_create(info);
}

void vgpu_destroy_pipeline(vgpu_pipeline pipeline) {
    renderer->pipeline_destroy(pipeline);
}

/* Commands */
void vgpu_push_debug_group(const char* name) {
    renderer->push_debug_group(name);
}

void vgpu_pop_debug_group(void) {
    renderer->pop_debug_group();
}

void vgpu_insert_debug_marker(const char* name) {
    renderer->insert_debug_marker(name);
}

void vgpu_begin_render_pass(const agpu_render_pass_info* info) {
    AGPU_ASSERT(info);
    AGPU_ASSERT(info->num_color_attachments || info->depth_stencil.texture);

    renderer->begin_render_pass(info);
}

void vgpu_end_render_pass(void) {
    renderer->end_render_pass();
}

void vgpu_bind_pipeline(vgpu_pipeline pipeline) {
    AGPU_ASSERT(pipeline);
    renderer->bind_pipeline(pipeline);
}

void vgpu_draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex) {
    renderer->draw(vertex_count, instance_count, first_vertex);
}

/* Utility methods */
uint32_t vgpu_calculate_mip_levels(uint32_t width, uint32_t height, uint32_t depth)
{
    uint32_t mipLevels = 0u;
    uint32_t size = AGPU_MAX(AGPU_MAX(width, height), depth);
    while (1u << mipLevels <= size) {
        ++mipLevels;
    }

    if (1u << mipLevels < size) {
        ++mipLevels;
    }

    return mipLevels;
}
