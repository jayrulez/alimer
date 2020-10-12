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

static const vgpu_driver* drivers[] = {
#if VGPU_DRIVER_D3D12
    & d3d12_driver,
#endif
#if VGPU_DRIVER_D3D11
    & D3D11_Driver,
#endif
#if VGPU_DRIVER_METAL
    & metal_driver,
#endif
#if VGPU_DRIVER_VULKAN
    & vulkan_driver,
#endif
#if VGPU_DRIVER_OPENGL
    & GL_Driver,
#endif
    NULL
};

static vgpu_config _vgpu_config_defaults(const vgpu_config* config) {
    vgpu_config def = *config;
    def.device_preference = VGPU_DEF(def.device_preference, VGPU_ADAPTER_TYPE_DISCRETE_GPU);
    def.swapchain_info.color_format = VGPU_DEF(def.swapchain_info.color_format, VGPU_TEXTURE_FORMAT_BGRA8);
    def.swapchain_info.depth_stencil_format = VGPU_DEF(def.swapchain_info.depth_stencil_format, VGPU_TEXTURE_FORMAT_UNDEFINED);
    def.swapchain_info.sample_count = VGPU_DEF(def.swapchain_info.sample_count, 1u);
    return def;
};

static vgpu_backend_type s_backend = VGPU_BACKEND_TYPE_COUNT;
static vgpu_renderer* renderer = NULL;

bool vgpu_set_preferred_backend(vgpu_backend_type backend)
{
    if (renderer != NULL)
        return false;

    s_backend = backend;
    return true;
}

bool vgpu_init(const char* app_name, const vgpu_config* config)
{
    VGPU_ASSERT(config);

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

    vgpu_config config_def = _vgpu_config_defaults(config);
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

void vgpu_query_caps(vgpu_caps* caps)
{
    VGPU_ASSERT(caps);
    VGPU_ASSERT(renderer);

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
vgpu_buffer vgpu_create_buffer(const vgpu_buffer_info* info) {
    VGPU_ASSERT(info);
    VGPU_ASSERT(info->size > 0);

    if (info->usage == VGPU_BUFFER_USAGE_IMMUTABLE && !info->data)
    {
        vgpu_log(VGPU_LOG_LEVEL_ERROR, "Cannot create immutable buffer without data");
        return NULL;
    }

    return renderer->buffer_create(info);
}

void vgpu_destroy_buffer(vgpu_buffer buffer) {
    VGPU_ASSERT(buffer);

    renderer->buffer_destroy(buffer);
}

/* Shader */
vgpu_shader vgpu_create_shader(const vgpu_shader_info* info) {
    VGPU_ASSERT(info);
    return renderer->shader_create(info);
}

void vgpu_destroy_shader(vgpu_shader shader) {
    VGPU_ASSERT(shader);
    renderer->shader_destroy(shader);
}

/* Texture */
static vgpu_texture_info _vgpu_texture_info_defaults(const vgpu_texture_info* info) {
    vgpu_texture_info def = *info;
    def.type = VGPU_DEF(def.type, VGPU_TEXTURE_TYPE_2D);
    def.format = VGPU_DEF(def.format, VGPU_TEXTURE_FORMAT_RGBA8);
    def.size.depth = VGPU_DEF(def.size.depth, 1);
    def.mip_level_count = VGPU_DEF(def.mip_level_count, 1);
    def.sample_count = VGPU_DEF(def.sample_count, 1);
    return def;
};

vgpu_texture vgpu_create_texture(const vgpu_texture_info* info)
{
    VGPU_ASSERT(info);
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
    VGPU_ASSERT(info);
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

void vgpu_begin_render_pass(const vgpu_render_pass_info* info) {
    VGPU_ASSERT(info);
    VGPU_ASSERT(info->num_color_attachments || info->depth_stencil.texture);

    renderer->begin_render_pass(info);
}

void vgpu_end_render_pass(void) {
    renderer->end_render_pass();
}

void vgpu_bind_pipeline(vgpu_pipeline pipeline) {
    VGPU_ASSERT(pipeline);
    renderer->bind_pipeline(pipeline);
}

void vgpu_draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex) {
    renderer->draw(vertex_count, instance_count, first_vertex);
}

/* Utility methods */

/* Pixel Format */
typedef struct vgpu_texture_format_desc
{
    vgpu_texture_format         format;
    vgpu_texture_format_type    type;
    uint8_t                     bits_per_pixel;
    struct
    {
        uint8_t             block_width;
        uint8_t             block_height;
        uint8_t             block_size;
        uint8_t             min_block_x;
        uint8_t             min_block_y;
    } compression;

    struct
    {
        uint8_t             depth;
        uint8_t             stencil;
        uint8_t             red;
        uint8_t             green;
        uint8_t             blue;
        uint8_t             alpha;
    } bits;
} vgpu_texture_format_desc;

const vgpu_texture_format_desc k_format_desc[] =
{
    // format                               type                                    bpp         compression             bits
    { VGPU_TEXTURE_FORMAT_UNDEFINED,        VGPU_TEXTURE_FORMAT_TYPE_UNKNOWN,       0,          {0, 0, 0, 0, 0},        {0, 0, 0, 0, 0, 0}},
    // 8-bit pixel formats
    { VGPU_TEXTURE_FORMAT_R8,               VGPU_TEXTURE_FORMAT_TYPE_UNORM,         8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    { VGPU_TEXTURE_FORMAT_R8S,              VGPU_TEXTURE_FORMAT_TYPE_SNORM,         8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    { VGPU_TEXTURE_FORMAT_R8U,              VGPU_TEXTURE_FORMAT_TYPE_UINT,          8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    { VGPU_TEXTURE_FORMAT_R8I,              VGPU_TEXTURE_FORMAT_TYPE_SINT,          8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},

    // 16-bit pixel formats
    { VGPU_TEXTURE_FORMAT_R16,              VGPU_TEXTURE_FORMAT_TYPE_UNORM,         16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPU_TEXTURE_FORMAT_R16S,             VGPU_TEXTURE_FORMAT_TYPE_SNORM,         16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPU_TEXTURE_FORMAT_R16U,             VGPU_TEXTURE_FORMAT_TYPE_UINT,          16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPU_TEXTURE_FORMAT_R16I,             VGPU_TEXTURE_FORMAT_TYPE_SINT,          16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPU_TEXTURE_FORMAT_R16F,             VGPU_TEXTURE_FORMAT_TYPE_FLOAT,         16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPU_TEXTURE_FORMAT_RG8,              VGPU_TEXTURE_FORMAT_TYPE_UNORM,         16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    { VGPU_TEXTURE_FORMAT_RG8S,             VGPU_TEXTURE_FORMAT_TYPE_SNORM,         16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    { VGPU_TEXTURE_FORMAT_RG8U,             VGPU_TEXTURE_FORMAT_TYPE_UINT,          16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    { VGPU_TEXTURE_FORMAT_RG8I,             VGPU_TEXTURE_FORMAT_TYPE_SINT,          16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},

    // Packed 16-bit pixel formats
    //{ VGPU_PIXEL_FORMAT_R5G6B5_UNORM,     VGPU_TEXTURE_FORMAT_TYPE_UNORM,       16,         {1, 1, 2, 1, 1},        {0, 0, 5, 6, 5, 0}},
    //{ VGPU_PIXEL_FORMAT_RGBA4_UNORM,      VGPU_TEXTURE_FORMAT_TYPE_UNORM,       16,         {1, 1, 2, 1, 1},        {0, 0, 4, 4, 4, 4}},

    // 32-bit pixel formats
    { VGPU_TEXTURE_FORMAT_R32,              VGPU_TEXTURE_FORMAT_TYPE_UINT,          32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
    { VGPU_TEXTURE_FORMAT_R32S,             VGPU_TEXTURE_FORMAT_TYPE_SINT,          32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
    { VGPU_TEXTURE_FORMAT_R32F,             VGPU_TEXTURE_FORMAT_TYPE_FLOAT,         32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
    { VGPU_TEXTURE_FORMAT_RG16,             VGPU_TEXTURE_FORMAT_TYPE_UNORM,         32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { VGPU_TEXTURE_FORMAT_RG16S,            VGPU_TEXTURE_FORMAT_TYPE_SNORM,         32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { VGPU_TEXTURE_FORMAT_RG16U,            VGPU_TEXTURE_FORMAT_TYPE_UINT,          32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { VGPU_TEXTURE_FORMAT_RG16I,            VGPU_TEXTURE_FORMAT_TYPE_SINT,          32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { VGPU_TEXTURE_FORMAT_RG16F,            VGPU_TEXTURE_FORMAT_TYPE_FLOAT,         32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { VGPU_TEXTURE_FORMAT_RGBA8,            VGPU_TEXTURE_FORMAT_TYPE_UNORM,         32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPU_TEXTURE_FORMAT_RGBA8S,           VGPU_TEXTURE_FORMAT_TYPE_SNORM,         32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPU_TEXTURE_FORMAT_RGBA8U,           VGPU_TEXTURE_FORMAT_TYPE_UINT,          32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPU_TEXTURE_FORMAT_RGBA8I,           VGPU_TEXTURE_FORMAT_TYPE_SINT,          32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPU_TEXTURE_FORMAT_BGRA8,            VGPU_TEXTURE_FORMAT_TYPE_UNORM,         32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},

    // Packed 32-Bit Pixel formats
    { VGPU_TEXTURE_FORMAT_RGB10A2,          VGPU_TEXTURE_FORMAT_TYPE_UNORM,         32,         {1, 1, 4, 1, 1},        {0, 0, 10, 10, 10, 2}},
    { VGPU_TEXTURE_FORMAT_RG11B10F,         VGPU_TEXTURE_FORMAT_TYPE_FLOAT,         32,         {1, 1, 4, 1, 1},        {0, 0, 11, 11, 10, 0}},
    { VGPU_TEXTURE_FORMAT_RGB9E5F,          VGPU_TEXTURE_FORMAT_TYPE_FLOAT,         32,         {1, 1, 4, 1, 1},        {0, 0, 9, 9, 9, 5}},

    // 64-Bit Pixel Formats
    { VGPU_TEXTURE_FORMAT_RG32U,            VGPU_TEXTURE_FORMAT_TYPE_UINT,          64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
    { VGPU_TEXTURE_FORMAT_RG32I,            VGPU_TEXTURE_FORMAT_TYPE_SINT,          64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
    { VGPU_TEXTURE_FORMAT_RG32F,            VGPU_TEXTURE_FORMAT_TYPE_FLOAT,         64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
    { VGPU_TEXTURE_FORMAT_RGBA16,           VGPU_TEXTURE_FORMAT_TYPE_UNORM,         64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { VGPU_TEXTURE_FORMAT_RGBA16S,          VGPU_TEXTURE_FORMAT_TYPE_SNORM,         64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { VGPU_TEXTURE_FORMAT_RGBA16U,          VGPU_TEXTURE_FORMAT_TYPE_UINT,          64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { VGPU_TEXTURE_FORMAT_RGBA16I,          VGPU_TEXTURE_FORMAT_TYPE_SINT,          64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { VGPU_TEXTURE_FORMAT_RGBA16F,          VGPU_TEXTURE_FORMAT_TYPE_FLOAT,         64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},

    // 128-Bit Pixel Formats
    { VGPU_TEXTURE_FORMAT_RGBA32U,          VGPU_TEXTURE_FORMAT_TYPE_UINT,        128,          {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},
    { VGPU_TEXTURE_FORMAT_RGBA32I,          VGPU_TEXTURE_FORMAT_TYPE_SINT,        128,          {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},
    { VGPU_TEXTURE_FORMAT_RGBA32F,          VGPU_TEXTURE_FORMAT_TYPE_FLOAT,       128,          {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},

    // Depth-stencil
    { VGPU_TEXTURE_FORMAT_D16,              VGPU_TEXTURE_FORMAT_TYPE_UNORM,       16,           {1, 1, 2, 1, 1},        {16, 0, 0, 0, 0, 0}},
    { VGPU_TEXTURE_FORMAT_D32F,             VGPU_TEXTURE_FORMAT_TYPE_FLOAT,       32,           {1, 1, 4, 1, 1},        {32, 0, 0, 0, 0, 0}},
    { VGPU_TEXTURE_FORMAT_D24S8,            VGPU_TEXTURE_FORMAT_TYPE_FLOAT,       32,           {1, 1, 4, 1, 1},        {24, 8, 0, 0, 0, 0}},

    // Compressed formats
    { VGPU_TEXTURE_FORMAT_BC1,              VGPU_TEXTURE_FORMAT_TYPE_UNORM,        4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPU_TEXTURE_FORMAT_BC2,              VGPU_TEXTURE_FORMAT_TYPE_UNORM,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_TEXTURE_FORMAT_BC3,              VGPU_TEXTURE_FORMAT_TYPE_UNORM,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_TEXTURE_FORMAT_BC4,              VGPU_TEXTURE_FORMAT_TYPE_UNORM,        4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPU_TEXTURE_FORMAT_BC4S,             VGPU_TEXTURE_FORMAT_TYPE_SNORM,        4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPU_TEXTURE_FORMAT_BC5,              VGPU_TEXTURE_FORMAT_TYPE_UNORM,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_TEXTURE_FORMAT_BC5S,             VGPU_TEXTURE_FORMAT_TYPE_SNORM,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_TEXTURE_FORMAT_BC6H,             VGPU_TEXTURE_FORMAT_TYPE_FLOAT,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_TEXTURE_FORMAT_BC6HS,            VGPU_TEXTURE_FORMAT_TYPE_FLOAT,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_TEXTURE_FORMAT_BC7,              VGPU_TEXTURE_FORMAT_TYPE_UNORM,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},

    /*
    // Compressed PVRTC Pixel Formats
    { VGPU_PIXEL_FORMAT_PVRTC_RGB2,             "PVRTC_RGB2",           VGPU_PIXEL_FORMAT_TYPE_UNORM,       2,          {8, 4, 8, 2, 2},        {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_PVRTC_RGBA2,            "PVRTC_RGBA2",          VGPU_PIXEL_FORMAT_TYPE_UNORM,       2,          {8, 4, 8, 2, 2},        {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_PVRTC_RGB4,             "PVRTC_RGB4",           VGPU_PIXEL_FORMAT_TYPE_UNORM,       4,          {4, 4, 8, 2, 2},        {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_PVRTC_RGBA4,            "PVRTC_RGBA4",          VGPU_PIXEL_FORMAT_TYPE_UNORM,       4,          {4, 4, 8, 2, 2},        {0, 0, 0, 0, 0, 0}},
    // Compressed ETC Pixel Formats
    { VGPU_PIXEL_FORMAT_ETC2_RGB8,              "ETC2_RGB8",            VGPU_PIXEL_FORMAT_TYPE_UNORM,       4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_ETC2_RGB8A1,            "ETC2_RGB8A1",          VGPU_PIXEL_FORMAT_TYPE_UNORM,       4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    // Compressed ASTC Pixel Formats
    { VGPU_PIXEL_FORMAT_ASTC4x4,                "ASTC4x4",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_ASTC5x5,                "ASTC5x5",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       6,          {5, 5, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_ASTC6x6,                "ASTC6x6",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       4,          {6, 6, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_ASTC8x5,                "ASTC8x5",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       4,          {8, 5, 16, 1, 1},       {0, 0, 0, 0, 0, 0} },
    { VGPU_PIXEL_FORMAT_ASTC8x6,                "ASTC8x6",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       3,          {8, 6, 16, 1, 1},       {0, 0, 0, 0, 0, 0} },
    { VGPU_PIXEL_FORMAT_ASTC8x8,                "ASTC8x8",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       3,          {8, 8, 16, 1, 1},       {0, 0, 0, 0, 0, 0} },
    { VGPU_PIXEL_FORMAT_ASTC10x10,              "ASTC10x10",            VGPU_PIXEL_FORMAT_TYPE_UNORM,       3,          {10, 10, 16, 1, 1},     {0, 0, 0, 0, 0, 0} },
    { VGPU_PIXEL_FORMAT_ASTC12x12,              "ASTC12x12",            VGPU_PIXEL_FORMAT_TYPE_UNORM,       3,          {12, 12, 16, 1, 1},     {0, 0, 0, 0, 0, 0} },*/
};

bool vgpu_is_depth_format(vgpu_texture_format format) {
    VGPU_ASSERT(k_format_desc[format].format == format);
    return k_format_desc[format].bits.depth > 0;
}

bool vgpu_is_stencil_format(vgpu_texture_format format) {
    VGPU_ASSERT(k_format_desc[format].format == format);
    return k_format_desc[format].bits.stencil > 0;
}

bool vgpu_is_depth_stencil_format(vgpu_texture_format format) {
    return vgpu_is_depth_format(format) || vgpu_is_stencil_format(format);
}

bool vgpu_is_compressed_format(vgpu_texture_format format) {
    VGPU_ASSERT(k_format_desc[format].format == format);
    return format >= VGPU_TEXTURE_FORMAT_BC1 && format <= VGPU_TEXTURE_FORMAT_ASTC_12x12;
}

uint32_t vgpu_calculate_mip_levels(uint32_t width, uint32_t height, uint32_t depth)
{
    uint32_t mipLevels = 0u;
    uint32_t size = VGPU_MAX(VGPU_MAX(width, height), depth);
    while (1u << mipLevels <= size) {
        ++mipLevels;
    }

    if (1u << mipLevels < size) {
        ++mipLevels;
    }

    return mipLevels;
}
