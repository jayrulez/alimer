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

#pragma once

#if defined(VGPU_SHARED_LIBRARY)
#   if defined(_WIN32)
#       if defined(VGPU_IMPLEMENTATION)
#           define VGPU_API __declspec(dllexport)
#       else
#           define VGPU_API __declspec(dllimport)
#       endif
#   else  // defined(_WIN32)
#       if defined(VGPU_IMPLEMENTATION)
#           define VGPU_API __attribute__((visibility("default")))
#       else
#           define VGPU_API
#       endif
#   endif  // defined(_WIN32)
#else       // defined(VGPU_SHARED_LIBRARY)
#   define VGPU_API
#endif  // defined(VGPU_SHARED_LIBRARY)

#if defined(_WIN32)
#   define VGPU_CALL __cdecl
#else
#   define VGPU_CALL
#endif

#include <stdint.h>

#ifndef VGPU_ASSERT
#   include <assert.h>
#   define VGPU_ASSERT(c) assert(c)
#endif

#if !defined(VGPU_DEFINE_ENUM_FLAG_OPERATORS)
#define VGPU_DEFINE_ENUM_FLAG_OPERATORS(EnumType, UnderlyingEnumType) \
inline constexpr EnumType operator | (EnumType a, EnumType b) { return (EnumType)((UnderlyingEnumType)(a) | (UnderlyingEnumType)(b)); } \
inline constexpr EnumType& operator |= (EnumType &a, EnumType b) { return a = a | b; } \
inline constexpr EnumType operator & (EnumType a, EnumType b) { return EnumType(((UnderlyingEnumType)a) & ((UnderlyingEnumType)b)); } \
inline constexpr EnumType& operator &= (EnumType &a, EnumType b) { return a = a & b; } \
inline constexpr EnumType operator ~ (EnumType a) { return EnumType(~((UnderlyingEnumType)a)); } \
inline constexpr EnumType operator ^ (EnumType a, EnumType b) { return EnumType(((UnderlyingEnumType)a) ^ ((UnderlyingEnumType)b)); } \
inline constexpr EnumType& operator ^= (EnumType &a, EnumType b) { return a = a ^ b; } \
inline constexpr bool any(EnumType a) { return ((UnderlyingEnumType)a) != 0; }
#endif

namespace vgpu
{
    /* Constants */
    static constexpr uint32_t kInvalidId = { 0xFFffFFff };
    enum {
        VGPU_NUM_INFLIGHT_FRAMES = 2u,
        VGPU_MAX_COLOR_ATTACHMENTS = 8u,
        VGPU_MAX_VERTEX_BUFFER_BINDINGS = 8u,
        VGPU_MAX_VERTEX_ATTRIBUTES = 16u,
        VGPU_MAX_VERTEX_ATTRIBUTE_OFFSET = 2047u,
        VGPU_MAX_VERTEX_BUFFER_STRIDE = 2048u,
    };

    /* Handles*/
    struct BufferHandle { uint32_t id; bool isValid() const { return id != kInvalidId; } };
    struct TextureHandle { uint32_t id; bool isValid() const { return id != kInvalidId; } };
    typedef struct vgpu_framebuffer_t* vgpu_framebuffer;

    static constexpr BufferHandle kInvalidBuffer = { kInvalidId };
    static constexpr TextureHandle kInvalidTexture = { kInvalidId };

    /* Enums */
    typedef enum vgpu_log_level {
        VGPU_LOG_LEVEL_ERROR = 0,
        VGPU_LOG_LEVEL_WARN = 1,
        VGPU_LOG_LEVEL_INFO = 2,
        VGPU_LOG_LEVEL_DEBUG = 3,
        _VGPU_LOG_LEVEL_COUNT,
        _VGPU_LOG_LEVEL_FORCE_U32 = 0x7FFFFFFF
    } vgpu_log_level;

    enum class BackendType : uint32_t {
        Null,
        Vulkan,
        Direct3D11,
        Count
    };

    /// Defines pixel format.
    enum class PixelFormat : uint32_t {
        Invalid = 0,
        // 8-bit pixel formats
        R8Unorm,
        R8Snorm,
        R8Uint,
        R8Sint,
        // 16-bit pixel formats
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
        VGPU_PIXEL_FORMAT_RGBA16_UINT,
        VGPU_PIXEL_FORMAT_RGBA16_SINT,
        VGPU_PIXEL_FORMAT_RGBA16_FLOAT,
        // 128-Bit Pixel Formats
        VGPU_PIXEL_FORMAT_RGBA32_UINT,
        VGPU_PIXEL_FORMAT_RGBA32_SINT,
        VGPU_PIXEL_FORMAT_RGBA32_FLOAT,
        // Depth-stencil
        VGPU_PIXEL_FORMAT_DEPTH32_FLOAT,
        VGPU_PIXEL_FORMAT_DEPTH24_STENCIL8,
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
        VGPU_PIXEL_FORMAT_BC6HRGB_UFLOAT,
        VGPU_PIXEL_FORMAT_BC6HRGB_SFLOAT,
        VGPU_PIXEL_FORMAT_BC7RGBA_UNORM,
        VGPU_PIXEL_FORMAT_BC7RGBA_UNORM_SRGB,

        Count,
    };

    /// Defines pixel format type.
    enum class PixelFormatType {
        /// Unknown format Type
        Unknown,
        /// floating-point formats.
        Float,
        /// Unsigned normalized formats.
        Unorm,
        /// Unsigned normalized SRGB formats
        UnormSrgb,
        /// Signed normalized formats.
        Snorm,
        /// Unsigned integer formats.
        Uint,
        /// Signed integer formats.
        Sint
    };

    enum class PixelFormatAspect {
        Color,
        Depth,
        Stencil,
        DepthStencil
    };

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

    typedef enum vgpu_load_op {
        VGPU_LOAD_OP_CLEAR = 0,
        VGPU_LOAD_OP_LOAD = 1,
        _VGPU_LOAD_OP_FORCE_U32 = 0x7FFFFFFF
    } vgpu_load_op;

    /* Structs */
    typedef struct vgpu_color {
        float r;
        float g;
        float b;
        float a;
    } vgpu_color;

    typedef struct vgpu_texture_info {
        vgpu_texture_type type;
        PixelFormat format;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t array_layers;
        uint32_t mip_levels;
        uint32_t usage;
        const void* content;
        const char* label;
        uintptr_t external_handle;
    } vgpu_texture_info;

    typedef struct vgpu_attachment_info {
        TextureHandle texture;
        uint32_t level;
        uint32_t slice;
    } vgpu_attachment_info;

    typedef struct vgpu_color_attachment_info {
        TextureHandle texture;
        uint32_t level;
        uint32_t slice;
        vgpu_load_op load_op;
        vgpu_color clear_color;
    } vgpu_color_attachment_info;

    typedef struct vgpu_depth_stencil_pass_info {
        vgpu_load_op depth_load_op;
        float clear_depth;
        vgpu_load_op stencil_load_op;
        uint8_t clear_stencil;
    } vgpu_depth_stencil_pass_info;

    typedef struct vgpu_framebuffer_info {
        uint32_t width;
        uint32_t height;
        uint32_t layers;
        vgpu_attachment_info color_attachments[VGPU_MAX_COLOR_ATTACHMENTS];
        vgpu_attachment_info depth_stencil;
    } vgpu_framebuffer_info;

    struct SwapchainInfo {
        void* display;
        void* handle;
        uint32_t            width;
        uint32_t            height;
        PixelFormat         colorFormat = PixelFormat::VGPU_PIXEL_FORMAT_BGRA8_UNORM;
        PixelFormat         depthStencilFormat = PixelFormat::Invalid;
        bool                isFullscreen;
    };

    enum class InitFlags : uint32_t {
        None = 0,
        DebugRutime = (1 << 0),
        GPUBasedValidation = (1 << 1),
        GPUPreferenceLowPower = (1 << 2),
        RenderDoc = (1 << 3)
    };
    VGPU_DEFINE_ENUM_FLAG_OPERATORS(InitFlags, uint32_t);

    typedef struct vgpu_allocation_callbacks {
        void* user_data;
        void* (VGPU_CALL* allocate)(void* user_data, size_t size);
        void* (VGPU_CALL* allocate_cleared)(void* user_data, size_t size);
        void (VGPU_CALL* free)(void* user_data, void* ptr);
    } vgpu_allocation_callbacks;

    typedef struct vgpu_features {
        bool independent_blend;
        bool computeShader;
        bool tessellationShader;
        bool multiViewport;
        bool indexUInt32;
        bool multiDrawIndirect;
        bool fillModeNonSolid;
        bool samplerAnisotropy;
        bool textureCompressionETC2;
        bool textureCompressionASTC_LDR;
        bool textureCompressionBC;
        bool textureCubeArray;
        bool raytracing;
    } vgpu_features;

    typedef struct vgpu_limits {
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
    } vgpu_limits;

    struct Caps {
        BackendType backendType;
        uint32_t vendorId;
        uint32_t deviceId;
        vgpu_features features;
        vgpu_limits limits;
    };

    /* Log functions */
    typedef void(VGPU_CALL* LogCallback)(void* user_data, vgpu_log_level level, const char* message);

    VGPU_API void setLogCallback(LogCallback callback, void* userData);
    VGPU_API void log(vgpu_log_level level, const char* format, ...);
    VGPU_API void logError(const char* format, ...);
    VGPU_API void logInfo(const char* format, ...);

    VGPU_API bool init(InitFlags flags, const SwapchainInfo& swapchainInfo);
    VGPU_API void shutdown(void);
    VGPU_API void beginFrame(void);
    VGPU_API void endFrame(void);
    VGPU_API const Caps* queryCaps();

    VGPU_API TextureHandle vgpu_create_texture(const vgpu_texture_info* info);
    VGPU_API void vgpu_destroy_texture(TextureHandle texture);

    VGPU_API vgpu_framebuffer vgpu_create_framebuffer(const vgpu_framebuffer_info* info);
    VGPU_API void vgpu_destroy_framebuffer(vgpu_framebuffer framebuffer);

    /* commands */
    VGPU_API TextureHandle vgpu_get_backbuffer_texture(void);
    //VGPU_API void vgpu_begin_pass(const vgpu_pass_begin_info* info);
    //VGPU_API void vgpu_end_pass(void);

    /* pixel format helpers */
    struct PixelFormatDesc
    {
        PixelFormat format;
        const char* name;
        uint32_t bytesPerBlock;
        uint32_t channelCount;
        PixelFormatType type;
        struct
        {
            bool isDepth;
            bool isStencil;
            bool isCompressed;
        };
        struct
        {
            uint32_t width;
            uint32_t height;
        } compressionRatio;
        int numChannelBits[4];
    };

    extern const VGPU_API PixelFormatDesc kFormatDesc[];

    /// Get the number of bytes per format
    inline uint32_t getFormatBytesPerBlock(PixelFormat format)
    {
        VGPU_ASSERT(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].bytesPerBlock;
    }

    inline uint32_t getFormatPixelsPerBlock(PixelFormat format)
    {
        VGPU_ASSERT(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].compressionRatio.width * kFormatDesc[(uint32_t)format].compressionRatio.height;
    }

    /// Check if the format has a depth component
    inline bool isDepthFormat(PixelFormat format)
    {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].isDepth;
    }

    /// Check if the format has a stencil component
    inline bool isStencilFormat(PixelFormat format)
    {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].isStencil;
    }

    /// Check if the format has depth or stencil components
    inline bool isDepthStencilFormat(PixelFormat format)
    {
        return isDepthFormat(format) || isStencilFormat(format);
    }

    /// Check if the format is a compressed format
    inline bool isCompressedFormat(PixelFormat format)
    {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].isCompressed;
    }

    /// Get the format compression ration along the x-axis.
    inline uint32_t getFormatWidthCompressionRatio(PixelFormat format)
    {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].compressionRatio.width;
    }

    /// Get the format compression ration along the y-axis
    inline uint32_t getFormatHeightCompressionRatio(PixelFormat format)
    {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].compressionRatio.height;
    }

    /// Get the number of channels
    inline uint32_t getFormatChannelCount(PixelFormat format)
    {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].channelCount;
    }

    /** Get the format Type
    */
    inline PixelFormatType getFormatType(PixelFormat format)
    {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].type;
    }

    inline uint32_t getNumChannelBits(PixelFormat format, int channel)
    {
        return kFormatDesc[(uint32_t)format].numChannelBits[channel];
    }

    /// Check if a format represents sRGB color space
    inline bool isSrgbFormat(PixelFormat format)
    {
        return (getFormatType(format) == PixelFormatType::UnormSrgb);
    }
}
