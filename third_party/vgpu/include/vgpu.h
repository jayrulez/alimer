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
typedef struct vgpu_texture { uint32_t id; } vgpu_texture;
typedef struct vgpu_buffer_t* vgpu_buffer;
typedef struct vgpu_framebuffer_t* vgpu_framebuffer;

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
    uint32_t format;
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

/* Device/Context */
VGPU_API bool vgpu_frame_begin(void);
VGPU_API void vgpu_frame_finish(void);

VGPU_API void vgpuInsertDebugMarker(const char* name);
VGPU_API void vgpuPushDebugGroup(const char* name);
VGPU_API void vgpuPopDebugGroup(void);
VGPU_API void vgpu_render_begin(vgpu_framebuffer framebuffer);
VGPU_API void vgpu_render_finish(void);

#ifdef __cplusplus
#ifndef MAKE_ENUM_FLAG
#define MAKE_ENUM_FLAG(TYPE, ENUM_TYPE)                                                                        \
	static inline ENUM_TYPE operator|(ENUM_TYPE a, ENUM_TYPE b) { return (ENUM_TYPE)((TYPE)(a) | (TYPE)(b)); } \
	static inline ENUM_TYPE operator&(ENUM_TYPE a, ENUM_TYPE b) { return (ENUM_TYPE)((TYPE)(a) & (TYPE)(b)); } \
	static inline ENUM_TYPE operator|=(ENUM_TYPE& a, ENUM_TYPE b) { return a = (a | b); }                      \
	static inline ENUM_TYPE operator&=(ENUM_TYPE& a, ENUM_TYPE b) { return a = (a & b); }

#endif
#else
#define MAKE_ENUM_FLAG(TYPE, ENUM_TYPE)
#endif

#ifdef __cplusplus
namespace vgpu
{
    /* Constants */
    static constexpr uint32_t kInvalidHandle = VGPU_INVALID_ID;
    static constexpr uint32_t kMaxColorAttachments = 8u;
    static constexpr uint32_t kMaxVertexBufferBindings = 8u;
    static constexpr uint32_t kMaxVertexAttributes = 16u;
    static constexpr uint32_t kMaxVertexAttributeOffset = 2047u;
    static constexpr uint32_t kMaxVertexBufferStride = 2048u;

    /* Handles */
    struct Context { uint32_t value; bool isValid() const { return value != kInvalidHandle; } };
    struct Texture { uint32_t value; bool isValid() const { return value != kInvalidHandle; } };
    const Context kInvalidContext = { kInvalidHandle };
    const Texture kInvalidTexture = { kInvalidHandle };

    /* Enums */
    enum class BackendType : uint32_t {
        Default,
        Null,
        D3D11,
        D3D12,
        Metal,
        Vulkan,
        OpenGL
    };

    enum class InitFlags : uint32_t {
        None = 0,
        DebugOutput         = (1 << 0),
        GPUBasedValidation  = (1 << 1),
        LowPower            = (1 << 2),
        RenderDoc           = (1 << 3),
    };
    MAKE_ENUM_FLAG(uint32_t, InitFlags);

    enum class PixelFormat : uint32_t {
        Undefined = 0,
        // 8-bit pixel formats
        R8Unorm,
        R8Snorm,
        R8Uint,
        R8Sint,
        // 16-bit pixel formats
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
        // 64-bit pixel formats
        RG32Uint,
        RG32Sint,
        RG32Float,
        RGBA16Uint,
        RGBA16Sint,
        RGBA16Float,
        // 128-bit pixel formats
        RGBA32Uint,
        RGBA32Sint,
        RGBA32Float,
        // Depth and Stencil Pixel Formats
        Depth32Float,
        Depth24UnormStencil8,
        // Compressed BC Pixel Formats
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
        BC6HRGBSfloat,
        BC7RGBAUnorm,
        BC7RGBAUnormSrgb,
    };

    enum class PixelFormatAspect : uint32_t {
        Color,
        Depth,
        Stencil,
        DepthStencil,
    };

    enum class PixelFormatType : uint32_t {
        Unknown = 0,
        Float,
        Unorm,
        UnormSrgb,
        Snorm,
        Sint,
        Uint
    };

    /* Structs */
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

    struct Caps {
        BackendType backend_type;
        vgpu_features features;
        vgpu_limits limits;
    };

    /* API calls */
    _VGPU_API_DECL void setPreferredBackend(BackendType backendType);
    _VGPU_API_DECL bool init(void* windowHandle, InitFlags flags = InitFlags::None);
    _VGPU_API_DECL void shutdown(void);
    _VGPU_API_DECL const Caps* getCaps();

    /* Helper methods */
    /**
    * Get the number of bytes per format
    */
    _VGPU_API_DECL uint32_t getFormatBytesPerBlock(PixelFormat format);
    _VGPU_API_DECL uint32_t getFormatPixelsPerBlock(PixelFormat format);
    
    /**
    * Check if the format has a depth component.
    */
    _VGPU_API_DECL bool isDepthFormat(PixelFormat format);
    /**
    * Check if the format has a stencil component.
    */
    _VGPU_API_DECL bool isStencilFormat(PixelFormat format);
    /**
    * Check if the format has depth or stencil components.
    */
    _VGPU_API_DECL bool isDepthStencilFormat(PixelFormat format);
    /**
    * Check if the format is a compressed format.
    */
    _VGPU_API_DECL bool isCompressedFormat(PixelFormat format);

    /**
    * Get the format compression ration along the x-axis.
    */
    _VGPU_API_DECL uint32_t getFormatWidthCompressionRatio(PixelFormat format);

    /**
    * Get the format compression ration along the y-axis.
    */
    _VGPU_API_DECL uint32_t getFormatHeightCompressionRatio(PixelFormat format);

    /**
    * Get the number of channels.
    */
    _VGPU_API_DECL uint32_t getFormatChannelCount(PixelFormat format);

    /**
    * Get the format type.
    */
    _VGPU_API_DECL PixelFormatType getFormatType(PixelFormat format);

    /**
    * Get the format aspect.
    */
    _VGPU_API_DECL PixelFormatAspect getFormatAspect(PixelFormat format);

    /**
    * Check if a format represents sRGB color space.
    */
    _VGPU_API_DECL bool isSrgbFormat(PixelFormat format);

    /**
    * Convert an SRGB format to linear. If the format is already linear, will return it.
    */
    _VGPU_API_DECL PixelFormat srgbToLinearFormat(PixelFormat format);

    /**
    * Convert an linear format to sRGB. If the format doesn't have a matching sRGB format, will return the original.
    */
    _VGPU_API_DECL PixelFormat linearToSrgbFormat(PixelFormat format);
}
#endif

#endif /* VGPU_H */
