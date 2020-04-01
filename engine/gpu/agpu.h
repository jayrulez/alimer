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

#ifndef AGPU_H_
#define AGPU_H_

#if defined(AGPU_SHARED_LIBRARY)
#   if defined(_WIN32)
#       if defined(AGPU_IMPLEMENTATION)
#           define AGPU_EXPORT __declspec(dllexport)
#       else
#           define AGPU_EXPORT __declspec(dllimport)
#       endif
#   else  // defined(_WIN32)
#       if defined(AGPU_IMPLEMENTATION)
#           define AGPU_EXPORT __attribute__((visibility("default")))
#       else
#           define AGPU_EXPORT
#       endif
#   endif  // defined(_WIN32)
#else       // defined(AGPU_SHARED_LIBRARY)
#   define AGPU_EXPORT
#endif  // defined(AGPU_SHARED_LIBRARY)

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct agpu_device agpu_device;
typedef struct agpu_context agpu_context;
typedef struct agpu_buffer agpu_buffer;
typedef struct gpu_swapchain { uint32_t id; } gpu_swapchain;

enum {
    AGPU_INVALID_ID = 0,
    AGPU_MAX_COLOR_ATTACHMENTS = 8u,
    AGPU_MAX_VERTEX_BUFFER_BINDINGS = 8u,
    AGPU_MAX_VERTEX_ATTRIBUTES = 16u,
    AGPU_MAX_VERTEX_ATTRIBUTE_OFFSET = 2047u,
    AGPU_MAX_VERTEX_BUFFER_STRIDE = 2048u,
    // Maximum object that can be created.
    AGPU_MAX_COMMAND_BUFFERS = 16u,
    AGPU_MAX_TEXTURES = 4096u,
    AGPU_MAX_BUFFERS = 4096u,
    AGPU_MAX_SAMPLERS = 4096u,
    AGPU_MAX_SHADERS = 512u,
    AGPU_MAX_FRAMEBUFFERS = 256u,
    AGPU_MAX_PIPELINES = 256u,
};

typedef enum agpu_log_level {
    AGPU_LOG_LEVEL_VERBOSE = 0,
    AGPU_LOG_LEVEL_INFO = 1,
    AGPU_LOG_LEVEL_WARNING = 2,
    AGPU_LOG_LEVEL_ERROR = 3,
    _AGPU_LOG_LEVEL_COUNT,
    _AGPU_LOG_LEVEL_FORCE_U32 = 0x7FFFFFFF
} agpu_log_level;

typedef enum agpu_backend {
    AGPU_BACKEND_DEFAULT = 0,
    AGPU_BACKEND_NULL,
    AGPU_BACKEND_VULKAN,
    AGPU_BACKEND_DIRECT3D12,
    AGPU_BACKEND_DIRECT3D11,
    AGPU_BACKEND_OPENGL,
    AGPU_BACKEND_COUNT
} agpu_backend;

typedef enum agpu_adapter_type {
    AGPU_ADAPTER_TYPE_DISCRETE_GPU = 0,
    AGPU_ADAPTER_TYPE_INTEGRATED_GPU = 1,
    AGPU_ADAPTER_TYPE_CPU = 2,
    AGPU_ADAPTER_TYPE_UNKNOW = 3,
    _AGPU_ADAPTER_TYPE_COUNT,
    _AGPU_ADAPTER_TYPE_FORCE_U32 = 0x7FFFFFFF
} agpu_adapter_type;

typedef enum agpu_queue_type {
    AGPU_QUEUE_TYPE_GRAPHICS = 0,
    AGPU_QUEUE_TYPE_COMPUTE,
    AGPU_QUEUE_TYPE_COPY,
    _AGPU_QUEUE_TYPE_COUNT,
    _AGPU_QUEUE_TYPE_FORCE_U32 = 0x7FFFFFFF
} agpu_queue_type;

/// Defines pixel format.
typedef enum agpu_pixel_format {
    AGPU_PIXEL_FORMAT_UNDEFINED = 0,
    // 8-bit pixel formats
    AGPU_PIXEL_FORMAT_R8_UNORM,
    AGPU_PIXEL_FORMAT_R8_SNORM,
    AGPU_PIXEL_FORMAT_R8_UINT,
    AGPU_PIXEL_FORMAT_R8_SINT,

    // 16-bit pixel formats
    AGPU_PIXEL_FORMAT_R16_UNORM,
    AGPU_PIXEL_FORMAT_R16_SNORM,
    AGPU_PIXEL_FORMAT_R16_UINT,
    AGPU_PIXEL_FORMAT_R16_SINT,
    AGPU_PIXEL_FORMAT_R16_FLOAT,
    AGPU_PIXEL_FORMAT_RG8_UNORM,
    AGPU_PIXEL_FORMAT_RG8_SNORM,
    AGPU_PIXEL_FORMAT_RG8_UINT,
    AGPU_PIXEL_FORMAT_RG8_SINT,

    // 32-bit pixel formats
    AGPU_PIXEL_FORMAT_R32_UINT,
    AGPU_PIXEL_FORMAT_R32_SINT,
    AGPU_PIXEL_FORMAT_R32_FLOAT,
    AGPU_PIXEL_FORMAT_RG16_UNORM,
    AGPU_PIXEL_FORMAT_RG16_SNORM,
    AGPU_PIXEL_FORMAT_RG16_UINT,
    AGPU_PIXEL_FORMAT_RG16_SINT,
    AGPU_PIXEL_FORMAT_RG16_FLOAT,
    AGPU_PIXEL_FORMAT_RGBA8_UNORM,
    AGPU_PIXEL_FORMAT_RGBA8_UNORM_SRGB,
    AGPU_PIXEL_FORMAT_RGBA8_SNORM,
    AGPU_PIXEL_FORMAT_RGBA8_UINT,
    AGPU_PIXEL_FORMAT_RGBA8_SINT,
    AGPU_PIXEL_FORMAT_BGRA8_UNORM,
    AGPU_PIXEL_FORMAT_BGRA8_UNORM_SRGB,

    // Packed 32-Bit Pixel formats
    AGPU_PIXEL_FORMAT_RGB10A2_UNORM,
    AGPU_PIXEL_FORMAT_RG11B10_FLOAT,

    // 64-Bit Pixel Formats
    AGPU_PIXEL_FORMAT_RG32_UINT,
    AGPU_PIXEL_FORMAT_RG32_SINT,
    AGPU_PIXEL_FORMAT_RG32_FLOAT,
    AGPU_PIXEL_FORMAT_RGBA16_UNORM,
    AGPU_PIXEL_FORMAT_RGBA16_SNORM,
    AGPU_PIXEL_FORMAT_RGBA16_UINT,
    AGPU_PIXEL_FORMAT_RGBA16_SINT,
    AGPU_PIXEL_FORMAT_RGBA16_FLOAT,

    // 128-Bit Pixel Formats
    AGPU_PIXEL_FORMAT_RGBA32_UINT,
    AGPU_PIXEL_FORMAT_RGBA32_SINT,
    AGPU_PIXEL_FORMAT_RGBA32_FLOAT,

    // Depth-stencil
    AGPU_PIXEL_FORMAT_D16_UNORM,
    AGPU_PIXEL_FORMAT_D32_FLOAT,
    AGPU_PIXEL_FORMAT_D24_UNORM_S8_UINT,
    AGPU_PIXEL_FORMAT_D32_FLOAT_S8_UINT,

    // Compressed formats
    AGPU_PIXEL_FORMAT_BC1_RGBA_UNORM,
    AGPU_PIXEL_FORMAT_BC1_RGBA_UNORM_SRGB,
    AGPU_PIXEL_FORMAT_BC2_RGBA_UNORM,
    AGPU_PIXEL_FORMAT_BC2_RGBA_UNORM_SRGB,
    AGPU_PIXEL_FORMAT_BC3_RGBA_UNORM,
    AGPU_PIXEL_FORMAT_BC3_RGBA_UNORM_SRGB,
    AGPU_PIXEL_FORMAT_BC4_R_UNORM,
    AGPU_PIXEL_FORMAT_BC4_R_SNORM,
    AGPU_PIXEL_FORMAT_BC5_RG_UNORM,
    AGPU_PIXEL_FORMAT_BC5_RG_SNORM,
    AGPU_PIXEL_FORMAT_BC6H_RGB_FLOAT,
    AGPU_PIXEL_FORMAT_BC6H_RGB_UFLOAT,
    AGPU_PIXEL_FORMAT_BC7_RGBA_UNORM,
    AGPU_PIXEL_FORMAT_BC7_RGBA_UNORM_SRGB,

    // Compressed PVRTC Pixel Formats
    AGPU_PIXEL_FORMAT_PVRTC_RGB2,
    AGPU_PIXEL_FORMAT_PVRTC_RGBA2,
    AGPU_PIXEL_FORMAT_PVRTC_RGB4,
    AGPU_PIXEL_FORMAT_PVRTC_RGBA4,

    // Compressed ETC Pixel Formats
    AGPU_PIXEL_FORMAT_ETC2_RGB8,
    AGPU_PIXEL_FORMAT_ETC2_RGB8_SRGB,
    AGPU_PIXEL_FORMAT_ETC2_RGB8A1,
    AGPU_PIXEL_FORMAT_ETC2_RGB8A1_SRGB,

    // Compressed ASTC Pixel Formats
    AGPU_PIXEL_FORMAT_ASTC4x4,
    AGPU_PIXEL_FORMAT_ASTC5x5,
    AGPU_PIXEL_FORMAT_ASTC6x6,
    AGPU_PIXEL_FORMAT_ASTC8x5,
    AGPU_PIXEL_FORMAT_ASTC8x6,
    AGPU_PIXEL_FORMAT_ASTC8x8,
    AGPU_PIXEL_FORMAT_ASTC10x10,
    AGPU_PIXEL_FORMAT_ASTC12x12,
    /// Pixel format count.
    AGPU_PIXEL_FORMAT_COUNT
} agpu_pixel_format;

typedef enum agpu_config_flags_bits {
    AGPU_CONFIG_FLAGS_NONE = 0,
    AGPU_CONFIG_FLAGS_VALIDATION = 0x1
} agpu_config_flags_bits;
typedef uint32_t agpu_config_flags;

typedef struct agpu_swapchain_desc {
    /// Native window handle.
    uintptr_t native_handle;

    uint32_t image_count;

    agpu_pixel_format   color_format;
    agpu_pixel_format   depth_stencil_format;
} agpu_swapchain_desc;

typedef struct agpu_buffer_desc {
    uint64_t size;
    uint32_t usage;
    const char* name;
} agpu_buffer_desc;

typedef struct agpu_features {
    bool  independent_blend;
    bool  compute_shader;
    bool  geometry_shader;
    bool  tessellation_shader;
    bool  multi_viewport;
    bool  index_uint32;
    bool  multi_draw_indirect;
    bool  fill_mode_non_solid;
    bool  sampler_anisotropy;
    bool  texture_compression_BC;
    bool  texture_compression_PVRTC;
    bool  texture_compression_ETC2;
    bool  texture_compression_ASTC;
    //bool  texture1D;
    bool  texture_3D;
    bool  texture_2D_array;
    bool  texture_cube_array;
    bool  raytracing;
} agpu_features;

typedef struct agpu_limits {
    uint32_t        max_vertex_attributes;
    uint32_t        max_vertex_bindings;
    uint32_t        max_vertex_attribute_offset;
    uint32_t        max_vertex_binding_stride;
    uint32_t        max_texture_size_1d;
    uint32_t        max_texture_size_2d;
    uint32_t        max_texture_size_3d;
    uint32_t        max_texture_size_cube;
    uint32_t        max_texture_array_layers;
    uint32_t        max_color_attachments;
    uint32_t        max_uniform_buffer_size;
    uint64_t        min_uniform_buffer_offset_alignment;
    uint32_t        max_storage_buffer_size;
    uint64_t        min_storage_buffer_offset_alignment;
    uint32_t        max_sampler_anisotropy;
    uint32_t        max_viewports;
    uint32_t        max_viewport_width;
    uint32_t        max_viewport_height;
    uint32_t        max_tessellation_patch_size;
    float           point_size_range_min;
    float           point_size_range_max;
    float           line_width_range_min;
    float           line_width_range_max;
    uint32_t        max_compute_shared_memory_size;
    uint32_t        max_compute_work_group_count_x;
    uint32_t        max_compute_work_group_count_y;
    uint32_t        max_compute_work_group_count_z;
    uint32_t        max_compute_work_group_invocations;
    uint32_t        max_compute_work_group_size_x;
    uint32_t        max_compute_work_group_size_y;
    uint32_t        max_compute_work_group_size_z;
} agpu_limits;

typedef void(*agpu_log_callback)(void* user_data, const char* message, agpu_log_level level);

typedef struct agpu_desc {
    agpu_config_flags flags;
    agpu_adapter_type preferred_adapter;
    uint32_t max_inflight_frames;
    const agpu_swapchain_desc* swapchain;
} agpu_desc;

#ifdef __cplusplus
extern "C" {
#endif

    /// Get the current log output function.
    AGPU_EXPORT void agpu_get_log_callback_function(agpu_log_callback* callback, void** user_data);
    /// Set the current log output function.
    AGPU_EXPORT void agpu_set_log_callback_function(agpu_log_callback callback, void* user_data);

    AGPU_EXPORT void agpu_log(const char* message, agpu_log_level level);
    AGPU_EXPORT void agpu_log_format(agpu_log_level level, const char* format, ...);

    AGPU_EXPORT agpu_device* agpu_create_device(const char* application_name, const agpu_desc* desc);
    AGPU_EXPORT void agpu_destroy_device(agpu_device* device);
    AGPU_EXPORT void agpu_wait_idle(agpu_device* device);
    AGPU_EXPORT void agpu_begin_frame(agpu_device* device);
    AGPU_EXPORT void agpu_end_frame(agpu_device* device);

    AGPU_EXPORT agpu_backend agpu_query_backend(agpu_device* device);
    AGPU_EXPORT agpu_features agpu_query_features(agpu_device* device);
    AGPU_EXPORT agpu_limits agpu_query_limits(agpu_device* device);

    AGPU_EXPORT agpu_buffer* agpu_create_buffer(agpu_device* device, const agpu_buffer_desc* desc);
    AGPU_EXPORT void agpu_destroy_buffer(agpu_device* device, agpu_buffer* buffer);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // AGPU_H_