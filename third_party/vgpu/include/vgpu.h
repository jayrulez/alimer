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

#ifndef __VGPU_H__
#define __VGPU_H__

// On Windows, use the stdcall convention, pn other platforms, use the default calling convention
#if defined(_WIN32)
#   define VGPU_API_CALL __stdcall
#else
#   define VGPU_API_CALL
#endif

#if defined(VGPU_SHARED_LIBRARY)
#   if defined(_WIN32)
#       if defined(VGPU_IMPLEMENTATION)
#           define VGPU_API __declspec(dllexport)
#       else
#           define VGPU_API __declspec(dllimport)
#       endif
#   else  // defined(_WIN32)
#       define VGPU_API __attribute__((visibility("default")))
#   endif  // defined(_WIN32)
#else       // defined(VGPU_SHARED_LIBRARY)
#   define VGPU_API
#endif  // defined(VGPU_SHARED_LIBRARY)

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
    typedef struct vgpu_buffer_t* vgpu_buffer;
    typedef struct vgpu_shader_t* vgpu_shader;
    typedef struct vgpu_texture_t* vgpu_texture;
    typedef struct vgpu_pipeline_t* vgpu_pipeline;

    /* Constants */
    enum {
        VGPU_NUM_INFLIGHT_FRAMES = 2u,
        VGPU_MAX_INFLIGHT_FRAMES = 3u,
        VGPU_MAX_LOG_MESSAGE_LENGTH = 4096u,
        VGPU_MAX_COLOR_ATTACHMENTS = 8u,
        VGPU_MAX_VERTEX_BUFFER_BINDINGS = 8u,
        VGPU_MAX_VERTEX_ATTRIBUTES = 16u,
        VGPU_MAX_VERTEX_ATTRIBUTE_OFFSET = 2047u,
        VGPU_MAX_VERTEX_BUFFER_STRIDE = 2048u
    };

    /* Enums */
    typedef enum vgpu_log_level {
        VGPU_LOG_LEVEL_INFO,
        VGPU_LOG_LEVEL_WARN,
        VGPU_LOG_LEVEL_ERROR,
        _VGPU_LOG_LEVEL_FORCE_U32 = 0x7FFFFFFF
    } vgpu_log_level;

    typedef enum vgpu_backend_type {
        /// Null renderer.
        VGPU_BACKEND_TYPE_NULL,
        /// Direct3D 11 backend.
        VGPU_BACKEND_TYPE_D3D11,
        /// Direct3D 12 backend.
        VGPU_BACKEND_TYPE_D3D12,
        /// Metal backend.
        VGPU_BACKEND_TYPE_METAL,
        /// Vulkan backend.
        VGPU_BACKEND_TYPE_VULKAN,
        /// OpenGL 3.3+ or GLES 3.0+ backend.
        VGPU_BACKEND_TYPE_OPENGL,
        /// Default best platform supported backend.
        VGPU_BACKEND_TYPE_COUNT,
        _VGPU_BACKEND_TYPE_FORCE_U32 = 0x7FFFFFFF
    } vgpu_backend_type;

    /// Defines pixel format.
    typedef enum agpu_texture_format {
        AGPU_TEXTURE_FORMAT_INVALID = 0,
        // 8-bit pixel formats
        R8Unorm,
        R8Snorm,
        R8Uint,
        R8Sint,
        // 16-bit pixel formats
        R16Unorm,
        R16Snorm,
        R16Uint,
        R16Sint,
        R16Float,
        RG8Unorm,
        RG8Snorm,
        RG8Uint,
        RG8Sint,
        // 32-bit pixel formats
        R32Uint,
        R32Sint,
        R32Float,
        RG16Unorm,
        RG16Snorm,
        RG16Uint,
        RG16Sint,
        RG16Float,
        AGPU_TEXTURE_FORMAT_RGBA8_UNORM,
        AGPU_TEXTURE_FORMAT_RGBA8_UNORM_SRGB,
        AGPU_TEXTURE_FORMAT_RGBA8_SNORM,
        AGPU_TEXTURE_FORMAT_RGBA8_UINT,
        AGPU_TEXTURE_FORMAT_RGBA8_SINT,
        AGPU_TEXTURE_FORMAT_BGRA8_UNORM,
        AGPU_TEXTURE_FORMAT_BGRA8_UNORM_SRGB,
        // Packed 32-Bit Pixel formats
        AGPU_TEXTURE_FORMAT_RGB10A2_UNORM,
        RG11B10Float,
        RGB9E5Float,
        // 64-Bit Pixel Formats
        RG32Uint,
        RG32Sint,
        RG32Float,
        AGPU_TEXTURE_FORMAT_RGBA16_UNORM,
        RGBA16Snorm,
        RGBA16Uint,
        RGBA16Sint,
        RGBA16Float,
        // 128-Bit Pixel Formats
        RGBA32Uint,
        RGBA32Sint,
        AGPU_TEXTURE_FORMAT_RGBA32_FLOAT,
        // Depth-stencil formats
        Stencil8,
        Depth16Unorm,
        Depth24Plus,
        Depth24PlusStencil8,
        Depth32Float,
        // Compressed BC formats
        BC1RGBAUnorm,
        BC1RGBAUnormSrgb,
        BC2RGBAUnorm,
        BC2RGBAUnormSrgb,
        BC3RGBAUnorm,
        BC3RGBAUnormSrgb,
        BC4RUnorm,
        BC4RSnorm,
        BC5RGUnorm,
        BC5RGSnorm,
        BC6HRGBUfloat,
        BC6HRGBFloat,
        BC7RGBAUnorm,
        BC7RGBAUnormSrgb,
        _AGPU_TEXTURE_FORMAT_COUNT,
        _AGPU_TEXTURE_FORMAT_FORCE_U32 = 0x7FFFFFFF
    } agpu_texture_format;

    typedef enum agpu_texture_type {
        AGPU_TEXTURE_TYPE_2D,
        AGPU_TEXTURE_TYPE_3D,
        AGPU_TEXTURE_TYPE_CUBE,
        AGPU_TEXTURE_TYPE_ARRAY,
        _AGPU_TEXTURE_TYPE_FORCE_U32 = 0x7FFFFFFF
    } agpu_texture_type;

    typedef enum agpu_texture_usage {
        AGPU_TEXTURE_USAGE_NONE = 0,
        AGPU_TEXTURE_USAGE_SAMPLED = (1 << 0),
        AGPU_TEXTURE_USAGE_STORAGE = (1 << 1),
        AGPU_TEXTURE_USAGE_RENDER_TARGET = (1 << 2),
        _AGPU_TEXTURE_USAGE_FORCE_U32 = 0x7FFFFFFF
    } agpu_texture_usage;

    typedef enum agpu_load_op {
        AGPU_LOAD_OP_CLEAR = 0,
        AGPU_LOAD_OP_LOAD = 1,
        _AGPU_LOAD_OP_FORCE_U32 = 0x7FFFFFFF
    } agpu_load_op;

    /* Structs */
    typedef struct vgpu_color {
        float r;
        float g;
        float b;
        float a;
    } vgpu_color;

    typedef struct vgpu_texture_info {
        agpu_texture_type type;
        agpu_texture_usage usage;
        agpu_texture_format format;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t mipmaps;
        const void* external_handle;
        const char* label;
    } vgpu_texture_info;

    typedef struct vgpu_shader_source {
        const void* code;
        size_t size;
        const char* entry;
    } vgpu_shader_source;

    typedef struct vgpu_shader_info {
        vgpu_shader_source vertex;
        vgpu_shader_source fragment;
        vgpu_shader_source compute;
        const char* label;
    } vgpu_shader_info;

    typedef struct vgpu_pipeline_info {
        vgpu_shader shader;
        const char* label;
    } vgpu_pipeline_info;

    typedef struct vgpu_color_attachment {
        vgpu_texture texture;
        uint32_t level;
        uint32_t slice;
        agpu_load_op load_op;
        vgpu_color clear_color;
    } vgpu_color_attachment;

    typedef struct agpu_depth_stencil_attachment {
        vgpu_texture texture;
        uint32_t mip_level;
        uint32_t level;
        uint32_t slice;
        agpu_load_op depth_load_op;
        agpu_load_op stencil_load_op;
        float clear_depth;
        uint8_t clear_stencil;
    } agpu_depth_stencil_attachment;

    typedef struct agpu_render_pass_info {
        uint32_t num_color_attachments;
        vgpu_color_attachment color_attachments[VGPU_MAX_COLOR_ATTACHMENTS];
        agpu_depth_stencil_attachment depth_stencil;
    } agpu_render_pass_info;

    typedef struct vgpu_features {
        bool independentBlend;
        bool computeShader;
        bool indexUInt32;
        bool fillModeNonSolid;
        bool samplerAnisotropy;
        bool textureCompressionETC2;
        bool textureCompressionASTC_LDR;
        bool textureCompressionBC;
        bool textureCubeArray;
        bool raytracing;
    } vgpu_features;

    typedef struct vgpu_limits {
        uint32_t        maxVertexAttributes;
        uint32_t        maxVertexBindings;
        uint32_t        maxVertexAttributeOffset;
        uint32_t        maxVertexBindingStride;
        uint32_t        maxTextureDimension2D;
        uint32_t        maxTextureDimension3D;
        uint32_t        maxTextureDimensionCube;
        uint32_t        maxTextureArrayLayers;
        uint32_t        maxColorAttachments;
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
    } vgpu_limits;

    typedef struct agpu_caps {
        vgpu_backend_type backend;
        uint32_t vendorID;
        uint32_t deviceID;
        vgpu_features features;
        vgpu_limits limits;
    } agpu_caps;

    typedef struct vgpu_swapchain_info {
        void* window_handle;
        uint32_t width;
        uint32_t height;
        agpu_texture_format color_format;
        agpu_texture_format depth_stencil_format;
        bool vsync;
        bool is_fullscreen;
        uint32_t sample_count;
    } vgpu_swapchain_info;

    typedef struct agpu_config {
        bool debug;
        vgpu_swapchain_info swapchain_info;
    } agpu_config;

    /* Log functions */
    typedef void(VGPU_API_CALL* vgpu_log_callback)(void* userData, vgpu_log_level level, const char* message);
    VGPU_API void vgpu_set_log_callback(vgpu_log_callback callback, void* user_data);
    VGPU_API void vgpu_log(vgpu_log_level level, const char* format, ...);

    /* Frame logic */
    VGPU_API bool agpu_set_preferred_backend(vgpu_backend_type backend);
    VGPU_API bool vgpu_init(const char* app_name, const agpu_config* config);
    VGPU_API void vgpu_shutdown(void);
    VGPU_API void vgpu_query_caps(agpu_caps* caps);
    VGPU_API bool vgpu_begin_frame(void);
    VGPU_API void vgpu_end_frame(void);

    /* Buffer */
    typedef enum vgpu_buffer_type {
        VGPU_BUFFER_TYPE_VERTEX,
        VGPU_BUFFER_TYPE_INDEX,
        VGPU_BUFFER_TYPE_UNIFORM,
        _VGPU_BUFFER_TYPE_FORCE_U32 = 0x7FFFFFFF
    } vgpu_buffer_type;

    typedef enum agpu_buffer_usage {
        AGPU_BUFFER_USAGE_IMMUTABLE,
        AGPU_BUFFER_USAGE_DYNAMIC,
        AGPU_BUFFER_USAGE_STREAM,
        _AGPU_BUFFER_USAGE_FORCE_U32 = 0x7FFFFFFF
    } agpu_buffer_usage;

    typedef struct agpu_buffer_info {
        uint64_t size;
        vgpu_buffer_type type;
        agpu_buffer_usage usage;
        const void* data;
        const char* label;
    } agpu_buffer_info;

    VGPU_API vgpu_buffer vgpu_create_buffer(const agpu_buffer_info* info);
    VGPU_API void vgpu_destroy_buffer(vgpu_buffer buffer);

    /* Shader */
    VGPU_API vgpu_shader vgpu_create_shader(const vgpu_shader_info* info);
    VGPU_API void vgpu_destroy_shader(vgpu_shader shader);

    /* Texture */
    VGPU_API vgpu_texture vgpu_create_texture(const vgpu_texture_info* info);
    VGPU_API void vgpu_destroy_texture(vgpu_texture handle);
    VGPU_API uint64_t vgpu_texture_get_native_handle(vgpu_texture handle);

    /* Pipeline */
    VGPU_API vgpu_pipeline vgpu_create_pipeline(const vgpu_pipeline_info* info);
    VGPU_API void vgpu_destroy_pipeline(vgpu_pipeline pipeline);

    /* Commands */
    VGPU_API void vgpu_push_debug_group(const char* name);
    VGPU_API void vgpu_pop_debug_group(void);
    VGPU_API void vgpu_insert_debug_marker(const char* name);
    VGPU_API void vgpu_begin_render_pass(const agpu_render_pass_info* info);
    VGPU_API void vgpu_end_render_pass(void);
    VGPU_API void vgpu_bind_pipeline(vgpu_pipeline pipeline);
    VGPU_API void vgpu_draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex);

    /* Utility methods */
    VGPU_API uint32_t vgpu_calculate_mip_levels(uint32_t width, uint32_t height, uint32_t depth);

#ifdef __cplusplus
}
#endif

#endif /* __VGPU_H__ */

