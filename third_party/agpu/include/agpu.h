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

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
    typedef struct agpu_buffer { uint32_t id; } agpu_buffer;
    typedef struct agpu_texture { uint32_t id; } agpu_texture;
    typedef struct agpu_swapchain { uint32_t id; } agpu_swapchain;

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
        AGPU_LOG_LEVEL_ERROR = 0,
        AGPU_LOG_LEVEL_WARN = 1,
        AGPU_LOG_LEVEL_INFO = 2,
        _AGPU_LOG_LEVEL_COUNT,
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

    typedef enum agpu_init_flags
    {
        AGPU_INIT_FLAGS_NONE = 0,
        AGPU_INIT_FLAGS_DEBUG = (1 << 0),
        AGPU_INIT_FLAGS_LOW_POWER_GPU = (1 << 1),
        AGPU_INIT_FLAGS_GPU_VALIDATION = (1 << 2),
        AGPU_INIT_FLAGS_RENDERDOC = (1 << 3),
        _AGPU_INIT_FLAGS_FORCE_U32 = 0x7FFFFFFF
    } agpu_init_flags;

    typedef enum agpu_buffer_usage {
        AGPU_BUFFER_USAGE_NONE = 0,
        AGPU_BUFFER_USAGE_UNIFORM = (1 << 0),
        AGPU_BUFFER_USAGE_STORAGE = (1 << 1),
        AGPU_BUFFER_USAGE_INDEX = (1 << 2),
        AGPU_BUFFER_USAGE_VERTEX = (1 << 3),
        AGPU_BUFFER_USAGE_INDIRECT = (1 << 4),
        _AGPU_BUFFER_USAGE_FORCE_U32 = 0x7FFFFFFF
    } agpu_buffer_usage;

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

    enum class LoadAction : uint32_t
    {
        Discard,
        Load,
        Clear
    };

    /* Structs */
    typedef struct agpu_color {
        float r;
        float g;
        float b;
        float a;
    } agpu_color;

    typedef struct agpu_buffer_info {
        uint64_t size;
        agpu_buffer_usage usage;
        void* data;
        const char* label;
    } agpu_buffer_info;

    typedef struct agpu_texture_info {
        agpu_texture_type type;
        agpu_texture_format format;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t mipmaps;
        const void* external_handle;
        const char* label;
    } agpu_texture_info;

    typedef struct agpu_swapchain_info {
        uint32_t width;
        uint32_t height;
        agpu_texture_format color_format;
        void* window_handle;
        const char* label;
    } agpu_swapchain_info;

    struct RenderPassColorAttachment
    {
        agpu_texture texture;
        uint32_t mipLevel = 0;
        union {
            uint32_t face = 0;
            uint32_t layer;
            uint32_t slice;
        };
        LoadAction loadAction;
        agpu_color clear_color;
    };

    struct RenderPassDepthStencilAttachment
    {
        agpu_texture texture;
        uint32_t mipLevel = 0;
        union {
            //TextureCubemapFace face = TextureCubemapFace::PositiveX;
            uint32_t face = 0;
            uint32_t layer;
            uint32_t slice;
        };
        LoadAction depthLoadAction = LoadAction::Clear;
        LoadAction stencilLoadOp = LoadAction::Discard;
        float clearDepth = 1.0f;
        uint8_t clearStencil = 0;
    };

    struct RenderPassDescription
    {
        uint32_t                                colorAttachmentsCount;
        const RenderPassColorAttachment* colorAttachments;
        const RenderPassDepthStencilAttachment* depthStencilAttachment = nullptr;
    };

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

    /* Log functions */
    typedef void(AGPU_API_CALL* logCallback)(void* userData, agpu_log_level level, const char* message);
    AGPU_API void agpu_set_log_callback(logCallback callback, void* user_data);
    AGPU_API void agpu_log(agpu_log_level level, const char* format, ...);

    /* Frame logic */
    AGPU_API bool agpu_set_preferred_backend(agpu_backend_type backend);
    AGPU_API bool agpu_init(agpu_init_flags flags, const agpu_swapchain_info* swapchain_info);
    AGPU_API void agpu_shutdown(void);
    AGPU_API void aqpu_query_caps(agpu_caps* caps);
    AGPU_API bool agpu_frame_begin(void);
    AGPU_API void agpu_frame_end(void);

    /* Resource creation methods */
    AGPU_API agpu_swapchain agpu_create_swapchain(const agpu_swapchain_info* info);
    AGPU_API void agpu_destroy_swapchain(agpu_swapchain swapchain);

    AGPU_API agpu_buffer agpu_create_buffer(const agpu_buffer_info* info);
    AGPU_API void agpu_destroy_buffer(agpu_buffer buffer);

    AGPU_API agpu_texture agpu_create_texture(const agpu_texture_info* info);
    AGPU_API void agpu_destroy_texture(agpu_texture handle);

    /* Commands */
    AGPU_API void agpu_push_debug_group(const char* name);
    AGPU_API void agpu_pop_debug_group(void);
    AGPU_API void agpu_insert_debug_marker(const char* name);
    AGPU_API void agpu_begin_render_pass(const RenderPassDescription* renderPass);
    AGPU_API void agpu_end_render_pass(void);

    /* Utility methods */
    AGPU_API uint32_t agpu_calculate_mip_levels(uint32_t width, uint32_t height, uint32_t depth = 1u);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
inline constexpr agpu_init_flags operator | (agpu_init_flags a, agpu_init_flags b) { return agpu_init_flags(uint32_t(a) | uint32_t(b)); }
inline constexpr agpu_init_flags& operator |= (agpu_init_flags& a, agpu_init_flags b) { return a = a | b; }
#endif
