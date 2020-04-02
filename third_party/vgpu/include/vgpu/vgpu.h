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

#ifndef VGPU_H_
#define VGPU_H_

#if defined(VGPU_SHARED_LIBRARY)
#   if defined(_WIN32)
#       if defined(VGPU_IMPLEMENTATION)
#           define VGPU_EXPORT __declspec(dllexport)
#       else
#           define VGPU_EXPORT __declspec(dllimport)
#       endif
#   else  // defined(_WIN32)
#       if defined(VGPU_IMPLEMENTATION)
#           define VGPU_EXPORT __attribute__((visibility("default")))
#       else
#           define VGPU_EXPORT
#       endif
#   endif  // defined(_WIN32)
#else       // defined(VGPU_SHARED_LIBRARY)
#   define VGPU_EXPORT
#endif  // defined(VGPU_SHARED_LIBRARY)

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct agpu_device agpu_device;
typedef struct vgpu_context vgpu_context;
typedef struct vgpu_buffer vgpu_buffer;
typedef struct vgpu_texture vgpu_texture;
typedef struct vgpu_command_buffer vgpu_command_buffer;

enum {
    VGPU_MAX_COLOR_ATTACHMENTS = 8u,
    VGPU_MAX_VERTEX_BUFFER_BINDINGS = 8u,
    VGPU_MAX_VERTEX_ATTRIBUTES = 16u,
    VGPU_MAX_VERTEX_ATTRIBUTE_OFFSET = 2047u,
    VGPU_MAX_VERTEX_BUFFER_STRIDE = 2048u,

    /// Maximum commands buffers per frame.
    VGPU_MAX_SUBMITTED_COMMAND_BUFFERS = 16u,
};

typedef enum vgpu_log_level {
    VGPU_LOG_LEVEL_OFF = 0,
    VGPU_LOG_LEVEL_VERBOSE,
    VGPU_LOG_LEVEL_DEBUG,
    VGPU_LOG_LEVEL_INFO,
    VGPU_LOG_LEVEL_WARN,
    VGPU_LOG_LEVEL_ERROR,
    VGPU_LOG_LEVEL_CRITICAL,
    VGPU_LOG_LEVEL_COUNT,
    _VGPU_LOG_LEVEL_FORCE_U32 = 0x7FFFFFFF
} vgpu_log_level;

typedef enum vgpu_backend {
    VGPU_BACKEND_DEFAULT = 0,
    VGPU_BACKEND_NULL,
    VGPU_BACKEND_VULKAN,
    VGPU_BACKEND_DIRECT3D12,
    VGPU_BACKEND_DIRECT3D11,
    VGPU_BACKEND_OPENGL,
    VGPU_BACKEND_COUNT
} vgpu_backend;

typedef enum vgpu_adapter_type {
    VGPU_ADAPTER_TYPE_DISCRETE_GPU = 0,
    VGPU_ADAPTER_TYPE_INTEGRATED_GPU = 1,
    VGPU_ADAPTER_TYPE_CPU = 2,
    VGPU_ADAPTER_TYPE_UNKNOW = 3,
    _VGPU_ADAPTER_TYPE_COUNT,
    _VGPU_ADAPTER_TYPE_FORCE_U32 = 0x7FFFFFFF
} vgpu_adapter_type;

typedef enum vgpu_queue_type {
    VGPU_QUEUE_TYPE_GRAPHICS = 0,
    VGPU_QUEUE_TYPE_COMPUTE,
    VGPU_QUEUE_TYPE_COPY,
    _VGPU_QUEUE_TYPE_COUNT,
    _VGPU_QUEUE_TYPE_FORCE_U32 = 0x7FFFFFFF
} vgpu_queue_type;

/// Defines pixel format.
typedef enum vgpu_pixel_format {
    VGPU_PIXEL_FORMAT_UNDEFINED = 0,
    // 8-bit pixel formats
    VGPU_PIXEL_FORMAT_R8_UNORM,
    VGPU_PIXEL_FORMAT_R8_SNORM,
    VGPU_PIXEL_FORMAT_R8_UINT,
    VGPU_PIXEL_FORMAT_R8_SINT,

    // 16-bit pixel formats
    VGPU_PIXEL_FORMAT_R16_UNORM,
    VGPU_PIXEL_FORMAT_R16_SNORM,
    VGPU_PIXEL_FORMAT_R16_UINT,
    VGPU_PIXEL_FORMAT_R16_SINT,
    VGPU_PIXEL_FORMAT_R16_FLOAT,
    VGPU_PIXEL_FORMAT_RG8_UNORM,
    VGPU_PIXEL_FORMAT_RG8_SNORM,
    VGPU_PIXEL_FORMAT_RG8_UINT,
    VGPU_PIXEL_FORMAT_RG8_SINT,

    // 32-bit pixel formats
    VGPU_PIXEL_FORMAT_R32_UINT,
    VGPU_PIXEL_FORMAT_R32_SINT,
    VGPU_PIXEL_FORMAT_R32_FLOAT,
    VGPU_PIXEL_FORMAT_RG16_UNORM,
    VGPU_PIXEL_FORMAT_RG16_SNORM,
    VGPU_PIXEL_FORMAT_RG16_UINT,
    VGPU_PIXEL_FORMAT_RG16_SINT,
    VGPU_PIXEL_FORMAT_RG16_FLOAT,
    VGPU_PIXEL_FORMAT_RGBA8_UNORM,
    VGPU_PIXEL_FORMAT_RGBA8_UNORM_SRGB,
    VGPU_PIXEL_FORMAT_RGBA8_SNORM,
    VGPU_PIXEL_FORMAT_RGBA8_UINT,
    VGPU_PIXEL_FORMAT_RGBA8_SINT,
    VGPU_PIXEL_FORMAT_BGRA8_UNORM,
    VGPU_PIXEL_FORMAT_BGRA8_UNORM_SRGB,

    // Packed 32-Bit Pixel formats
    VGPU_PIXEL_FORMAT_RGB10A2_UNORM,
    VGPU_PIXEL_FORMAT_RG11B10_FLOAT,

    // 64-Bit Pixel Formats
    VGPU_PIXEL_FORMAT_RG32_UINT,
    VGPU_PIXEL_FORMAT_RG32_SINT,
    VGPU_PIXEL_FORMAT_RG32_FLOAT,
    VGPU_PIXEL_FORMAT_RGBA16_UNORM,
    VGPU_PIXEL_FORMAT_RGBA16_SNORM,
    VGPU_PIXEL_FORMAT_RGBA16_UINT,
    VGPU_PIXEL_FORMAT_RGBA16_SINT,
    VGPU_PIXEL_FORMAT_RGBA16_FLOAT,

    // 128-Bit Pixel Formats
    VGPU_PIXEL_FORMAT_RGBA32_UINT,
    VGPU_PIXEL_FORMAT_RGBA32_SINT,
    VGPU_PIXEL_FORMAT_RGBA32_FLOAT,

    // Depth-stencil
    VGPU_PIXEL_FORMAT_D16_UNORM,
    VGPU_PIXEL_FORMAT_D32_FLOAT,
    VGPU_PIXEL_FORMAT_D24_UNORM_S8_UINT,
    VGPU_PIXEL_FORMAT_D32_FLOAT_S8_UINT,

    // Compressed formats
    VGPU_PIXEL_FORMAT_BC1_RGBA_UNORM,
    VGPU_PIXEL_FORMAT_BC1_RGBA_UNORM_SRGB,
    VGPU_PIXEL_FORMAT_BC2_RGBA_UNORM,
    VGPU_PIXEL_FORMAT_BC2_RGBA_UNORM_SRGB,
    VGPU_PIXEL_FORMAT_BC3_RGBA_UNORM,
    VGPU_PIXEL_FORMAT_BC3_RGBA_UNORM_SRGB,
    VGPU_PIXEL_FORMAT_BC4_R_UNORM,
    VGPU_PIXEL_FORMAT_BC4_R_SNORM,
    VGPU_PIXEL_FORMAT_BC5_RG_UNORM,
    VGPU_PIXEL_FORMAT_BC5_RG_SNORM,
    VGPU_PIXEL_FORMAT_BC6H_RGB_FLOAT,
    VGPU_PIXEL_FORMAT_BC6H_RGB_UFLOAT,
    VGPU_PIXEL_FORMAT_BC7_RGBA_UNORM,
    VGPU_PIXEL_FORMAT_BC7_RGBA_UNORM_SRGB,

    // Compressed PVRTC Pixel Formats
    VGPU_PIXEL_FORMAT_PVRTC_RGB2,
    VGPU_PIXEL_FORMAT_PVRTC_RGBA2,
    VGPU_PIXEL_FORMAT_PVRTC_RGB4,
    VGPU_PIXEL_FORMAT_PVRTC_RGBA4,

    // Compressed ETC Pixel Formats
    VGPU_PIXEL_FORMAT_ETC2_RGB8,
    VGPU_PIXEL_FORMAT_ETC2_RGB8_SRGB,
    VGPU_PIXEL_FORMAT_ETC2_RGB8A1,
    VGPU_PIXEL_FORMAT_ETC2_RGB8A1_SRGB,

    // Compressed ASTC Pixel Formats
    VGPU_PIXEL_FORMAT_ASTC4x4,
    VGPU_PIXEL_FORMAT_ASTC5x5,
    VGPU_PIXEL_FORMAT_ASTC6x6,
    VGPU_PIXEL_FORMAT_ASTC8x5,
    VGPU_PIXEL_FORMAT_ASTC8x6,
    VGPU_PIXEL_FORMAT_ASTC8x8,
    VGPU_PIXEL_FORMAT_ASTC10x10,
    VGPU_PIXEL_FORMAT_ASTC12x12,
    /// Pixel format count.
    VGPU_PIXEL_FORMAT_COUNT,
    _VGPU_PIXEL_FORMAT_FORCE_U32 = 0x7FFFFFFF
} vgpu_pixel_format;

/// Defines pixel format type.
typedef enum vgpu_pixel_format_type {
    /// Unknown format Type
    VGPU_PIXEL_FORMAT_TYPE_UNKNOWN = 0,
    /// _FLOATing-point formats.
    VGPU_PIXEL_FORMAT_TYPE_FLOAT,
    /// Unsigned normalized formats.
    VGPU_PIXEL_FORMAT_TYPE_UNORM,
    /// Unsigned normalized SRGB formats
    VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,
    /// Signed normalized formats.
    VGPU_PIXEL_FORMAT_TYPE_SNORM,
    /// Unsigned integer formats.
    VGPU_PIXEL_FORMAT_TYPE_UINT,
    /// Signed integer formats.
    VGPU_PIXEL_FORMAT_TYPE_SINT,
    /// PixelFormat type count.
    VGPU_PIXEL_FORMAT_TYPE_COUNT,
    _VGPU_PIXEL_FORMAT_TYPE_FORCE_U32 = 0x7FFFFFFF
} vgpu_pixel_format_type;

typedef enum vgpu_present_mode {
    VGPU_PRESENT_MODE_IMMEDIATE = 0,
    VGPU_PRESENT_MODE_MAILBOX = 1,
    VGPU_PRESENT_MODE_FIFO = 2,
    _VGPU_PRESENT_MODE_COUNT,
    _VGPU_PRESENT_MODE_FORCE_U32 = 0x7FFFFFFF
} vgpu_present_mode;

typedef enum vgpu_buffer_usage {
    VPU_BUFFER_USAGE_NONE = 0,
    VPU_BUFFER_USAGE_COPY_SRC = 0x00000001,
    VPU_BUFFER_USAGE_COPY_DEST = 0x00000002,
    VPU_BUFFER_USAGE_INDEX = 0x00000004,
    VPU_BUFFER_USAGE_VERTEX = 0x00000008,
    VPU_BUFFER_USAGE_UNIFORM = 0x00000010,
    VPU_BUFFER_USAGE_STORAGE = 0x00000008,
    VPU_BUFFER_USAGE_INDIRECT = 0x00000020,
    _VPU_BUFFER_USAGE_FORCE_U32 = 0x7FFFFFFF
} vgpu_buffer_usage;
typedef uint32_t vgpu_buffer_usage_flags;

typedef enum vgpu_texture_type {
    VGPU_TEXTURE_TYPE_2D,
    VGPU_TEXTURE_TYPE_3D,
    VGPU_TEXTURE_TYPE_CUBE,
    VGPU_TEXTURE_TYPE_COUNT,
    _VGPU_TEXTURE_TYPE_COUNT_FORCE_U32 = 0x7FFFFFFF
} vgpu_texture_type;

typedef enum vgpu_texture_usage {
    VGPU_TEXTURE_USAGE_NONE = 0,
    VGPU_TEXTURE_USAGE_COPY_SRC = 0x01,
    VGPU_TEXTURE_USAGE_COPY_DEST = 0x02,
    VGPU_TEXTURE_USAGE_SAMPLED = 0x04,
    VGPU_TEXTURE_USAGE_STORAGE = 0x08,
    VGPU_TEXTURE_USAGE_OUTPUT_ATTACHMENT = 0x10,
    _VGPU_TEXTURE_USAGE_FORCE_U32 = 0x7FFFFFFF
} vgpu_texture_usage;
typedef uint32_t vgpu_texture_usage_flags;

typedef enum vgpu_config_flags_bits {
    VGPU_CONFIG_FLAGS_NONE = 0,
    VGPU_CONFIG_FLAGS_VALIDATION = 0x1
} vgpu_config_flags_bits;
typedef uint32_t vgpu_config_flags;

/* Structs */
typedef struct vgpu_extent3d {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
} vgpu_extent3d;

typedef struct vgpu_context_desc {
    /// Native window handle.
    uintptr_t native_handle;

    /// Number of inflight frames (2 by default).
    uint32_t max_inflight_frames;

    /// Preferred image count.
    uint32_t image_count;

    /// Preferred srgb color format.
    bool srgb;

    /// Preferred depth-stencil format.
    vgpu_pixel_format   depth_stencil_format;

    vgpu_present_mode present_mode;
} vgpu_context_desc;

typedef struct vgpu_buffer_desc {
    uint64_t size;
    vgpu_buffer_usage_flags usage;
    const char* name;
} vgpu_buffer_desc;

typedef struct vgpu_texture_desc {
    vgpu_texture_type type;
    vgpu_texture_usage_flags usage;
    vgpu_extent3d extent;
    vgpu_pixel_format format;
    uint32_t mip_levels;
    uint32_t sample_count;
    const char* name;
} vgpu_texture_desc;

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

typedef void(*vgpu_log_callback)(void* user_data, const char* message, vgpu_log_level level);

typedef struct agpu_desc {
    vgpu_config_flags flags;
    vgpu_adapter_type preferred_adapter;
    const vgpu_context_desc* main_context_desc;
} agpu_desc;

#ifdef __cplusplus
extern "C" {
#endif

    /// Get the current log output function.
    VGPU_EXPORT void vgpu_get_log_callback_function(vgpu_log_callback* callback, void** user_data);
    /// Set the current log output function.
    VGPU_EXPORT void vgpu_set_log_callback_function(vgpu_log_callback callback, void* user_data);

    VGPU_EXPORT void vgpu_log(vgpu_log_level level, const char* message);
    VGPU_EXPORT void vgpu_log_format(vgpu_log_level level, const char* format, ...);

    VGPU_EXPORT agpu_device* vgpu_create_device(const char* application_name, const agpu_desc* desc);
    VGPU_EXPORT void vgpu_destroy_device(agpu_device* device);
    VGPU_EXPORT void vgpu_wait_idle(agpu_device* device);
    VGPU_EXPORT void vgpu_begin_frame(agpu_device* device);
    VGPU_EXPORT void vgpu_end_frame(agpu_device* device);

    VGPU_EXPORT vgpu_context* vgpu_create_context(agpu_device* device, const vgpu_context_desc* desc);
    VGPU_EXPORT void vgpu_destroy_context(agpu_device* device, vgpu_context* context);
    VGPU_EXPORT void vgpu_set_context(agpu_device* device, vgpu_context* context);

    VGPU_EXPORT vgpu_backend vgpu_query_backend(agpu_device* device);
    VGPU_EXPORT agpu_features vgpu_query_features(agpu_device* device);
    VGPU_EXPORT agpu_limits vgpu_query_limits(agpu_device* device);

    VGPU_EXPORT vgpu_buffer* vgpu_create_buffer(agpu_device* device, const vgpu_buffer_desc* desc);
    VGPU_EXPORT void vgpu_destroy_buffer(agpu_device* device, vgpu_buffer* buffer);

    VGPU_EXPORT vgpu_texture* vgpu_create_texture(agpu_device* device, const vgpu_texture_desc* desc);
    VGPU_EXPORT void vgpu_destroy_texture(agpu_device* device, vgpu_texture* texture);

    /// Check if the format has a depth component.
    VGPU_EXPORT bool vgpu_is_depth_format(vgpu_pixel_format format);
    /// Check if the format has a stencil component.
    VGPU_EXPORT bool vgpu_is_stencil_format(vgpu_pixel_format format);
    /// Check if the format has depth or stencil components.
    VGPU_EXPORT bool vgpu_is_depth_stencil_format(vgpu_pixel_format format);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // VGPU_H_
