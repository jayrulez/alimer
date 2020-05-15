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

#ifndef VGPU_H_
#define VGPU_H_

#if defined(VGPU_SHARED_LIBRARY)
#   if defined(_WIN32)
#       if defined(VGPU_IMPLEMENTATION)
#           define _VGPU_API_DECL __declspec(dllexport)
#       else
#           define _VGPU_API_DECL __declspec(dllimport)
#       endif
#   else  // defined(_WIN32)
#       define _VGPU_API_DECL __attribute__((visibility("default")))
#   endif  // defined(_WIN32)
#else       // defined(VGPU_SHARED_LIBRARY)
#   define _VGPU_API_DECL
#endif  // defined(VGPU_SHARED_LIBRARY)

#ifdef __cplusplus
#   define VGPU_API extern "C" _VGPU_API_DECL
#else
#   define VGPU_API extern _VGPU_API_DECL
#endif

#include <stdint.h>
#include <stddef.h>

/* Constants */
enum {
    VGPU_MAX_COLOR_ATTACHMENTS = 8u,
    VGPU_MAX_VERTEX_BUFFER_BINDINGS = 8u,
    VGPU_MAX_VERTEX_ATTRIBUTES = 16u,
    VGPU_MAX_VERTEX_ATTRIBUTE_OFFSET = 2047u,
    VGPU_MAX_VERTEX_BUFFER_STRIDE = 2048u,
};

/* Handles */
typedef struct vgpu_texture_t* vgpu_texture;
typedef struct vgpu_buffer_t* vgpu_buffer;
typedef struct vgpu_sampler_t* vgpu_sampler;
typedef struct vgpu_shader_t* vgpu_shader;

typedef uint32_t VGPUFlags;
typedef uint32_t vgpu_bool;

typedef enum vgpu_log_level {
    VGPU_LOG_LEVEL_OFF = 0,
    VGPU_LOG_LEVEL_ERROR = 1,
    VGPU_LOG_LEVEL_WARN = 2,
    VGPU_LOG_LEVEL_INFO = 3,
    VGPU_LOG_LEVEL_DEBUG = 4,
    VGPU_LOG_LEVEL_TRACE = 5,
    _VGPU_LOG_LEVEL_COUNT,
    _VGPU_LOG_LEVEL_FORCE_U32 = 0x7FFFFFFF
} vgpu_log_level;

typedef enum vgpu_backend_type {
    VGPU_BACKEND_TYPE_VULKAN,
    VGPU_BACKEND_TYPE_DIRECT3D12,
    VGPU_BACKEND_TYPE_DIRECT3D11,
    VGPU_BACKEND_TYPE_OPENGL,
    VGPU_BACKEND_TYPE_COUNT,
    _VGPU_BACKEND_TYPE_FORCE_U32 = 0x7FFFFFFF
} vgpu_backend_type;

typedef enum VGPUPresentMode {
    VGPUPresentMode_Fifo = 0,
    VGPUPresentMode_Mailbox = 1,
    VGPUPresentMode_Immediate = 2,
    VGPUPresentMode_Force32 = 0x7FFFFFFF
} VGPUPresentMode;

/// Defines pixel format.
typedef enum VGPUTextureFormat {
    VGPUTextureFormat_Undefined = 0,
    // 8-bit pixel formats
    VGPUTextureFormat_R8Unorm,
    VGPUTextureFormat_R8Snorm,
    VGPUTextureFormat_R8Uint,
    VGPUTextureFormat_R8Sint,

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
    VGPUTextureFormat_RG11B10Float,

    // 64-Bit Pixel Formats
    VGPUTextureFormat_RG32Uint,
    VGPUTextureFormat_RG32Sint,
    VGPUTextureFormat_RG32Float,
    VGPUTextureFormat_RGBA16Uint,
    VGPUTextureFormat_RGBA16Sint,
    VGPUTextureFormat_RGBA16Float,

    // 128-Bit Pixel Formats
    VGPUTextureFormat_RGBA32Uint,
    VGPUTextureFormat_RGBA32Sint,
    VGPUTextureFormat_RGBA32Float,

    // Depth-stencil
    VGPUTextureFormat_Depth16Unorm,
    VGPUTextureFormat_Depth32Float,
    VGPUTextureFormat_Depth24Plus,
    VGPUTextureFormat_Depth24PlusStencil8,

    // Compressed formats
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
    VGPUTextureFormat_BC6HRGBSfloat,
    VGPUTextureFormat_BC7RGBAUnorm,
    VGPUTextureFormat_BC7RGBAUnormSrgb,

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
    /// Unsigned normalized SRGB formats
    VGPUTextureFormatType_UnormSrgb,
    /// Signed normalized formats.
    VGPUTextureFormatType_Snorm,
    /// Unsigned integer formats.
    VGPUTextureFormatType_Uint,
    /// Signed integer formats.
    VGPUTextureFormatType_Sint,

    VGPUTextureFormatType_Force32 = 0x7FFFFFFF
} VGPUTextureFormatType;

typedef enum VGPUTextureDimension {
    VGPUTextureDimension_2D = 0,
    VGPUTextureDimension_3D,
    VGPUTextureDimension_CUBE,
    VGPUTextureDimension_Force32 = 0x7FFFFFFF
} VGPUTextureDimension;

typedef enum VGPUTextureUsage {
    VGPUTextureUsage_None = 0x00000000,
    VGPUTextureUsage_Sampled = 0x04,
    VGPUTextureUsage_Storage = 0x08,
    VGPUTextureUsage_OutputAttachment = 0x10,
    VGPUTextureUsage_Force32 = 0x7FFFFFFF
} VGPUTextureUsage;
typedef VGPUFlags VGPUTextureUsageFlags;

typedef enum VGPUShaderStage {
    VGPUShaderStage_None = 0x00000000,
    VGPUShaderStage_Vertex = 0x00000001,
    VGPUShaderStage_Hull = 0x00000002,
    VGPUShaderStage_Domain = 0x00000004,
    VGPUShaderStage_Geometry = 0x00000008,
    VGPUShaderStage_Fragment = 0x00000010,
    VGPUShaderStage_Compute = 0x00000020,
    VGPUShaderStage_Force32 = 0x7FFFFFFF
} VGPUShaderStage;
typedef uint32_t VGPUShaderStageFlags;

typedef enum VGPUVertexFormat {
    VGPUVertexFormat_Invalid = 0,
    VGPUVertexFormat_UChar2,
    VGPUVertexFormat_UChar4,
    VGPUVertexFormat_Char2,
    VGPUVertexFormat_Char4,
    VGPUVertexFormat_UChar2Norm,
    VGPUVertexFormat_UChar4Norm,
    VGPUVertexFormat_Char2Norm,
    VGPUVertexFormat_Char4Norm,
    VGPUVertexFormat_UShort2,
    VGPUVertexFormat_UShort4,
    VGPUVertexFormat_Short2,
    VGPUVertexFormat_Short4,
    VGPUVertexFormat_UShort2Norm,
    VGPUVertexFormat_UShort4Norm,
    VGPUVertexFormat_Short2Norm,
    VGPUVertexFormat_Short4Norm,
    VGPUVertexFormat_Half2,
    VGPUVertexFormat_Half4,
    VGPUVertexFormat_Float,
    VGPUVertexFormat_Float2,
    VGPUVertexFormat_Float3,
    VGPUVertexFormat_Float4,
    VGPUVertexFormat_UInt,
    VGPUVertexFormat_UInt2,
    VGPUVertexFormat_UInt3,
    VGPUVertexFormat_UInt4,
    VGPUVertexFormat_Int,
    VGPUVertexFormat_Int2,
    VGPUVertexFormat_Int3,
    VGPUVertexFormat_Int4,
    VGPUVertexFormat_Force32
} VGPUVertexFormat;

typedef enum VGPUInputStepMode {
    VGPUInputStepMode_Vertex = 0,
    VGPUInputStepMode_Instance = 1,
    VGPUInputStepMode_Force32 = 0x7FFFFFFF
} VGPUInputStepMode;

typedef enum VGPUPrimitiveTopology {
    VGPUPrimitiveTopology_PointList = 0x00000000,
    VGPUPrimitiveTopology_LineList = 0x00000001,
    VGPUPrimitiveTopology_LineStrip = 0x00000002,
    VGPUPrimitiveTopology_TriangleList = 0x00000003,
    VGPUPrimitiveTopology_TriangleStrip = 0x00000004,
    VGPUPrimitiveTopology_Force32 = 0x7FFFFFFF
} VGPUPrimitiveTopology;

typedef enum VGPUIndexType {
    VGPUIndexType_Uint16 = 0,
    VGPUIndexType_Uint32 = 1,
    VGPUIndexType_Force32 = 0x7FFFFFFF
} VGPUIndexType;

typedef enum {
    VGPU_COMPARE_FUNCTION_UNDEFINED = 0,
    VGPU_COMPARE_FUNCTION_NEVER = 1,
    VGPU_COMPARE_FUNCTION_LESS = 2,
    VGPU_COMPARE_FUNCTION_LESS_EQUAL = 3,
    VGPU_COMPARE_FUNCTION_GREATER = 4,
    VGPU_COMPARE_FUNCTION_GREATER_EQUAL = 5,
    VGPU_COMPARE_FUNCTION_EQUAL = 6,
    VGPU_COMPARE_FUNCTION_NOT_EQUAL = 7,
    VGPU_COMPARE_FUNCTION_ALWAYS = 8,
    _VGPU_COMPARE_FUNCTION_FORCE_U32 = 0x7FFFFFFF
} vgpu_compare_function;

typedef enum VGPULoadOp {
    VGPULoadOp_DontCare = 0,
    VGPULoadOp_Load = 1,
    VGPULoadOp_Clear = 2,
    VGPULoadOp_Force32 = 0x7FFFFFFF
} VGPULoadOp;

typedef enum VGPUTextureLayout {
    VGPUTextureLayout_Undefined = 0,
    VGPUTextureLayout_General,
    VGPUTextureLayout_RenderTarget,
    VGPUTextureLayout_ShaderRead,
    VGPUTextureLayout_ShaderWrite,
    VGPUTextureLayout_Present,
    VGPUTextureLayout_Force32 = 0x7FFFFFFF
} VGPUTextureLayout;

/* Callbacks */
typedef void(*vgpu_log_callback)(void* user_data, vgpu_log_level level, const char* message);

/* Structs */
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

typedef struct vgpu_rect {
    int32_t     x;
    int32_t     y;
    int32_t    width;
    int32_t    height;
} vgpu_rect;

typedef struct vgpu_viewport {
    float       x;
    float       y;
    float       width;
    float       height;
    float       minDepth;
    float       maxDepth;
} vgpu_viewport;

typedef struct vgpu_features {
    vgpu_bool independentBlend;
    vgpu_bool computeShader;
    vgpu_bool geometryShader;
    vgpu_bool tessellationShader;
    vgpu_bool multiViewport;
    vgpu_bool indexUint32;
    vgpu_bool multiDrawIndirect;
    vgpu_bool fillModeNonSolid;
    vgpu_bool samplerAnisotropy;
    vgpu_bool textureCompressionETC2;
    vgpu_bool textureCompressionASTC_LDR;
    vgpu_bool textureCompressionBC;
    vgpu_bool textureCubeArray;
    vgpu_bool raytracing;
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
    vgpu_features features;
    vgpu_limits limits;
} vgpu_caps;

typedef struct VGPURenderPassColorAttachment {
    vgpu_texture     texture;
    uint32_t        mipLevel;
    uint32_t        slice;
    VGPULoadOp      loadOp;
    VGPUColor       clearColor;
} VGPURenderPassColorAttachment;

typedef struct VGPURenderPassDepthStencilAttachment {
    vgpu_texture     texture;
    VGPULoadOp      depthLoadOp;
    float           clearDepth;
    VGPULoadOp      stencilLoadOp;
    uint32_t        clearStencil;
} VGPURenderPassDepthStencilAttachment;

typedef struct VGPURenderPassDescriptor {
    const char* label;
    VGPURenderPassColorAttachment           colorAttachments[VGPU_MAX_COLOR_ATTACHMENTS];
    VGPURenderPassDepthStencilAttachment    depthStencilAttachment;
} VGPURenderPassDescriptor;

typedef struct VgpuVertexBufferLayoutDescriptor {
    uint32_t                stride;
    VGPUInputStepMode       stepMode;
} VgpuVertexBufferLayoutDescriptor;

typedef struct VgpuVertexAttributeDescriptor {
    VGPUVertexFormat            format;
    uint32_t                    offset;
    uint32_t                    bufferIndex;
} VgpuVertexAttributeDescriptor;

typedef struct VgpuVertexDescriptor {
    VgpuVertexBufferLayoutDescriptor    layouts[VGPU_MAX_VERTEX_BUFFER_BINDINGS];
    VgpuVertexAttributeDescriptor       attributes[VGPU_MAX_VERTEX_ATTRIBUTES];
} VgpuVertexDescriptor;

typedef struct VgpuRenderPipelineDescriptor {
    //VgpuShader                  shader;
    VgpuVertexDescriptor        vertexDescriptor;
    VGPUPrimitiveTopology       primitiveTopology;
} VgpuRenderPipelineDescriptor;

typedef struct VgpuComputePipelineDescriptor {
    uint32_t dummy;
    //VgpuShader                  shader;
} VgpuComputePipelineDescriptor;

typedef struct vgpu_swapchain_info {
    const char* label;
    /// Native window handle (HWND, IUnknown, ANativeWindow, NSWindow).
    uintptr_t           handle;

    VGPUTextureFormat   format;
    uint32_t            width;
    uint32_t            height;
    VGPUPresentMode     presentMode;
} vgpu_swapchain_info;

typedef struct vgpu_config {
    vgpu_backend_type type;
    vgpu_bool debug;
    void (*callback)(void* context, vgpu_log_level level, const char* message);
    void* context;
    void* (*get_proc_address)(const char*);
    const vgpu_swapchain_info* swapchain_info;
} vgpu_config;

/* Log functions */
VGPU_API void vgpu_log(vgpu_log_level level, const char* format, ...);

VGPU_API vgpu_backend_type vgpuGetDefaultPlatformBackend(void);
VGPU_API vgpu_bool vgpuIsBackendSupported(vgpu_backend_type backend);
VGPU_API vgpu_bool vgpu_init(const vgpu_config* config);
VGPU_API void vgpu_shutdown(void);
VGPU_API vgpu_backend_type vgpu_get_backend(void);
VGPU_API vgpu_caps vgpu_get_caps(void);
VGPU_API VGPUTextureFormat vgpuGetDefaultDepthFormat(void);
VGPU_API VGPUTextureFormat vgpuGetDefaultDepthStencilFormat(void);

VGPU_API void vgpu_frame_begin(void);
VGPU_API void vgpu_frame_end(void);

/* Texture */
typedef struct {
    const char* label;
    VGPUTextureUsageFlags usage;
    VGPUTextureDimension dimension;
    VGPUExtent3D size;
    VGPUTextureFormat format;
    uint32_t mipLevelCount;
    uint32_t sampleCount;
    /// Initial content to initialize with.
    const void* data;
    /// Pointer to external texture handle
    const void* external_handle;
} VGPUTextureDescriptor;

VGPU_API vgpu_texture vgpuCreateTexture(const VGPUTextureDescriptor* descriptor);
VGPU_API void vgpuDestroyTexture(vgpu_texture texture);

/* Buffer */
typedef enum {
    VGPU_BUFFER_USAGE_NONE = 0,
    VGPU_BUFFER_USAGE_VERTEX = 1,
    VGPU_BUFFER_USAGE_INDEX = 2,
    VGPU_BUFFER_USAGE_UNIFORM = 4,
    VGPU_BUFFER_USAGE_STORAGE = 8,
    VGPU_BUFFER_USAGE_INDIRECT = 16,
    VGPU_BUFFER_USAGE_DYNAMIC = 32,
    VGPU_BUFFER_USAGE_CPU_ACCESSIBLE = 64,
    _VGPU_BUFFER_USAGE_FORCE_U32 = 0x7FFFFFFF
} vgpu_buffer_usage;

typedef struct {
    VGPUFlags usage;
    uint64_t size;
    const void* data;
    const char* label;
} vgpu_buffer_info;

VGPU_API vgpu_buffer vgpu_create_buffer(const vgpu_buffer_info* info);
VGPU_API void vgpu_destroy_buffer(vgpu_buffer buffer);

/* Sampler */
typedef enum {
    VGPU_FILTER_NEAREST = 0x00000000,
    VGPU_FILTER_LINEAR = 0x00000001,
    _VGPU_FILTER_FORCE_U32 = 0x7FFFFFFF
} vgpu_filter;

typedef enum {
    VGPU_ADDRESS_MODE_CLAMP_TO_EDGE = 0,
    VGPU_ADDRESS_MODE_REPEAT = 1,
    VGPU_ADDRESS_MODE_MIRROR_REPEAT = 2,
    _VGPU_ADDRESS_MODE_FORCE_U32 = 0x7FFFFFFF
} vgpu_address_mode;

typedef struct {
    vgpu_address_mode       mode_u;
    vgpu_address_mode       mode_v;
    vgpu_address_mode       mode_w;
    vgpu_filter             mag_filter;
    vgpu_filter             min_filter;
    vgpu_filter             mipmap_filter;
    float                   lodMinClamp;
    float                   lodMaxClamp;
    vgpu_compare_function   compare;
    //uint32_t                max_anisotropy;
    const char* label;
} vgpu_sampler_info;

VGPU_API vgpu_sampler vgpu_create_sampler(const vgpu_sampler_info* info);
VGPU_API void vgpu_destroy_sampler(vgpu_sampler sampler);

/* Shader */
typedef struct {
    const void* code;
    uint64_t size;
    const char* source;
    const char* entry;
} vgpu_shader_source;

typedef struct {
    vgpu_shader_source vertex;
    vgpu_shader_source fragment;
    vgpu_shader_source compute;
    const char* label;
} vgpu_shader_info;

VGPU_API vgpu_shader vgpu_create_shader(const vgpu_shader_info* info);
VGPU_API void vgpu_destroy_shader(vgpu_shader shader);

/* Pipeline */
typedef struct {
    vgpu_bool depth_write_enabled;
    vgpu_compare_function depth_compare;
} vgpu_depth_stencil_state;

//VGPU_EXPORT VgpuPipeline vgpuCreateRenderPipeline(const VgpuRenderPipelineDescriptor* descriptor);
//VGPU_EXPORT VgpuPipeline vgpuCreateComputePipeline(const VgpuComputePipelineDescriptor* descriptor);
//VGPU_EXPORT void vgpuDestroyPipeline(VgpuPipeline pipeline);

/* Commands */
VGPU_API void vgpuCmdBeginRenderPass(const VGPURenderPassDescriptor* descriptor);
VGPU_API void vgpuCmdEndRenderPass(void);
/*VGPU_EXPORT void vgpuCmdSetShader(VgpuCommandBuffer commandBuffer, VgpuShader shader);
VGPU_EXPORT void vgpuCmdSetVertexBuffer(VgpuCommandBuffer commandBuffer, uint32_t binding, VgpuBuffer buffer, uint64_t offset, VgpuVertexInputRate inputRate);
VGPU_EXPORT void vgpuCmdSetIndexBuffer(VgpuCommandBuffer commandBuffer, VgpuBuffer buffer, uint64_t offset, VgpuIndexType indexType);

VGPU_EXPORT void vgpuCmdSetViewport(VgpuCommandBuffer commandBuffer, VgpuViewport viewport);
VGPU_EXPORT void vgpuCmdSetViewports(VgpuCommandBuffer commandBuffer, uint32_t viewportCount, const VgpuViewport* pViewports);
VGPU_EXPORT void vgpuCmdSetScissor(VgpuCommandBuffer commandBuffer, VgpuRect2D scissor);
VGPU_EXPORT void vgpuCmdSetScissors(VgpuCommandBuffer commandBuffer, uint32_t scissorCount, const VgpuRect2D* pScissors);

VGPU_EXPORT void vgpuCmdSetPrimitiveTopology(VgpuCommandBuffer commandBuffer, VgpuPrimitiveTopology topology);
VGPU_EXPORT void vgpuCmdDraw(VgpuCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t firstVertex);
VGPU_EXPORT void vgpuCmdDrawIndexed(VgpuCommandBuffer commandBuffer, uint32_t indexCount, uint32_t firstIndex, int32_t vertexOffset);
*/

/* Helper methods */
/// Get the number of bits per format
VGPU_API uint32_t vgpu_get_format_bits_per_pixel(VGPUTextureFormat format);
VGPU_API uint32_t vgpu_get_format_block_size(VGPUTextureFormat format);

/// Get the format compression ration along the x-axis.
VGPU_API uint32_t vgpuGetFormatBlockWidth(VGPUTextureFormat format);
/// Get the format compression ration along the y-axis.
VGPU_API uint32_t vgpuGetFormatBlockHeight(VGPUTextureFormat format);
/// Get the format Type.
VGPU_API VGPUTextureFormatType vgpuGetFormatType(VGPUTextureFormat format);

/// Check if the format has a depth component.
VGPU_API vgpu_bool vgpu_is_depth_format(VGPUTextureFormat format);
/// Check if the format has a stencil component.
VGPU_API vgpu_bool vgpu_is_stencil_format(VGPUTextureFormat format);
/// Check if the format has depth or stencil components.
VGPU_API vgpu_bool vgpu_is_depth_stencil_format(VGPUTextureFormat format);
/// Check if the format is a compressed format.
VGPU_API vgpu_bool vgpu_is_compressed_format(VGPUTextureFormat format);
/// Get format string name.
VGPU_API const char* vgpu_get_format_name(VGPUTextureFormat format);

#ifdef __cplusplus
namespace vgpu
{
    enum class TextureUsage : uint32_t {
        None = VGPUTextureUsage_None,
        Sampled = VGPUTextureUsage_Sampled,
        Storage = VGPUTextureUsage_Storage,
        OutputAttachment = VGPUTextureUsage_OutputAttachment
    };
}
#endif

#endif // VGPU_H_
