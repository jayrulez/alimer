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

#if !defined(AGPU_DEFINE_ENUM_FLAG_OPERATORS)
#define AGPU_DEFINE_ENUM_FLAG_OPERATORS(EnumType, UnderlyingEnumType) \
inline constexpr EnumType operator | (EnumType a, EnumType b) { return (EnumType)((UnderlyingEnumType)(a) | (UnderlyingEnumType)(b)); } \
inline constexpr EnumType& operator |= (EnumType &a, EnumType b) { return a = a | b; } \
inline constexpr EnumType operator & (EnumType a, EnumType b) { return EnumType(((UnderlyingEnumType)a) & ((UnderlyingEnumType)b)); } \
inline constexpr EnumType& operator &= (EnumType &a, EnumType b) { return a = a & b; } \
inline constexpr EnumType operator ~ (EnumType a) { return EnumType(~((UnderlyingEnumType)a)); } \
inline constexpr EnumType operator ^ (EnumType a, EnumType b) { return EnumType(((UnderlyingEnumType)a) ^ ((UnderlyingEnumType)b)); } \
inline constexpr EnumType& operator ^= (EnumType &a, EnumType b) { return a = a ^ b; } \
inline constexpr bool any(EnumType a) { return ((UnderlyingEnumType)a) != 0; }
#endif /* !defined(AGPU_DEFINE_ENUM_FLAG_OPERATORS) */

namespace agpu
{
    /* Constants */
    static constexpr uint32_t kMaxLogMessageLength = 4096u;
    static constexpr uint32_t kInvalidHandleValue = 0xFFffFFff;
    static constexpr uint32_t kMaxColorAttachments = 8u;
    static constexpr uint32_t kMaxVertexBufferBindings = 8u;
    static constexpr uint32_t kMaxVertexAttributes = 16u;
    static constexpr uint32_t kMaxVertexAttributeOffset = 2047u;
    static constexpr uint32_t kMaxVertexBufferStride = 2048u;

    /* Handles */
    struct BufferHandle { uint32_t value; bool isValid() const { return value != kInvalidHandleValue; } };
    struct TextureHandle { uint32_t value; bool isValid() const { return value != kInvalidHandleValue; } };
    struct SharedHandle { uint32_t value; bool isValid() const { return value != kInvalidHandleValue; } };

    static constexpr BufferHandle kInvalidBuffer = { kInvalidHandleValue };
    static constexpr TextureHandle kInvalidTexture = { kInvalidHandleValue };
    static constexpr SharedHandle kInvalidShared = { kInvalidHandleValue };

    /* Enums */
    enum BackendType : uint32_t {
        /// Null renderer.
        Null,
        /// Direct3D 11 backend.
        Direct3D11,
        /// Direct3D 12 backend.
        Direct3D12,
        /// Metal backend.
        Metal,
        /// Vulkan backend.
        Vulkan,
        /// OpenGL 3.3+ or GLES 3.0+ backend.
        OpenGL,
        /// Default best platform supported backend.
        Count
    };

    enum class InitFlags : uint32_t {
        None = 0,
        DebugRuntime = 1 << 0,
        LowPowerGPUPreference = 1 << 1,
        GPUBasedValidation = 1 << 2,
        RenderDoc = 1 << 3,
    };
    AGPU_DEFINE_ENUM_FLAG_OPERATORS(InitFlags, uint32_t);

    enum class LogLevel : uint32_t {
        Error = 0,
        Warn = 1,
        Info = 2,
        Debug = 3,
        Count
    };

    /// Defines pixel format.
    enum class PixelFormat : uint32_t
    {
        Invalid = 0,
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
        RGBA8Unorm,
        RGBA8UnormSrgb,
        RGBA8Snorm,
        RGBA8Uint,
        RGBA8Sint,
        BGRA8Unorm,
        BGRA8UnormSrgb,
        // Packed 32-Bit Pixel formats
        RGB10A2Unorm,
        RG11B10Float,
        RGB9E5Float,
        // 64-Bit Pixel Formats
        RG32Uint,
        RG32Sint,
        RG32Float,
        RGBA16Unorm,
        RGBA16Snorm,
        RGBA16Uint,
        RGBA16Sint,
        RGBA16Float,
        // 128-Bit Pixel Formats
        RGBA32Uint,
        RGBA32Sint,
        RGBA32Float,
        // Depth-stencil formats
        Depth16Unorm,
        Depth32Float,
        Depth24UnormStencil8,
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
        Count
    };

    /* Struct */
    struct Features {
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
    };

    struct Limits {
        uint32_t        max_vertex_attributes;
        uint32_t        max_vertex_bindings;
        uint32_t        max_vertex_attribute_offset;
        uint32_t        max_vertex_binding_stride;
        uint32_t        maxTextureDimension1D;
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
    };

    struct Caps {
        BackendType backend;
        uint32_t vendorID;
        uint32_t deviceID;
        Features features;
        Limits limits;
    };

    struct PresentationParameters
    {
        void* windowHandle;
        uint32_t backBufferWidth;
        uint32_t backBufferHeight;
        PixelFormat colorFormat = PixelFormat::BGRA8Unorm;
        bool enableVSync = false;
        bool isFullscreen = false;
    };

    /* Callbacks */
    typedef void(AGPU_API_CALL* logCallback)(void* userData, LogLevel level, const char* message);

    /* Log functions */
    AGPU_API void setLogCallback(logCallback callback, void* user_data);
    AGPU_API void logError(const char* format, ...);
    AGPU_API void logWarn(const char* format, ...);
    AGPU_API void logInfo(const char* format, ...);

    AGPU_API bool setPreferredBackend(BackendType backend);
    AGPU_API bool init(InitFlags flags, const PresentationParameters* presentationParameters);
    AGPU_API void shutdown(void);
    AGPU_API void resize(uint32_t width, uint32_t height);
    AGPU_API bool beginFrame(void);
    AGPU_API void endFrame(void);

    AGPU_API const Caps* QueryCaps(void);
    //AGPU_API agpu_texture_format_info agpu_query_texture_format_info(agpu_texture_format format);

    AGPU_API BufferHandle CreateBuffer(uint32_t count, uint32_t stride, const void* initialData);
    AGPU_API void DestroyBuffer(BufferHandle handle);

    /* Commands */
    AGPU_API void PushDebugGroup(const char* name);
    AGPU_API void PopDebugGroup(void);
    AGPU_API void InsertDebugMarker(const char* name);
}
