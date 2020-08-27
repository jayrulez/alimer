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

#ifndef AGPU_H
#define AGPU_H

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
#include <stdbool.h>

#if !defined(GPU_DEFINE_ENUM_FLAG_OPERATORS)
#define GPU_DEFINE_ENUM_FLAG_OPERATORS(EnumType, UnderlyingEnumType) \
inline constexpr EnumType operator | (EnumType a, EnumType b) { return (EnumType)((UnderlyingEnumType)(a) | (UnderlyingEnumType)(b)); } \
inline constexpr EnumType& operator |= (EnumType &a, EnumType b) { return a = a | b; } \
inline constexpr EnumType operator & (EnumType a, EnumType b) { return EnumType(((UnderlyingEnumType)a) & ((UnderlyingEnumType)b)); } \
inline constexpr EnumType& operator &= (EnumType &a, EnumType b) { return a = a & b; } \
inline constexpr EnumType operator ~ (EnumType a) { return EnumType(~((UnderlyingEnumType)a)); } \
inline constexpr EnumType operator ^ (EnumType a, EnumType b) { return EnumType(((UnderlyingEnumType)a) ^ ((UnderlyingEnumType)b)); } \
inline constexpr EnumType& operator ^= (EnumType &a, EnumType b) { return a = a ^ b; } \
inline constexpr bool any(EnumType a) { return ((UnderlyingEnumType)a) != 0; }
#endif /* !defined(GPU_DEFINE_ENUM_FLAG_OPERATORS) */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


    /* Handles */
    typedef struct agpu_context_t* agpu_context;
    typedef struct agpu_texture_t* agpu_texture;

    /* Enums */
    typedef enum agpu_log_level {
        AGPU_LOG_LEVEL_ERROR = 0,
        AGPU_LOG_LEVEL_WARN = 1,
        AGPU_LOG_LEVEL_INFO = 2,
        AGPU_LOG_LEVEL_DEBUG = 3,
        _AGPU_LOG_LEVEL_COUNT,
        _AGPU_LOG_LEVEL_FORCE_U32 = 0x7FFFFFFF
    } agpu_log_level;

    typedef enum agpu_backend_type {
        AGPU_BACKEND_TYPE_NULL,
        AGPU_BACKEND_TYPE_D3D11,
        AGPU_BACKEND_TYPE_D3D12,
        AGPU_BACKEND_TYPE_VULKAN,
        AGPU_BACKEND_TYPE_COUNT,
        _AGPU_BACKEND_TYPE_FORCE_U32 = 0x7FFFFFFF
    } agpu_backend_type;

    typedef enum agpu_device_preference {
        AGPU_DEVICE_PREFERENCE_DEFAULT = 0,
        AGPU_DEVICE_PREFERENCE_LOW_POWER = 1,
        AGPU_DEVICE_PREFERENCE_HIGH_PERFORMANCE = 2,
        _AGPU_DEVICE_PREFERENCE_FORCE_U32 = 0x7FFFFFFF
    } agpu_device_preference;

    /// Defines pixel format.
    typedef enum agpu_texture_format {
        AGPU_TEXTURE_FORMAT_UNDEFINED = 0,
        // 8-bit pixel formats
        AGPU_TEXTURE_FORMAT_R8_UNORM,
        AGPU_TEXTURE_FORMAT_R8_SNORM,
        AGPU_TEXTURE_FORMAT_R8_UINT,
        AGPU_TEXTURE_FORMAT_R8_SINT,
        // 16-bit pixel formats
        AGPU_TEXTURE_FORMAT_R16_UNORM,
        AGPU_TEXTURE_FORMAT_R16_SNORM,
        AGPU_TEXTURE_FORMAT_R16_UINT,
        AGPU_TEXTURE_FORMAT_R16_SINT,
        AGPU_TEXTURE_FORMAT_R16_FLOAT,
        AGPU_TEXTURE_FORMAT_RG8_UNORM,
        AGPU_TEXTURE_FORMAT_RG8_SNORM,
        AGPU_TEXTURE_FORMAT_RG8_UINT,
        AGPU_TEXTURE_FORMAT_RG8_SINT,
        // 32-bit pixel formats
        AGPU_TEXTURE_FORMAT_R32_UINT,
        AGPU_TEXTURE_FORMAT_R32_SINT,
        AGPU_TEXTURE_FORMAT_R32_FLOAT,
        AGPU_TEXTURE_FORMAT_RG16_UINT,
        AGPU_TEXTURE_FORMAT_RG16_SINT,
        AGPU_TEXTURE_FORMAT_RG16_FLOAT,
        AGPU_TEXTURE_FORMAT_RGBA8_UNORM,
        AGPU_TEXTURE_FORMAT_RGBA8_SRGB,
        AGPU_TEXTURE_FORMAT_RGBA8_SNORM,
        AGPU_TEXTURE_FORMAT_RGBA8_UINT,
        AGPU_TEXTURE_FORMAT_RGBA8_SINT,
        AGPU_TEXTURE_FORMAT_BGRA8_UNORM,
        AGPU_TEXTURE_FORMAT_BGRA8_SRGB,
        // Packed 32-Bit Pixel formats
        AGPU_TEXTURE_FORMAT_RGB10A2_UNORM,
        AGPU_TEXTURE_FORMAT_RG11B10_FLOAT,
        // 64-Bit Pixel Formats
        AGPU_TEXTURE_FORMAT_RG32_UINT,
        AGPU_TEXTURE_FORMAT_RG32_SINT,
        AGPU_TEXTURE_FORMAT_RG32_FLOAT,
        AGPU_TEXTURE_FORMAT_RGBA16_UINT,
        AGPU_TEXTURE_FORMAT_RGBA16_SINT,
        AGPU_TEXTURE_FORMAT_RGBA16_FLOAT,
        // 128-Bit Pixel Formats
        AGPU_TEXTURE_FORMAT_RGBA32_UINT,
        AGPU_TEXTURE_FORMAT_RGBA32_SINT,
        AGPU_TEXTURE_FORMAT_RGBA32_FLOAT,

        // Depth-stencil
        AGPU_TEXTURE_FORMAT_D16_UNORM,
        AGPU_TEXTURE_FORMAT_D32_FLOAT,
        AGPU_TEXTURE_FORMAT_D24_UNORM_S8_UINT,
        AGPU_TEXTURE_FORMAT_D32_FLOAT_S8_UINT,

        // Compressed BC formats
        AGPU_TEXTURE_FORMAT_BC1_UNORM,
        AGPU_TEXTURE_FORMAT_BC1_SRGB,
        AGPU_TEXTURE_FORMAT_BC2_UNORM,
        AGPU_TEXTURE_FORMAT_BC2_SRGB,
        AGPU_TEXTURE_FORMAT_BC3_UNORM,
        AGPU_TEXTURE_FORMAT_BC3_SRGB,
        AGPU_TEXTURE_FORMAT_BC4_UNORM,
        AGPU_TEXTURE_FORMAT_BC4_SNORM,
        AGPU_TEXTURE_FORMAT_BC5_UNORM,
        AGPU_TEXTURE_FORMAT_BC5_SNORM,
        AGPU_TEXTURE_FORMAT_BC6H_UFLOAT,
        AGPU_TEXTURE_FORMAT_BC6H_SFLOAT,
        AGPU_TEXTURE_FORMAT_BC7_UNORM,
        AGPU_TEXTURE_FORMAT_BC7_SRGB,

        _AGPU_TEXTURE_FORMAT_FORCE_U32 = 0x7FFFFFFF
    } agpu_texture_format;

    /* Structs */
    typedef struct agpu_swapchain_info {
        uint32_t width;
        uint32_t height;
        agpu_texture_format color_format;
        agpu_texture_format depth_stencil_format;
        bool vsync;
        bool fullscreen;
        /// Native window handle (HWND, IUnknown, ANativeWindow, NSWindow).
        void* window_handle;
    } agpu_swapchain_info;

    typedef struct agpu_context_info {
        const agpu_swapchain_info* swapchain_info;
    } agpu_context_info;

    typedef struct agpu_init_info {
        agpu_backend_type backend_type;
        bool debug;
        agpu_device_preference device_preference;
    } agpu_init_info;

    typedef struct agpu_features {
        bool independent_blend;
        bool compute_shader;
        bool tessellation_shader;
        bool multi_viewport;
        bool index_uint32;
        bool multi_draw_indirect;
        bool fill_mode_non_solid;
        bool sampler_anisotropy;
        bool texture_compression_ETC2;
        bool texture_compression_ASTC_LDR;
        bool texture_compression_BC;
        bool texture_cube_array;
        bool raytracing;
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

    typedef struct agpu_device_caps {
        agpu_backend_type backend_type;
        uint32_t vendor_id;
        uint32_t device_id;
        agpu_features features;
        agpu_limits limits;
    } agpu_device_caps;

    typedef struct agpu_texture_format_info {
        bool sample;        /* pixel format can be sampled in shaders */
        bool filter;        /* pixel format can be sampled with filtering */
        bool render;        /* pixel format can be used as render target */
        bool blend;         /* alpha-blending is supported */
        bool msaa;          /* pixel format can be used as MSAA render target */
        bool depth;         /* pixel format is a depth format */
    } agpu_texture_format_info;

    /* Callbacks */
    typedef void(AGPU_API_CALL* agpu_log_callback)(void* user_data, agpu_log_level level, const char* message);

    /* Log functions */
    AGPU_API void agpu_set_log_callback(agpu_log_callback callback, void* user_data);
    AGPU_API void agpu_log_error(const char* format, ...);
    AGPU_API void agpu_log_warn(const char* format, ...);
    AGPU_API void agpu_log_info(const char* format, ...);

    AGPU_API agpu_device_caps agpu_query_caps(void);
    AGPU_API agpu_texture_format_info agpu_query_texture_format_info(agpu_texture_format format);

    /* Context */
    AGPU_API agpu_context agpu_create_context(const agpu_context_info* info);
    AGPU_API void agpu_destroy_context(agpu_context context);
    AGPU_API void agpu_set_context(agpu_context context);
    AGPU_API void agpu_begin_frame(void);
    AGPU_API void agpu_end_frame(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

namespace gpu
{
    static constexpr uint32_t kMaxColorAttachments = 8u;
    static constexpr uint32_t kMaxVertexBufferBindings = 8u;
    static constexpr uint32_t kMaxVertexAttributes = 16u;
    static constexpr uint32_t kMaxVertexAttributeOffset = 2047u;
    static constexpr uint32_t kMaxVertexBufferStride = 2048u;
    static constexpr uint32_t kMaxViewportAndScissorRects = 8u;
    static constexpr uint32_t kInvalidHandleId = 0xFFffFFff;

    struct BufferHandle { uint32_t id; bool IsValid() const { return id != kInvalidHandleId; } };
    struct TextureHandle { uint32_t id; bool IsValid() const { return id != kInvalidHandleId; } };

    static constexpr BufferHandle kInvalidBuffer = { kInvalidHandleId };
    static constexpr TextureHandle kInvalidTexture = { kInvalidHandleId };

    using CommandList = uint8_t;
    static constexpr CommandList kMaxCommandLists = 16;

    enum class LogLevel : uint32_t
    {
        Error = AGPU_LOG_LEVEL_ERROR,
        Warn = AGPU_LOG_LEVEL_WARN,
        Info = AGPU_LOG_LEVEL_INFO,
        Debug = AGPU_LOG_LEVEL_DEBUG,
    };

    enum class BackendType : uint32_t
    {
        Null = 0,
        D3D11,
        D3D12,
        Vulkan,
        Count
    };

    enum class InitFlags : uint32_t
    {
        None = 0,
        DebugOutput = (1 << 0),
        VerticalSync = (1 << 1),
        LowPowerGPU = (1 << 2),
    };
    GPU_DEFINE_ENUM_FLAG_OPERATORS(InitFlags, uint32_t);

    /* Structs */
    struct RenderPassColorAttachment
    {
        TextureHandle texture;
        uint32_t mipLevel = 0;
        union {
            //TextureCubemapFace face = TextureCubemapFace::PositiveX;
            uint32_t layer;
            uint32_t slice;
        };
        //LoadAction loadAction = LoadAction::Clear;
        //Color clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    };

    struct RenderPassDepthStencilAttachment
    {
        TextureHandle texture;
        uint32_t mipLevel = 0;
        union {
            //TextureCubemapFace face = TextureCubemapFace::PositiveX;
            uint32_t layer;
            uint32_t slice;
        };
        //LoadAction depthLoadAction = LoadAction::Clear;
       // LoadAction stencilLoadOp = LoadAction::DontCare;
        float clearDepth = 1.0f;
        uint8_t clearStencil = 0;
    };

    struct RenderPassDescriptor
    {
        // Render area will be clipped to the actual framebuffer.
        //RectU renderArea = { UINT32_MAX, UINT32_MAX };

        RenderPassColorAttachment colorAttachments[kMaxColorAttachments];
        RenderPassDepthStencilAttachment depthStencilAttachment;
    };

    void SetPreferredBackend(BackendType backend);
    bool Init(void* windowHandle, InitFlags initFlags);
    void Shutdown();
}

#endif /* AGPU_H */

