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

#ifndef VGPU_H
#define VGPU_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
#   define _VGPU_EXTERN extern "C"
#else
#   define _VGPU_EXTERN extern
#endif

#if defined(VGPU_SHARED_LIBRARY)
#   if defined(_WIN32)
#       if defined(VGPU_IMPLEMENTATION)
#           define _VGPU_API_DECL __declspec(dllexport)
#       else
#           define _VGPU_API_DECL __declspec(dllimport)
#       endif
#   else
#       define _VGPU_API_DECL __attribute__((visibility("default")))
#   endif
#else
#   define _VGPU_API_DECL
#endif

#define VGPU_API _VGPU_EXTERN _VGPU_API_DECL

/* Constants */
enum {
    VGPU_INVALID_ID = 0,
    VGPU_MAX_COLOR_ATTACHMENTS = 8u,
    VGPU_MAX_VERTEX_BUFFER_BINDINGS = 8u,
    VGPU_MAX_VERTEX_ATTRIBUTES = 16u,
    VGPU_MAX_VERTEX_ATTRIBUTE_OFFSET = 2047u,
    VGPU_MAX_VERTEX_BUFFER_STRIDE = 2048u,
};

/* Handles */
typedef struct vgpu_swapchain_t* vgpu_swapchain;
typedef struct vgpu_texture_t* vgpu_texture;
typedef struct vgpu_buffer_t* vgpu_buffer;
typedef struct vgpu_framebuffer_t* vgpu_framebuffer;

typedef enum vgpu_texture_format {
    VGPU_TEXTURE_FORMAT_UNDEFINED = 0,
    VGPUTextureFormat_R8UNorm,
    VGPUTextureFormat_R8SNorm,
    VGPUTextureFormat_R8UInt,
    VGPUTextureFormat_R8SInt,
    VGPUTextureFormat_R16UInt,
    VGPUTextureFormat_R16SInt,
    VGPUTextureFormat_R16Float,
    VGPUTextureFormat_RG8UNorm,
    VGPUTextureFormat_RG8SNorm,
    VGPUTextureFormat_RG8UInt,
    VGPUTextureFormat_RG8SInt,
    VGPUTextureFormat_R32Float,
    VGPUTextureFormat_R32UInt,
    VGPUTextureFormat_R32SInt,
    VGPUTextureFormat_RG16UInt,
    VGPUTextureFormat_RG16SInt,
    VGPUTextureFormat_RG16Float,
    VGPU_TEXTURE_FORMAT_RGBA8,
    VGPU_TEXTURE_FORMAT_RGBA8_SRGB,
    VGPUTextureFormat_RGBA8SNorm,
    VGPUTextureFormat_RGBA8UInt,
    VGPUTextureFormat_RGBA8SInt,
    VGPU_TEXTURE_FORMAT_BGRA8,
    VGPU_TEXTURE_FORMAT_BGRA8_SRGB,
    VGPU_TEXTURE_FORMAT_RGB10A2,
    VGPU_TEXTURE_FORMAT_RG11B10F,
    VGPUTextureFormat_RG32Float,
    VGPUTextureFormat_RG32UInt,
    VGPUTextureFormat_RG32SInt,
    VGPUTextureFormat_RGBA16UInt,
    VGPUTextureFormat_RGBA16SInt,
    VGPUTextureFormat_RGBA16Float,
    VGPUTextureFormat_RGBA32Float,
    VGPUTextureFormat_RGBA32UInt,
    VGPUTextureFormat_RGBA32SInt,
    VGPU_TEXTURE_FORMAT_D32F,
    VGPU_TEXTURE_FORMAT_D24_PLUS,
    VGPU_TEXTURE_FORMAT_D24S8,
    VGPU_TEXTURE_FORMAT_BC1,
    VGPU_TEXTURE_FORMAT_BC1_SRGB,
    VGPU_TEXTURE_FORMAT_BC2,
    VGPU_TEXTURE_FORMAT_BC2_SRGB,
    VGPU_TEXTURE_FORMAT_BC3,
    VGPU_TEXTURE_FORMAT_BC3_SRGB,
    VGPU_TEXTURE_FORMAT_BC4,
    VGPU_TEXTURE_FORMAT_BC4S,
    VGPU_TEXTURE_FORMAT_BC5,
    VGPU_TEXTURE_FORMAT_BC5S,
    VGPU_TEXTURE_FORMAT_BC6H_UFLOAT,
    VGPU_TEXTURE_FORMAT_BC6H_SFLOAT,
    VGPU_TEXTURE_FORMAT_BC7,
    VGPU_TEXTURE_FORMAT_BC7_SRGB,
    _VGPU_TEXTURE_FORMAT_FORCE_U32 = 0x7FFFFFFF
} vgpu_texture_format;

typedef enum VGPUPixelFormatAspect {
    VGPUPixelFormatAspect_Color,
    VGPUPixelFormatAspect_Depth,
    VGPUPixelFormatAspect_Stencil,
    VGPUPixelFormatAspect_DepthStencil,
} VGPUPixelFormatAspect;

typedef enum VGPUPixelFormatType {
    VGPUPixelFormatType_Unknown = 0,
    VGPUPixelFormatType_Float,
    VGPUPixelFormatType_UNorm,
    VGPUPixelFormatType_UNormSrgb,
    VGPUPixelFormatType_SNorm,
    VGPUPixelFormatType_SInt,
    VGPUPixelFormatType_UInt,
    VGPUPixelFormatType_Force32 = 0x7FFFFFFF
} VGPUPixelFormatType;

/* Structures */
typedef struct vgpu_allocation_callbacks {
    void* user_data;
    void* (*allocate_memory)(void* user_data, size_t size);
    void* (*allocate_cleared_memory)(void* user_data, size_t size);
    void (*free_memory)(void* user_data, void* ptr);
} vgpu_allocation_callbacks;

typedef struct vgpu_color {
    float r;
    float g;
    float b;
    float a;
} vgpu_color;

/* Log */
typedef enum vgpu_log_level {
    VGPU_LOG_LEVEL_ERROR,
    VGPU_LOG_LEVEL_WARN,
    VGPU_LOG_LEVEL_INFO,
    VGPU_LOG_LEVEL_DEBUG,
    _VGPU_LOG_LEVEL_FORCE_U32 = 0x7FFFFFFF
} vgpu_log_level;
typedef void (*vgpu_PFN_log)(void* userData, vgpu_log_level level, const char* message);

VGPU_API void vgpu_log_set_log_callback(vgpu_PFN_log callback, void* user_data);
VGPU_API void vgpu_log(vgpu_log_level level, const char* format, ...);
VGPU_API void vgpu_log_error(const char* format, ...);

/* Allocation functions */
VGPU_API void vgpu_set_allocation_callbacks(const vgpu_allocation_callbacks* callbacks);

/* Texture */
typedef enum vgpu_texture_type {
    VGPU_TEXTURE_TYPE_2D,
    VGPU_TEXTURE_TYPE_3D,
    VGPU_TEXTURE_TYPE_CUBE,
    _VGPU_TEXTURE_TYPE_FORCE_U32 = 0x7FFFFFFF
} vgpu_texture_type;

typedef enum vgpu_texture_usage {
    VGPU_TEXTURE_USAGE_SAMPLED = (1 << 0),
    VGPU_TEXTURE_USAGE_STORAGE = (1 << 1),
    VGPU_TEXTURE_USAGE_RENDER_TARGET = (1 << 2),
    _VGPU_TEXTURE_USAGE_FORCE_U32 = 0x7FFFFFFF
} vgpu_texture_usage;
typedef uint32_t vgpu_texture_usage_flags;

typedef struct {
    uint32_t width;
    uint32_t height;
    union {
        uint32_t depth;
        uint32_t array_layers;
    };
    uint32_t mip_levels;
    vgpu_texture_format format;
    vgpu_texture_type type;
    vgpu_texture_usage_flags usage;
    uint32_t sample_count;
    const void* external_handle;
    const char* label;
} vgpu_texture_info;

VGPU_API vgpu_texture vgpu_texture_create(const vgpu_texture_info* info);
VGPU_API void vgpu_texture_destroy(vgpu_texture texture);
VGPU_API uint32_t vgpu_texture_get_width(vgpu_texture texture, uint32_t mip_level);
VGPU_API uint32_t vgpu_texture_get_height(vgpu_texture texture, uint32_t mip_level);

/* Framebuffer */
typedef enum vgpu_load_op {
    VGPU_LOAD_OP_CLEAR = 0,
    VGPU_LOAD_OP_LOAD = 1,
    VGPU_LOAD_OP_DISCARD = 2,
    _VGPU_LOAD_OP__FORCE_U32 = 0x7FFFFFFF
} vgpu_load_op;

typedef struct vgpu_color_attachment {
    vgpu_texture    texture;
    uint32_t        level;
    uint32_t        slice;
    vgpu_load_op    load_op;
    vgpu_color      clear_color;
} vgpu_color_attachment;

typedef struct gpu_depth_stencil_attachment {
    vgpu_texture texture;
    uint32_t level;
    uint32_t slice;
    vgpu_load_op load;
    float clear;
    struct {
        vgpu_load_op load;
        uint8_t clear;
    } stencil;
} gpu_depth_stencil_attachment;

typedef struct vgpu_framebuffer_info {
    vgpu_color_attachment color_attachments[VGPU_MAX_COLOR_ATTACHMENTS];
    gpu_depth_stencil_attachment depth_stencil_attachment;
    uint32_t width;
    uint32_t height;
    uint32_t layers;
    const char* label;
} vgpu_framebuffer_info;

VGPU_API vgpu_framebuffer vgpu_framebuffer_create(const vgpu_framebuffer_info* info);
VGPU_API void vgpu_framebuffer_destroy(vgpu_framebuffer framebuffer);
VGPU_API vgpu_framebuffer vgpu_framebuffer_get_default(void);

/* Buffer */
typedef enum {
    VGPU_BUFFER_USAGE_NONE = 0,
    VGPU_BUFFER_USAGE_VERTEX = 1 << 0,
    VGPU_BUFFER_USAGE_INDEX = 1 << 1,
    _VGPU_BUFFER_USAGE_FORCE_U32 = 0x7FFFFFFF
} vgpu_buffer_usage;
typedef uint32_t vgpu_buffer_usage_flags;

typedef struct {
    uint64_t size;
    vgpu_buffer_usage_flags usage;
    void* data;
    const char* label;
} vgpu_buffer_info;

VGPU_API vgpu_buffer vgpu_buffer_create(const vgpu_buffer_info* info);
VGPU_API void vgpu_buffer_destroy(vgpu_buffer buffer);

/* Swapchain */
VGPU_API vgpu_swapchain vgpu_swapchain_create(uintptr_t window_handle, vgpu_texture_format color_format, vgpu_texture_format depth_stencil_format);
VGPU_API void vgpu_swapchain_destroy(vgpu_swapchain swapchain);
VGPU_API void vgpu_swapchain_resize(vgpu_swapchain swapchain, uint32_t width, uint32_t height);
VGPU_API void vgpu_swapchain_present(vgpu_swapchain swapchain);

/* Device/Context */
typedef enum vgpu_backend_type {
    VGPU_BACKEND_TYPE_DEFAULT,
    VGPU_BACKEND_TYPE_NULL,
    VGPU_BACKEND_TYPE_D3D11,
    VGPU_BACKEND_TYPE_D3D12,
    VGPU_BACKEND_TYPE_METAL,
    VGPU_BACKEND_TYPE_VULKAN,
    VGPU_BACKEND_TYPE_OPENGL,
    VGPU_BACKEND_TYPE_COUNT,
    _VGPU_BACKEND_TYPE_FORCE_U32 = 0x7FFFFFFF
} vgpu_backend_type;

typedef enum vgpu_device_preference {
    VGPU_DEVICE_PREFERENCE_HIGH_PERFORMANCE = 0,
    VGPU_DEVICE_PREFERENCE_LOW_POWER = 1,
    VGPU_DEVICE_PREFERENCE_DONT_CARE = 2,
    _VGPU_DEVICE_PREFERENCE_FORCE_U32 = 0x7FFFFFFF
} vgpu_device_preference;

typedef enum vgpu_present_mode {
    VGPU_PRESENT_MODE_FIFO,
    VGPU_PRESENT_MODE_IMMEDIATE,
    VGPU_PRESENT_MODE_MAILBOX,
    _VGPU_PRESENT_MODE_FORCE_U32 = 0x7FFFFFFF
} vgpu_present_mode;

typedef struct vgpu_config {
    vgpu_backend_type backend_type;
    bool debug;
    bool profile;
    vgpu_device_preference device_preference;
    vgpu_present_mode present_mode;
    uintptr_t display_handle;   /**< Display, wl_display. */
    uintptr_t window_handle;    /**< HWND, ANativeWindow, NSWindow, etc. */
    vgpu_texture_format color_format;
    vgpu_texture_format depth_stencil_format;
    void* (*getProcAddress)(const char* funcName);
} vgpu_config;

typedef struct vgpu_features {
    bool independent_blend;
    bool compute_shader;
    bool geometry_shader;
    bool tessellation_shader;
    bool multi_viewport;
    bool index_type_uint32;
    bool multi_draw_indirect;
    bool fill_mode_non_solid;
    bool sampler_anisotropy;
    bool texture_compression_ETC2;
    bool texture_compression_ASTC_LDR;
    bool texture_compression_BC;
    bool texture_cube_array;
    bool raytracing;
} vgpu_features;

typedef struct vgpu_limits {
    uint32_t        max_vertex_attributes;
    uint32_t        max_vertex_bindings;
    uint32_t        max_vertex_attribute_offset;
    uint32_t        max_vertex_binding_stride;
    uint32_t        max_texture_size_2d;
    uint32_t        max_texture_size_3d;
    uint32_t        max_texture_size_cube;
    uint32_t        max_texture_array_layers;
    uint32_t        max_color_attachments;
    uint32_t        max_uniform_buffer_size;
    uint64_t        min_uniform_buffer_offset_alignment;
    uint32_t        max_storage_buffer_size;
    uint64_t        min_storage_buffer_offset_alignment;
    float           max_sampler_anisotropy;
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
} vgpu_limits;

typedef struct vgpu_caps {
    vgpu_backend_type backend_type;
    vgpu_features features;
    vgpu_limits limits;
} vgpu_caps;

VGPU_API bool vgpu_init(const vgpu_config* config);
VGPU_API void vgpu_shutdown(void);
VGPU_API void vgpu_get_caps(vgpu_caps* caps);
VGPU_API bool vgpu_frame_begin(void);
VGPU_API void vgpu_frame_finish(void);

VGPU_API void vgpuInsertDebugMarker(const char* name);
VGPU_API void vgpuPushDebugGroup(const char* name);
VGPU_API void vgpuPopDebugGroup(void);
VGPU_API void vgpu_render_begin(vgpu_framebuffer framebuffer);
VGPU_API void vgpu_render_finish(void);

/* Helper methods */
/// Check if the format is color format.
VGPU_API bool vgpu_is_color_format(vgpu_texture_format format);
/// Check if the format has a depth component.
VGPU_API bool vgpu_is_depth_format(vgpu_texture_format format);
/// Check if the format has a stencil component.
VGPU_API bool vgpu_is_stencil_format(vgpu_texture_format format);
/// Check if the format has depth or stencil components.
VGPU_API bool vgpu_is_depth_stencil_format(vgpu_texture_format format);
/// Check if the format is a compressed format.
VGPU_API bool vgpu_is_compressed_format(vgpu_texture_format format);

#endif /* VGPU_H */
