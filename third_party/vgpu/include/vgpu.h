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
    typedef struct VGPUTexture_T* VGPUTexture;
    typedef struct vgpu_pipeline_t* vgpu_pipeline;

    /* Constants */
    enum {
        VGPU_MAX_PHYSICAL_DEVICE_NAME_SIZE = 256u,
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

    typedef enum vgpu_adapter_type {
        VGPU_ADAPTER_TYPE_OTHER,
        VGPU_ADAPTER_TYPE_INTEGRATED_GPU,
        VGPU_ADAPTER_TYPE_DISCRETE_GPU,
        VGPU_ADAPTER_TYPE_VIRTUAL_GPU,
        VGPU_ADAPTER_TYPE_CPU,
        _VGPU_ADAPTER_TYPE_FORCE_U32 = 0x7FFFFFFF
    } vgpu_adapter_type;

    /// Defines pixel format.
    typedef enum VGPUTextureFormat {
        VGPUTextureFormat_Undefined = 0,
        // 8-bit pixel formats
        VGPUTextureFormat_R8Unorm = 0x00000001,
        VGPUTextureFormat_R8Snorm = 0x00000002,
        VGPUTextureFormat_R8Uint = 0x00000003,
        VGPUTextureFormat_R8Sint = 0x00000004,
        // 16-bit pixel formats
        VGPUTextureFormat_R16Unorm,
        VGPUTextureFormat_R16Snorm,
        VGPUTextureFormat_R16Uint,
        VGPUTextureFormat_R16Sint,
        VGPUTextureFormat_R16Float,
        VGPUTextureFormat_RG8Unorm,
        VGPUTextureFormat_RG8Snorm,
        VGPUTextureFormat_RG8Uint,
        VGPUTextureFormat_RG8Sint,
        // 32-bit pixel formats
        VGPUTextureFormat_R32Uint,
        VGPUTextureFormat_R32Sint,
        VGPUTextureFormat_R32Float,
        VGPUTextureFormat_RG16Unorm,
        VGPUTextureFormat_RG16Snorm,
        VGPUTextureFormat_RG16Uint,
        VGPUTextureFormat_RG16Sint,
        VGPUTextureFormat_RG16Float,
        VGPUTextureFormat_RGBA8Unorm,
        VGPUTextureFormat_RGBA8UnormSrgb,
        VGPUTextureFormat_RGBA8Snorm,
        VGPUTextureFormat_RGBA8Uint,
        VGPUTextureFormat_RGBA8Sint,
        VGPUTextureFormat_BGRA8Unorm,
        VGPUTextureFormat_BGRA8UnormSrgb,
        // Packed 32-Bit Pixel formats
        VGPUTextureFormat_RGB10A2Unorm,
        VGPUTextureFormat_RG11B10Ufloat,
        VGPUTextureFormat_RGB9E5Ufloat,
        // 64-Bit Pixel Formats
        VGPUTextureFormat_RG32Uint,
        VGPUTextureFormat_RG32Sint,
        VGPUTextureFormat_RG32Float,
        VGPUTextureFormat_RGBA16Unorm,
        VGPUTextureFormat_RGBA16Snorm,
        VGPUTextureFormat_RGBA16Uint,
        VGPUTextureFormat_RGBA16Sint,
        VGPUTextureFormat_RGBA16Float,
        // 128-Bit Pixel Formats
        VGPUTextureFormat_RGBA32Uint,
        VGPUTextureFormat_RGBA32Sint,
        VGPUTextureFormat_RGBA32Float,
        // Depth-stencil formats
        VGPUTextureFormat_Depth16Unorm,
        VGPUTextureFormat_Depth32Float,
        VGPUTextureFormat_Stencil8,
        VGPUTextureFormat_Depth24UnormStencil8,
        VGPUTextureFormat_Depth32FloatStencil8,
        // Compressed BC formats
        VGPUTextureFormat_BC1RGBAUnorm,
        VGPUTextureFormat_BC1RGBAUnormSrgb,
        VGPUTextureFormat_BC2RGBAUnorm,
        VGPUTextureFormat_BC2RGBAUnormSrgb,
        VGPUTextureFormat_BC3RGBAUnorm,
        VGPUTextureFormat_BC3RGBAUnormSrgb,
        VGPUTextureFormat_BC4RUnorm,
        VGPUTextureFormat_BC4RSnorm,
        VGPUTextureFormat_BC5RGUnorm,
        VGPUTextureFormat_BC5RGSnorm,
        VGPUTextureFormat_BC6HRGBUfloat,
        VGPUTextureFormat_BC6HRGBFloat,
        VGPUTextureFormat_BC7RGBAUnorm,
        VGPUTextureFormat_BC7RGBAUnormSrgb,
        // Compressed PVRTC Pixel Formats
        VGPUTextureFormat_PVRTC_RGB2,
        VGPUTextureFormat_PVRTC_RGBA2,
        VGPUTextureFormat_PVRTC_RGB4,
        VGPUTextureFormat_PVRTC_RGBA4,
        // Compressed ETC Pixel Formats
        VGPUTextureFormat_ETC2_RGB8,
        VGPUTextureFormat_ETC2_RGB8Srgb,
        VGPUTextureFormat_ETC2_RGB8A1,
        VGPUTextureFormat_ETC2_RGB8A1Srgb,
        // Compressed ASTC Pixel Formats
        VGPUTextureFormat_ASTC_4x4,
        VGPUTextureFormat_ASTC_5x4,
        VGPUTextureFormat_ASTC_5x5,
        VGPUTextureFormat_ASTC_6x5,
        VGPUTextureFormat_ASTC_6x6,
        VGPUTextureFormat_ASTC_8x5,
        VGPUTextureFormat_ASTC_8x6,
        VGPUTextureFormat_ASTC_8x8,
        VGPUTextureFormat_ASTC_10x5,
        VGPUTextureFormat_ASTC_10x6,
        VGPUTextureFormat_ASTC_10x8,
        VGPUTextureFormat_ASTC_10x10,
        VGPUTextureFormat_ASTC_12x10,
        VGPUTextureFormat_ASTC_12x12,

        VGPUTextureFormat_Count,
        VGPUTextureFormat_Force32 = 0x7FFFFFFF
    } VGPUTextureFormat;

    /// Defines pixel format type.
    typedef enum VGPUTextureFormatType {
        /// Unknown format Type
        VGPUTextureFormatType_Unknown = 0,
        /// floating-point formats.
        VGPUTextureFormatType_Float,
        /// Unsigned normalized formats.
        VGPUTextureFormatType_Unorm,
        /// Unsigned normalized SRGB formats.
        VGPUTextureFormatType_UnormSrgb,
        /// Signed normalized formats.
        VGPUTextureFormatType_Snorm,
        /// Unsigned integer formats.
        VGPUTextureFormatType_Uint,
        /// Signed integer formats.
        VGPUTextureFormatType_Sint,
        VGPUTextureFormatType_Force32 = 0x7FFFFFFF
    } VGPUTextureFormatType;

    typedef enum VGPUTextureType {
        VGPUTextureType_2D,
        VGPUTextureType_3D,
        VGPUTextureType_Cube,
        VGPUTextureType_Force32 = 0x7FFFFFFF
    } VGPUTextureType;

    typedef enum VGPUTextureUsage {
        VGPUTextureUsage_None = 0x00000000,
        VGPUTextureUsage_CopySrc = 0x00000001,
        VGPUTextureUsage_CopyDst = 0x00000002,
        VGPUTextureUsage_Sampled = 0x00000004,
        VGPUTextureUsage_Storage = 0x00000008,
        VGPUTextureUsage_OutputAttachment = 0x00000010,
        VGPUTextureUsage_Force32 = 0x7FFFFFFF
    } VGPUTextureUsage;

    typedef enum vgpu_load_op {
        VGPU_LOAD_OP_CLEAR = 0,
        VGPU_LOAD_OP_LOAD = 1,
        _VGPU_LOAD_OP_FORCE_U32 = 0x7FFFFFFF
    } vgpu_load_op;

    /* Structs */
    typedef struct VGPUExtent2D {
        uint32_t width;
        uint32_t height;
    } VGPUExtent2D;

    typedef struct VGPUExtent3D {
        uint32_t width;
        uint32_t height;
        uint32_t depth;
    } VGPUExtent3D;

    typedef struct VGPUColor {
        float r;
        float g;
        float b;
        float a;
    } VGPUColor;

    typedef struct vgpu_texture_info {
        VGPUTextureType         type;
        VGPUTextureUsage        usage;
        VGPUTextureFormat       format;
        VGPUExtent3D            size;
        uint32_t                mip_level_count;
        uint32_t                sample_count;
        const uint64_t          external_handle;
        const char*             label;
    } vgpu_texture_info;

    typedef struct VGPUTextureViewDescriptor {
        VGPUTexture         source;
        VGPUTextureType     type;
        VGPUTextureFormat   format;
        uint32_t            baseMipmap;
        uint32_t            mipmapCount;
        uint32_t            baseLayer;
        uint32_t            layerCount;
    } VGPUTextureViewDescriptor;

    typedef struct vgpu_shader_source {
        const void*     code;
        size_t          size;
        const char*     entry;
    } vgpu_shader_source;

    typedef struct vgpu_shader_info {
        vgpu_shader_source  vertex;
        vgpu_shader_source  fragment;
        vgpu_shader_source  compute;
        const char*         label;
    } vgpu_shader_info;

    typedef struct vgpu_pipeline_info {
        vgpu_shader     shader;
        const char*     label;
    } vgpu_pipeline_info;

    typedef struct vgpu_color_attachment {
        VGPUTexture     texture;
        uint32_t        level;
        uint32_t        slice;
        vgpu_load_op    load_op;
        VGPUColor       clearColor;
    } vgpu_color_attachment;

    typedef struct vgpu_depth_stencil_attachment {
        VGPUTexture     texture;
        uint32_t        level;
        uint32_t        slice;
        vgpu_load_op    depth_load_op;
        vgpu_load_op    stencil_load_op;
        float           clear_depth;
        uint8_t         clear_stencil;
    } vgpu_depth_stencil_attachment;

    typedef struct vgpu_render_pass_info {
        uint32_t                        num_color_attachments;
        vgpu_color_attachment           color_attachments[VGPU_MAX_COLOR_ATTACHMENTS];
        vgpu_depth_stencil_attachment   depth_stencil;
    } vgpu_render_pass_info;

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
        uint32_t        max_uniform_buffer_range;
        uint64_t        min_uniform_buffer_offset_alignment;
        uint32_t        max_storage_buffer_range;
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
        uint32_t        max_compute_work_group_count[3];
        uint32_t        max_compute_work_group_invocations;
        uint32_t        max_compute_work_group_size[3];
    } vgpu_limits;

    typedef struct vgpu_caps {
        vgpu_backend_type   backend;
        uint32_t            vendor_id;
        uint32_t            adapter_id;
        vgpu_adapter_type   adapter_type;
        char                adapter_name[VGPU_MAX_PHYSICAL_DEVICE_NAME_SIZE];
        vgpu_features       features;
        vgpu_limits         limits;
    } vgpu_caps;

    typedef struct vgpu_swapchain_info {
        void*                   window_handle;
        VGPUTextureFormat       color_format;
        VGPUTextureFormat       depth_stencil_format;
        bool                    vsync;
        uint32_t                sample_count;
    } vgpu_swapchain_info;

    typedef struct vgpu_config {
        bool                    debug;
        vgpu_adapter_type       device_preference;
        vgpu_swapchain_info     swapchain_info;
    } vgpu_config;

    /* Log functions */
    typedef void(VGPU_API_CALL* vgpu_log_callback)(void* userData, vgpu_log_level level, const char* message);
    VGPU_API void vgpu_set_log_callback(vgpu_log_callback callback, void* user_data);
    VGPU_API void vgpu_log(vgpu_log_level level, const char* format, ...);

    /* Frame logic */
    VGPU_API bool vgpu_set_preferred_backend(vgpu_backend_type backend);
    VGPU_API bool vgpu_init(const char* app_name, const vgpu_config* config);
    VGPU_API void vgpu_shutdown(void);
    VGPU_API void vgpu_query_caps(vgpu_caps* caps);
    VGPU_API bool vgpu_begin_frame(void);
    VGPU_API void vgpu_end_frame(void);

    /* Buffer */
    typedef enum vgpu_buffer_type {
        VGPU_BUFFER_TYPE_VERTEX,
        VGPU_BUFFER_TYPE_INDEX,
        VGPU_BUFFER_TYPE_UNIFORM,
        _VGPU_BUFFER_TYPE_FORCE_U32 = 0x7FFFFFFF
    } vgpu_buffer_type;

    typedef enum vgpu_buffer_usage {
        VGPU_BUFFER_USAGE_IMMUTABLE,
        VGPU_BUFFER_USAGE_DYNAMIC,
        VGPU_BUFFER_USAGE_STREAM,
        _VGPU_BUFFER_USAGE_FORCE_U32 = 0x7FFFFFFF
    } vgpu_buffer_usage;

    typedef struct vgpu_buffer_info {
        uint64_t size;
        vgpu_buffer_type type;
        vgpu_buffer_usage usage;
        const void* data;
        const char* label;
    } vgpu_buffer_info;

    VGPU_API vgpu_buffer vgpu_create_buffer(const vgpu_buffer_info* info);
    VGPU_API void vgpu_destroy_buffer(vgpu_buffer buffer);

    /* Shader */
    VGPU_API vgpu_shader vgpu_create_shader(const vgpu_shader_info* info);
    VGPU_API void vgpu_destroy_shader(vgpu_shader shader);

    /* Texture */
    VGPU_API VGPUTexture vgpuTextureCreate(const vgpu_texture_info* info);
    VGPU_API void vgpuTextureDestroy(VGPUTexture handle);
    VGPU_API bool vgpuTextureInitView(VGPUTexture texture, const VGPUTextureViewDescriptor* descriptor);
    VGPU_API uint64_t vgpuTextureGetNativeHandle(VGPUTexture handle);

    /* Pipeline */
    VGPU_API vgpu_pipeline vgpu_create_pipeline(const vgpu_pipeline_info* info);
    VGPU_API void vgpu_destroy_pipeline(vgpu_pipeline pipeline);

    /* Commands */
    VGPU_API void vgpu_push_debug_group(const char* name);
    VGPU_API void vgpu_pop_debug_group(void);
    VGPU_API void vgpu_insert_debug_marker(const char* name);
    VGPU_API void vgpu_begin_render_pass(const vgpu_render_pass_info* info);
    VGPU_API void vgpu_end_render_pass(void);
    VGPU_API void vgpu_bind_pipeline(vgpu_pipeline pipeline);
    VGPU_API void vgpu_draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex);

    /* Utility methods */
    /// Check if the format has a depth component.
    VGPU_API bool vgpuIsDepthFormat(VGPUTextureFormat format);
    /// Check if the format has a stencil component.
    VGPU_API bool vgpuIsStencilFormat(VGPUTextureFormat format);
    /// Check if the format has depth or stencil components.
    VGPU_API bool vgpuIsDepthStencilFormat(VGPUTextureFormat format);
    /// Check if the format is a compressed format.
    VGPU_API bool vgpuIsCompressedFormat(VGPUTextureFormat format);
    /// Check if the format is BC compressed format.
    VGPU_API bool vgpuIsBcCompressedFormat(VGPUTextureFormat format);

    VGPU_API uint32_t vgpu_calculate_mip_levels(uint32_t width, uint32_t height, uint32_t depth);

#ifdef __cplusplus
}
#endif

#endif /* __VGPU_H__ */

