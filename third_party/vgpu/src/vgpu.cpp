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
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h> /* memset */

/* Allocation */
void* vgpu_default_allocate_memory(void* user_data, size_t size) {
    VGPU_UNUSED(user_data);
    return malloc(size);
}

void* vgpu_default_allocate_cleard_memory(void* user_data, size_t size) {
    VGPU_UNUSED(user_data);
    void* mem = malloc(size);
    memset(mem, 0, size);
    return mem;
}

void vgpu_default_free_memory(void* user_data, void* ptr) {
    VGPU_UNUSED(user_data);
    free(ptr);
}

const vgpu_allocation_callbacks vgpu_default_alloc_cb = {
    nullptr,
    vgpu_default_allocate_memory,
    vgpu_default_allocate_cleard_memory,
    vgpu_default_free_memory
};

const vgpu_allocation_callbacks* vgpu_alloc_cb = &vgpu_default_alloc_cb;
void* vgpu_allocation_user_data = nullptr;

void vgpu_set_allocation_callbacks(const vgpu_allocation_callbacks* callbacks) {
    if (callbacks == nullptr) {
        vgpu_alloc_cb = &vgpu_default_alloc_cb;
    }
    else {
        vgpu_alloc_cb = callbacks;
    }
}

/* Log */
#define VGPU_MAX_LOG_MESSAGE (4096)
static vgpu_log_callback s_log_function = nullptr;
static void* s_log_user_data = nullptr;

void vgpu_set_log_callback(vgpu_log_callback callback, void* user_data) {
    s_log_function = callback;
    s_log_user_data = user_data;
}

void vgpu_log(vgpu_log_level level, const char* format, ...) {
    if (s_log_function) {
        va_list args;
        va_start(args, format);
        char message[VGPU_MAX_LOG_MESSAGE];
        vsnprintf(message, VGPU_MAX_LOG_MESSAGE, format, args);
        s_log_function(s_log_user_data, level, message);
        va_end(args);
    }
}

void vgpu_log_error(const char* format, ...) {
    if (s_log_function) {
        va_list args;
        va_start(args, format);
        char message[VGPU_MAX_LOG_MESSAGE];
        vsnprintf(message, VGPU_MAX_LOG_MESSAGE, format, args);
        s_log_function(s_log_user_data, VGPU_LOG_LEVEL_ERROR, message);
        va_end(args);
    }
}

void vgpu_log_info(const char* format, ...) {
    if (s_log_function) {
        va_list args;
        va_start(args, format);
        char message[VGPU_MAX_LOG_MESSAGE];
        vsnprintf(message, VGPU_MAX_LOG_MESSAGE, format, args);
        s_log_function(s_log_user_data, VGPU_LOG_LEVEL_INFO, message);
        va_end(args);
    }
}


/* Drivers */
static const vgpu_driver* drivers[] = {
#if defined(VGPU_DRIVER_D3D11)
    &d3d11_driver,
#endif
#if defined(VGPU_DRIVER_D3D12) 
    &d3d12_driver,
#endif
#if defined(VGPU_DRIVER_VULKAN)&& defined(TODO)
    &vulkan_driver,
#endif
    nullptr
};

static vgpu_renderer* s_gpu_renderer = nullptr;

static vgpu_init_info _vgpu_config_defaults(const vgpu_init_info* desc) {
    vgpu_init_info def = *desc;
    def.swapchain.color_format = _vgpu_def(desc->swapchain.color_format, VGPU_PIXEL_FORMAT_BGRA8_UNORM);
    def.swapchain.depth_stencil_format = _vgpu_def(desc->swapchain.depth_stencil_format, VGPU_PIXEL_FORMAT_DEPTH32_FLOAT);
    return def;
}

bool vgpu_init(const vgpu_init_info* info) {
    VGPU_ASSERT(info);

    if (s_gpu_renderer) {
        return true;
    }

    if (info->preferred_backend == VGPU_BACKEND_TYPE_DEFAULT) {
        for (uint32_t i = 0; _vgpu_count_of(drivers); i++) {
            if (drivers[i]->is_supported()) {
                s_gpu_renderer = drivers[i]->init_renderer();
                break;
            }
        }
    }
    else {
        for (uint32_t i = 0; _vgpu_count_of(drivers); i++) {
            if (drivers[i]->backendType == info->preferred_backend && drivers[i]->is_supported()) {
                s_gpu_renderer = drivers[i]->init_renderer();
                break;
            }
        }
    }

    vgpu_init_info def = _vgpu_config_defaults(info);
    if (!s_gpu_renderer->init(&def)) {
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

vgpu_caps vgpu_query_caps() {
    return s_gpu_renderer->query_caps();
}

void vgpu_begin_frame(void) {
    s_gpu_renderer->begin_frame();
}

void vgpu_end_frame(void) {
    s_gpu_renderer->end_frame();
}

/* Texture */
static vgpu_texture_info _vgpu_texture_info_def(const vgpu_texture_info* info) {
    vgpu_texture_info def = *info;
    def.type = _vgpu_def(info->type, VGPU_TEXTURE_TYPE_2D);
    def.format = _vgpu_def(info->format, VGPU_PIXEL_FORMAT_RGBA8_UNORM);
    def.width = _vgpu_def(info->width, 1u);
    def.height = _vgpu_def(info->height, 1u);
    def.depth = _vgpu_def(info->depth, 1u);
    def.array_layers = _vgpu_def(info->array_layers, 1u);
    def.mip_levels = _vgpu_def(info->mip_levels, 1u);
    return def;
}

vgpu_texture vgpu_create_texture(const vgpu_texture_info* info) {
    //VGPU_ASSERT(s_gpu_renderer);
    VGPU_ASSERT(info);

    vgpu_texture_info def = _vgpu_texture_info_def(info);
    //return s_gpu_renderer->texture_create(&def);
    return nullptr;
}

void vgpu_destroy_texture(vgpu_texture texture) {
    //VGPU_ASSERT(s_gpu_renderer);
    VGPU_ASSERT(texture);

    //s_gpu_renderer->texture_destroy(texture);
}

vgpu_texture_info vgpu_query_texture_info(vgpu_texture texture) {
    //VGPU_ASSERT(s_gpu_renderer);
    //return s_gpu_renderer->query_texture_info(texture);
    return {};
}

vgpu_texture vgpu_get_backbuffer_texture(void) {
    //return s_gpu_renderer->get_backbuffer_texture();
    return nullptr;
}

void vgpu_begin_pass(const vgpu_pass_begin_info* info) {
}

void vgpu_end_pass(void) {
}

/* Pixel format helpers */
struct pixel_format_desc
{
    vgpu_pixel_format format;
    const char* name;
    uint32_t bytesPerBlock;
    uint32_t channelCount;
    vgpu_pixel_format_type Type;
    struct
    {
        bool depth;
        bool stencil;
        bool compressed;
    };
    struct
    {
        uint32_t width;
        uint32_t height;
    } compression_ratio;
    int numChannelBits[4];
};

const pixel_format_desc k_format_desc[] =
{
    // Format, Name, BytesPerBlock, ChannelCount, Type, {depth, stencil, compressed}, {CompressionRatio.Width,CompressionRatio.Height}, {numChannelBits.x, numChannelBits.y, numChannelBits.z, numChannelBits.w}
    {VGPU_PIXEL_FORMAT_UNDEFINED,   "Unknown",      0,      0,  VGPU_PIXEL_FORMAT_TYPE_UNKNOWN,    {false, false, false},       {1, 1},     {0, 0, 0, 0}},
    {VGPU_PIXEL_FORMAT_R8_UNORM,    "R8Unorm",      1,      1,  VGPU_PIXEL_FORMAT_TYPE_UNORM,      {false, false, false},       {1, 1},     {8, 0, 0, 0}},
    {VGPU_PIXEL_FORMAT_R8_SNORM,    "R8Snorm",      1,      1,  VGPU_PIXEL_FORMAT_TYPE_SNORM,      {false, false, false},       {1, 1},     {8, 0, 0, 0}},
    {VGPU_PIXEL_FORMAT_R8_UINT,     "R8Int",        1,      1,  VGPU_PIXEL_FORMAT_TYPE_SINT,       {false, false, false},       {1, 1},     {8, 0, 0, 0}},
    {VGPU_PIXEL_FORMAT_R8_SINT,     "R8Uint",       1,      1,  VGPU_PIXEL_FORMAT_TYPE_UINT,       {false, false, false},       {1, 1},     {8, 0, 0, 0}},

    {VGPU_PIXEL_FORMAT_R16_UINT,    "R16Uint",      2,      1,  VGPU_PIXEL_FORMAT_TYPE_UINT,       {false, false, false},      {1, 1},     {16, 0, 0, 0}},
    {VGPU_PIXEL_FORMAT_R16_SINT,    "R16Int",       2,      1,  VGPU_PIXEL_FORMAT_TYPE_SINT,       {false, false, false},      {1, 1},     {16, 0, 0, 0}},
    {VGPU_PIXEL_FORMAT_R16_FLOAT,   "R16Float",     2,      1,  VGPU_PIXEL_FORMAT_TYPE_FLOAT,      {false, false, false},      {1, 1},     {16, 0, 0, 0}},
    {VGPU_PIXEL_FORMAT_RG8_UNORM,   "RG8Unorm",     2,      2,  VGPU_PIXEL_FORMAT_TYPE_UNORM,      {false, false, false},      {1, 1},     {8, 8, 0, 0}},
    {VGPU_PIXEL_FORMAT_RG8_SNORM,   "RG8Snorm",     2,      2,  VGPU_PIXEL_FORMAT_TYPE_SNORM,      {false, false, false},      {1, 1},     {8, 8, 0, 0}},
    {VGPU_PIXEL_FORMAT_RG8_UINT,    "RG8Uint",      2,      2,  VGPU_PIXEL_FORMAT_TYPE_UINT,       {false, false, false},      {1, 1},     {8, 8, 0, 0}},
    {VGPU_PIXEL_FORMAT_RG8_SINT,    "RG8Int",       2,      2,  VGPU_PIXEL_FORMAT_TYPE_SINT,       {false, false, false},      {1, 1},     {8, 8, 0, 0}},

    {VGPU_PIXEL_FORMAT_R32_UINT,    "R32Uint",      4,      1,  VGPU_PIXEL_FORMAT_TYPE_UINT,       {false, false, false},      {1, 1},     {32, 0, 0, 0}},
    {VGPU_PIXEL_FORMAT_R32_SINT,    "R32Int",       4,      1,  VGPU_PIXEL_FORMAT_TYPE_SINT,       {false, false, false},      {1, 1},     {32, 0, 0, 0}},
    {VGPU_PIXEL_FORMAT_R32_FLOAT,   "R32Float",     4,      1,  VGPU_PIXEL_FORMAT_TYPE_FLOAT,      {false, false, false},      {1, 1},     {32, 0, 0, 0}},

    {VGPU_PIXEL_FORMAT_RG16_UINT,           "RG16Uint",         4,      2,  VGPU_PIXEL_FORMAT_TYPE_UINT,       {false,  false, false},      {1, 1},     {16, 16, 0, 0}},
    {VGPU_PIXEL_FORMAT_RG16_SINT,           "RG16Int",          4,      2,  VGPU_PIXEL_FORMAT_TYPE_SINT,       {false,  false, false},      {1, 1},     {16, 16, 0, 0}},
    {VGPU_PIXEL_FORMAT_RG16_FLOAT,          "RG16Float",        4,      2,  VGPU_PIXEL_FORMAT_TYPE_FLOAT,      {false,  false, false},      {1, 1},     {16, 16, 0, 0}},
    {VGPU_PIXEL_FORMAT_RGBA8_UNORM,         "RGBA8Unorm",       4,      4,  VGPU_PIXEL_FORMAT_TYPE_UNORM,      {false,  false, false},      {1, 1},     {8, 8, 8, 8}},
    {VGPU_PIXEL_FORMAT_RGBA8_UNORM_SRGB,    "RGBA8UnormSrgb",   4,      4,  VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,  {false, false, false},      {1, 1},     {8, 8, 8, 8}},
    {VGPU_PIXEL_FORMAT_RGBA8_SNORM,         "RGBA8Snorm",       4,      4,  VGPU_PIXEL_FORMAT_TYPE_SNORM,      {false, false, false},       {1, 1},     {8, 8, 8, 8}},
    {VGPU_PIXEL_FORMAT_RGBA8_UINT,          "RGBA8Uint",        4,      4,  VGPU_PIXEL_FORMAT_TYPE_UINT,       {false, false, false},       {1, 1},     {8, 8, 8, 8}},
    {VGPU_PIXEL_FORMAT_RGBA8_SINT,          "RGBA8Sint",        4,      4,  VGPU_PIXEL_FORMAT_TYPE_SINT,       {false, false, false},       {1, 1},     {8, 8, 8, 8}},
    {VGPU_PIXEL_FORMAT_BGRA8_UNORM,         "BGRA8Unorm",       4,      4,  VGPU_PIXEL_FORMAT_TYPE_UNORM,      {false, false, false},       {1, 1},     {8, 8, 8, 8}},
    {VGPU_PIXEL_FORMAT_BGRA8_UNORM_SRGB,    "BGRA8UnormSrgb",   4,      4,  VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,  {false, false, false},      {1, 1},     {8, 8, 8, 8}},

    {VGPU_PIXEL_FORMAT_RGB10A2_UNORM,       "RGB10A2Unorm",     4,      4,  VGPU_PIXEL_FORMAT_TYPE_UNORM,       {false,  false, false},     {1, 1},     {10, 10, 10, 2}},
    {VGPU_PIXEL_FORMAT_RG11B10_FLOAT,       "RG11B10Float",     4,      3,  VGPU_PIXEL_FORMAT_TYPE_FLOAT,       {false,  false, false},     {1, 1},     {11, 11, 10, 0}},

    {VGPU_PIXEL_FORMAT_RG32_UINT,           "RG32Uint",         8,      2,  VGPU_PIXEL_FORMAT_TYPE_UINT,       {false,  false, false},      {1, 1},     {32, 32, 0, 0}},
    {VGPU_PIXEL_FORMAT_RG32_SINT,           "RG32Sin",          8,      2,  VGPU_PIXEL_FORMAT_TYPE_SINT,       {false,  false, false},      {1, 1},     {32, 32, 0, 0}},
    {VGPU_PIXEL_FORMAT_RG32_FLOAT,          "RG32Float",        8,      2,  VGPU_PIXEL_FORMAT_TYPE_FLOAT,      {false,  false, false},      {1, 1},     {32, 32, 0, 0}},
    {VGPU_PIXEL_FORMAT_RGBA16_UINT,         "RGBA16Uint",       8,      4,  VGPU_PIXEL_FORMAT_TYPE_UINT,       {false,  false, false},      {1, 1},     {16, 16, 16, 16}},
    {VGPU_PIXEL_FORMAT_RGBA16_SINT,         "RGBA16Sint",       8,      4,  VGPU_PIXEL_FORMAT_TYPE_SINT,       {false,  false, false},      {1, 1},     {16, 16, 16, 16}},
    {VGPU_PIXEL_FORMAT_RGBA16_FLOAT,        "RGBA16Float",      8,      4,  VGPU_PIXEL_FORMAT_TYPE_FLOAT,      {false,  false, false},      {1, 1},     {16, 16, 16, 16}},

    {VGPU_PIXEL_FORMAT_RGBA32_UINT,         "RGBA32Uint",       16,     4,  VGPU_PIXEL_FORMAT_TYPE_UINT,       {false,  false, false},      {1, 1},     {32, 32, 32, 32}},
    {VGPU_PIXEL_FORMAT_RGBA32_SINT,         "RGBA32Int",        16,     4,  VGPU_PIXEL_FORMAT_TYPE_SINT,       {false,  false, false},      {1, 1},     {32, 32, 32, 32}},
    {VGPU_PIXEL_FORMAT_RGBA32_FLOAT,        "RGBA32Float",      16,     4,  VGPU_PIXEL_FORMAT_TYPE_FLOAT,      {false,  false, false},      {1, 1},     {32, 32, 32, 32}},

    {VGPU_PIXEL_FORMAT_DEPTH32_FLOAT,       "D32Float",             4,  1,  VGPU_PIXEL_FORMAT_TYPE_FLOAT,      {true, false, false},        {1, 1},     {32, 0, 0, 0}},
    {VGPU_PIXEL_FORMAT_DEPTH24_STENCIL8,    "Depth24PlusStencil8",  4,  2,  VGPU_PIXEL_FORMAT_TYPE_UNORM,      {true, true, false},         {1, 1},     {24, 8, 0, 0}},

    {VGPU_PIXEL_FORMAT_BC1,                 "BC1RGBAUnorm",         8,      3,  VGPU_PIXEL_FORMAT_TYPE_UNORM,       {false,  false, true},      {4, 4},     {64, 0, 0, 0}},
    {VGPU_PIXEL_FORMAT_BC1_SRGB,            "BC1RGBAUnormSrgb",     8,      3,  VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,  {false,  false, true},      {4, 4},     {64, 0, 0, 0}},
    {VGPU_PIXEL_FORMAT_BC2,                 "BC2RGBAUnorm",         16,     4,  VGPU_PIXEL_FORMAT_TYPE_UNORM,       {false,  false, true},      {4, 4},     {128, 0, 0, 0}},
    {VGPU_PIXEL_FORMAT_BC2_SRGB,            "BC2RGBAUnormSrgb",     16,     4,  VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,  {false,  false, true},      {4, 4},     {128, 0, 0, 0}},
    {VGPU_PIXEL_FORMAT_BC3,                 "BC3RGBAUnorm",         16,     4,  VGPU_PIXEL_FORMAT_TYPE_UNORM,       {false,  false, true},      {4, 4},     {128, 0, 0, 0}},
    {VGPU_PIXEL_FORMAT_BC3_SRGB,            "BC3RGBAUnormSrgb",     16,     4,  VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,  {false,  false, true},      {4, 4},     {128, 0, 0, 0}},
    {VGPU_PIXEL_FORMAT_BC4R_UNORM,          "BC4RUnorm",            8,      1,  VGPU_PIXEL_FORMAT_TYPE_UNORM,       {false,  false, true},      {4, 4},     {64, 0, 0, 0}},
    {VGPU_PIXEL_FORMAT_BC4R_SNORM,          "BC4RSnorm",            8,      1,  VGPU_PIXEL_FORMAT_TYPE_SNORM,       {false,  false, true},      {4, 4},     {64, 0, 0, 0}},
    {VGPU_PIXEL_FORMAT_BC5RG_UNORM,         "BC5RGUnorm",           16,     2,  VGPU_PIXEL_FORMAT_TYPE_UNORM,       {false,  false, true},      {4, 4},     {128, 0, 0, 0}},
    {VGPU_PIXEL_FORMAT_BC5RG_SNORM,         "BC5RGSnorm",           16,     2,  VGPU_PIXEL_FORMAT_TYPE_SNORM,       {false,  false, true},      {4, 4},     {128, 0, 0, 0}},
    {VGPU_PIXEL_FORMAT_BC6HRGB_UFLOAT,      "BC6HU16",              16,     3,  VGPU_PIXEL_FORMAT_TYPE_FLOAT,       {false,  false, true},      {4, 4},     {128, 0, 0, 0}},
    {VGPU_PIXEL_FORMAT_BC6HRGB_SFLOAT,      "BC6HS16",              16,     3,  VGPU_PIXEL_FORMAT_TYPE_FLOAT,       {false,  false, true},      {4, 4},     {128, 0, 0, 0}},
    {VGPU_PIXEL_FORMAT_BC7RGBA_UNORM,       "BC7RGBAUnorm",         16,     4,  VGPU_PIXEL_FORMAT_TYPE_UNORM,       {false,  false, true},      {4, 4},     {128, 0, 0, 0}},
    {VGPU_PIXEL_FORMAT_BC7RGBA_UNORM_SRGB,  "BC7RGBAUnormSrgb",     16,     4,  VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,  {false,  false, true},      {4, 4},     {128, 0, 0, 0}},
};

static_assert(_vgpu_count_of(k_format_desc) == _VGPU_PIXEL_FORMAT_COUNT, "Format desc table has a wrong size");


uint32_t vgpu_get_format_bytes_per_block(vgpu_pixel_format format) {
    VGPU_ASSERT(k_format_desc[format].format == format);
    return k_format_desc[format].bytesPerBlock;
}

uint32_t vgpu_get_format_pixels_per_block(vgpu_pixel_format format) {
    VGPU_ASSERT(k_format_desc[format].format == format);
    return k_format_desc[format].compression_ratio.width * k_format_desc[(uint32_t)format].compression_ratio.height;
}

VGPU_API bool vgpu_is_depth_format(vgpu_pixel_format format) {
    VGPU_ASSERT(k_format_desc[format].format == format);
    return k_format_desc[format].depth;
}

VGPU_API bool vgpu_is_stencil_format(vgpu_pixel_format format) {
    VGPU_ASSERT(k_format_desc[format].format == format);
    return k_format_desc[format].stencil;
}

VGPU_API bool vgpu_is_depth_stencil_format(vgpu_pixel_format format) {
    return vgpu_is_depth_format(format) || vgpu_is_stencil_format(format);
}

VGPU_API bool vgpu_is_compressed_format(vgpu_pixel_format format) {
    VGPU_ASSERT(k_format_desc[format].format == format);
    return k_format_desc[format].compressed;
}
