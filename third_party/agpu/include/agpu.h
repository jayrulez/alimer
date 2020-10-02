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

#pragma once

// On Windows, use the stdcall convention, pn other platforms, use the default calling convention
#if defined(_WIN32)
#   define AGPU_API_CALL __stdcall
#else
#   define AGPU_API_CALL
#endif

#if defined(AGPU_SHARED_LIBRARY)
#   if defined(_WIN32)
#       if defined(AGPU_IMPLEMENTATION)
#           define AGPU_API __declspec(dllexport)
#       else
#           define AGPU_API __declspec(dllimport)
#       endif
#   else  // defined(_WIN32)
#       define AGPU_API __attribute__((visibility("default")))
#   endif  // defined(_WIN32)
#else       // defined(AGPU_SHARED_LIBRARY)
#   define AGPU_API
#endif  // defined(AGPU_SHARED_LIBRARY)

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
    typedef struct agpu_buffer_t* agpu_buffer;
    typedef struct agpu_shader_t* agpu_shader;
    typedef struct agpu_texture_t* agpu_texture;
    typedef struct agpu_pipeline_t* agpu_pipeline;
    
    /* Constants */
    enum {
        AGPU_INVALID_ID = 0,
        AGPU_NUM_INFLIGHT_FRAMES = 2u,
        AGPU_MAX_INFLIGHT_FRAMES = 3u,
        AGPU_MAX_LOG_MESSAGE_LENGTH = 4096u,
        AGPU_MAX_COLOR_ATTACHMENTS = 8u,
        AGPU_MAX_VERTEX_BUFFER_BINDINGS = 8u,
        AGPU_MAX_VERTEX_ATTRIBUTES = 16u,
        AGPU_MAX_VERTEX_ATTRIBUTE_OFFSET = 2047u,
        AGPU_MAX_VERTEX_BUFFER_STRIDE = 2048u
    };

    /* Enums */
    typedef enum agpu_log_level {
        AGPU_LOG_LEVEL_INFO,
        AGPU_LOG_LEVEL_WARN,
        AGPU_LOG_LEVEL_ERROR,
        _AGPU_LOG_LEVEL_FORCE_U32 = 0x7FFFFFFF
    } agpu_log_level;

    typedef enum agpu_backend_type {
        /// Null renderer.
        AGPU_BACKEND_TYPE_NULL,
        /// Direct3D 11 backend.
        AGPU_BACKEND_TYPE_D3D11,
        /// Direct3D 12 backend.
        AGPU_BACKEND_TYPE_D3D12,
        /// Metal backend.
        AGPU_BACKEND_TYPE_METAL,
        /// Vulkan backend.
        AGPU_BACKEND_TYPE_VULKAN,
        /// OpenGL 3.3+ or GLES 3.0+ backend.
        AGPU_BACKEND_TYPE_OPENGL,
        /// Default best platform supported backend.
        AGPU_BACKEND_TYPE_COUNT,
        _AGPU_BACKEND_TYPE_FORCE_U32 = 0x7FFFFFFF
    } agpu_backend_type;

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
    typedef struct agpu_color {
        float r;
        float g;
        float b;
        float a;
    } agpu_color;

    typedef struct agpu_texture_info {
        agpu_texture_type type;
        agpu_texture_usage usage;
        agpu_texture_format format;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t mipmaps;
        const void* external_handle;
        const char* label;
    } agpu_texture_info;

    typedef struct agpu_color_attachment
    {
        agpu_texture texture;
        uint32_t mip_level;
        union {
            uint32_t face;
            uint32_t layer;
            uint32_t slice;
        };
        agpu_load_op load_op;
        agpu_color clear_color;
    } agpu_color_attachment;

    typedef struct agpu_depth_stencil_attachment
    {
        agpu_texture texture;
        uint32_t mip_level;
        union {
            uint32_t face;
            uint32_t layer;
            uint32_t slice;
        };

        agpu_load_op depth_load_op;
        agpu_load_op stencil_load_op;
        float clear_depth;
        uint8_t clear_stencil;
    } agpu_depth_stencil_attachment;

    typedef struct agpu_render_pass_info
    {
        uint32_t num_color_attachments;
        agpu_color_attachment color_attachments[AGPU_MAX_COLOR_ATTACHMENTS];
        agpu_depth_stencil_attachment depth_stencil;
    } agpu_render_pass_info;

    typedef struct agpu_features {
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
    } agpu_features;

    typedef struct agpu_limits {
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
    } agpu_limits;

    typedef struct agpu_caps {
        agpu_backend_type backend;
        uint32_t vendorID;
        uint32_t deviceID;
        agpu_features features;
        agpu_limits limits;
    } agpu_caps;

    typedef struct agpu_swapchain_info {
        void* window_handle;
        uint32_t width;
        uint32_t height;
        agpu_texture_format color_format;
        agpu_texture_format depth_stencil_format;
        bool vsync;
        bool is_fullscreen;
        uint32_t sample_count;
    } agpu_swapchain_info;

    typedef struct agpu_config {
        bool debug;
        agpu_swapchain_info swapchain_info;
    } agpu_config;

    /* Log functions */
    typedef void(AGPU_API_CALL* agpu_log_callback)(void* userData, agpu_log_level level, const char* message);
    AGPU_API void agpu_set_log_callback(agpu_log_callback callback, void* user_data);
    AGPU_API void agpu_log(agpu_log_level level, const char* format, ...);

    /* Frame logic */
    AGPU_API bool agpu_set_preferred_backend(agpu_backend_type backend);
    AGPU_API bool agpu_init(const char* app_name, const agpu_config* config);
    AGPU_API void agpu_shutdown(void);
    AGPU_API void agpuQueryCaps(agpu_caps* caps);
    AGPU_API bool agpu_begin_frame(void);
    AGPU_API void agpu_end_frame(void);

    /* Buffer */
    typedef enum agpu_buffer_type {
        AGPU_BUFFER_TYPE_VERTEX,
        AGPU_BUFFER_TYPE_INDEX,
        AGPU_BUFFER_TYPE_UNIFORM,
        _AGPU_BUFFER_TYPE_FORCE_U32 = 0x7FFFFFFF
    } agpu_buffer_type;

    typedef enum agpu_buffer_usage {
        AGPU_BUFFER_USAGE_IMMUTABLE,
        AGPU_BUFFER_USAGE_DYNAMIC,
        AGPU_BUFFER_USAGE_STREAM,
        _AGPU_BUFFER_USAGE_FORCE_U32 = 0x7FFFFFFF
    } agpu_buffer_usage;

    typedef struct agpu_buffer_info {
        uint64_t size;
        agpu_buffer_type type;
        agpu_buffer_usage usage;
        const void* data;
        const char* label;
    } agpu_buffer_info;

    AGPU_API agpu_buffer agpu_create_buffer(const agpu_buffer_info* info);
    AGPU_API void agpu_destroy_buffer(agpu_buffer buffer);

    /* Shader */
    typedef struct {
        const void* code;
        size_t size;
        const char* entry;
    } agpu_shader_source;

    typedef struct {
        agpu_shader_source vertex;
        agpu_shader_source fragment;
        agpu_shader_source compute;
        const char* label;
    } agpu_shader_info;

    AGPU_API agpu_shader agpu_create_shader(const agpu_shader_info* info);
    AGPU_API void agpu_destroy_shader(agpu_shader shader);

    /* Texture */
    AGPU_API agpu_texture agpu_create_texture(const agpu_texture_info* info);
    AGPU_API void agpu_destroy_texture(agpu_texture handle);

    /* Pipeline */
    typedef struct {
        agpu_shader shader;
        const char* label;
    } agpu_pipeline_info;

    AGPU_API agpu_pipeline agpu_create_pipeline(const agpu_pipeline_info* info);
    AGPU_API void agpu_destroy_pipeline(agpu_pipeline pipeline);

    /* Commands */
    AGPU_API void agpu_push_debug_group(const char* name);
    AGPU_API void agpu_pop_debug_group(void);
    AGPU_API void agpu_insert_debug_marker(const char* name);
    AGPU_API void agpu_begin_render_pass(const agpu_render_pass_info* info);
    AGPU_API void agpu_end_render_pass(void);
    AGPU_API void agpu_bind_pipeline(agpu_pipeline pipeline);
    AGPU_API void agpu_draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex);

    /* Utility methods */
    AGPU_API uint32_t agpuCalculateMipLevels(uint32_t width, uint32_t height, uint32_t depth);

#ifdef __cplusplus
}
#endif
