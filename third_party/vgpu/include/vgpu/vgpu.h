//
// Copyright (c) 2019 Amer Koleci.
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
#           define VGPU_EXPORT __declspec(dllexport)
#       else
#           define VGPU_EXPORT __declspec(dllimport)
#       endif
#   else  // defined(_WIN32)
#       if defined(VGPU_IMPLEMENTATION)
#           define VGPU_EXPORT __attribute__((visibility("default")))
#       else
#           define VGPU_EXPORT
#       endif
#   endif  // defined(_WIN32)
#else       // defined(VGPU_SHARED_LIBRARY)
#   define VGPU_EXPORT
#endif  // defined(VGPU_SHARED_LIBRARY)

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct VGPUBufferImpl* VGPUBuffer;
typedef struct VGPUTextureImpl* VGPUTexture;
//typedef struct VGPUTextureViewImpl* VGPUTextureView;
typedef struct vgpu_sampler_t* vgpu_sampler;
typedef struct VGPURenderPassImpl* VGPURenderPass;

enum {
    VGPU_MAX_COLOR_ATTACHMENTS = 8u,
    VGPU_MAX_VERTEX_BUFFER_BINDINGS = 8u,
    VGPU_MAX_VERTEX_ATTRIBUTES = 16u,
    VGPU_MAX_VERTEX_ATTRIBUTE_OFFSET = 2047u,
    VGPU_MAX_VERTEX_BUFFER_STRIDE = 2048u,
};

typedef enum vgpu_log_level {
    VGPU_LOG_LEVEL_OFF = 0,
    VGPU_LOG_LEVEL_VERBOSE,
    VGPU_LOG_LEVEL_DEBUG,
    VGPU_LOG_LEVEL_INFO,
    VGPU_LOG_LEVEL_WARN,
    VGPU_LOG_LEVEL_ERROR,
    VGPU_LOG_LEVEL_CRITICAL,
    VGPU_LOG_LEVEL_COUNT,
    _VGPU_LOG_LEVEL_FORCE_U32 = 0x7FFFFFFF
} vgpu_log_level;

typedef enum vgpu_backend {
    VGPU_BACKEND_DEFAULT,
    VGPU_BACKEND_NULL,
    VGPU_BACKEND_D3D11,
    VGPU_BACKEND_D3D12,
    VGPU_BACKEND_VULKAN,
    VGPU_BACKEND_OPENGL,
    VGPU_BACKEND_OPENGLES,
    VGPU_BACKEND_FORCE_U32 = 0x7FFFFFFF
} vgpu_backend;

typedef enum VGPUAdapterType {
    VGPUAdapterType_DiscreteGPU = 0x00000000,
    VGPUAdapterType_IntegratedGPU = 0x00000001,
    VGPUAdapterType_CPU = 0x00000002,
    VGPUAdapterType_Unknown = 0x00000003,
    VGPUAdapterType_Force32 = 0x7FFFFFFF
} VGPUAdapterType;

typedef enum VGPUPresentMode {
    VGPUPresentMode_Fifo = 0,
    VGPUPresentMode_Mailbox = 1,
    VGPUPresentMode_Immediate = 2,
    VGPUPresentMode_Force32 = 0x7FFFFFFF
} VGPUPresentMode;

/// Defines pixel format.
typedef enum vgpu_pixel_format {
    VGPU_PIXELFORMAT_UNDEFINED = 0,
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
    VGPU_PIXELFORMAT_DEPTH16_UNORM,
    VGPU_PIXELFORMAT_DEPTH32_FLOAT,
    VGPU_PIXELFORMAT_DEPTH24_PLUS,
    VGPU_PIXELFORMAT_DEPTH24_PLUS_STENCIL8,

    // Compressed formats
    VGPUTextureFormat_BC1RGBAUnorm,
    VGPUTextureFormat_BC1RGBAUnormSrgb,
    VGPUTextureFormat_BC2RGBAUnorm ,
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

    VGPU_PIXEL_FORMAT_COUNT,
    VGPU_PIXEL_FORMAT_FORCE_U32 = 0x7FFFFFFF
} vgpu_pixel_format;

/// Defines pixel format type.
typedef enum vgpu_pixel_format_type {
    /// Unknown format Type
    VGPU_PIXEL_FORMAT_TYPE_UNKNOWN = 0,
    /// floating-point formats.
    VGPU_PIXEL_FORMAT_TYPE_FLOAT,
    /// Unsigned normalized formats.
    VGPU_PIXEL_FORMAT_TYPE_UNORM,
    /// Unsigned normalized SRGB formats
    VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,
    /// Signed normalized formats.
    VGPU_PIXEL_FORMAT_TYPE_SNORM,
    /// Unsigned integer formats.
    VGPU_PIXEL_FORMAT_TYPE_UINT,
    /// Signed integer formats.
    VGPU_PIXEL_FORMAT_TYPE_SINT,

    VGPU_PIXEL_FORMAT_TYPE_FORCE_U32 = 0x7FFFFFFF
} vgpu_pixel_format_type;

typedef enum vgpu_sample_count {
    VGPU_SAMPLE_COUNT_1 = 1,
    VGPU_SAMPLE_COUNT_2 = 2,
    VGPU_SAMPLE_COUNT_4 = 4,
    VGPU_SAMPLE_COUNT_8 = 8,
    VGPU_SAMPLE_COUNT_16 = 16,
    VGPU_SAMPLE_COUNT_32 = 32,
} vgpu_sample_count;

typedef enum vgpu_texture_type {
    VGPU_TEXTURE_TYPE_2D = 0,
    VGPU_TEXTURE_TYPE_3D,
    VGPU_TEXTURE_TYPE_CUBE,
    _VGPU_TEXTURE_TYPE_COUNT,
    _VGPU_TEXTURE_TYPE_FORCE_U32 = 0x7FFFFFFF
} vgpu_texture_type;

typedef enum vgpu_texture_usage {
    VGPU_TEXTURE_USAGE_NONE = 0,
    VGPU_TEXTURE_USAGE_SAMPLED = 0x01,
    VGPU_TEXTURE_USAGE_STORAGE = 0x02,
    VGPU_TEXTURE_USAGE_RENDERTARGET = 0x04,
    _VGPU_TEXTURE_USAGE_FORCE_U32 = 0x7FFFFFFF
} vgpu_texture_usage;
typedef uint32_t vgpu_texture_usage_flags;

typedef enum VGPUBufferUsage {
    VGPU_BUFFER_USAGE_NONE = 0,
    VGPU_BUFFER_USAGE_VERTEX = 1,
    VGPU_BUFFER_USAGE_INDEX = 2,
    VGPU_BUFFER_USAGE_UNIFORM = 4,
    VGPU_BUFFER_USAGE_STORAGE = 8,
    VGPU_BUFFER_USAGE_INDIRECT = 16,
    VGPU_BUFFER_USAGE_DYNAMIC = 32,
    VGPU_BUFFER_USAGE_CPU_ACCESSIBLE = 64,
} VGPUBufferUsage;
typedef uint32_t VGPUBufferUsageFlags;

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

typedef enum vgpu_compare_function {
    VGPU_COMPARE_FUNCTION_UNDEFINED = 0,
    VGPU_COMPARE_FUNCTION_NEVER = 1,
    VGPU_COMPARE_FUNCTION_LESS = 2,
    VGPU_COMPARE_FUNCTION_LESS_EQUAL = 3,
    VGPU_COMPARE_FUNCTION_GREATER = 4,
    VGPU_COMPARE_FUNCTION_GREATER_EQUAL = 5,
    VGPU_COMPARE_FUNCTION_EQUAL = 6,
    VGPU_COMPARE_FUNCTION_NOT_EQUAL = 7,
    VGPU_COMPARE_FUNCTION_ALWAYS = 8,
    VGPU_COMPARE_FUNCTION_FORCE_U32 = 0x7FFFFFFF
} vgpu_compare_function;

typedef enum vgpu_filter {
    VGPU_FILTER_NEAREST = 0,
    VGPU_FILTER_LINEAR = 1,
    VGPU_FILTER_FORCE_U32 = 0x7FFFFFFF
} vgpu_filter;

typedef enum vgpu_address_mode {
    VGPU_ADDRESS_MODE_CLAMP_TO_EDGE = 0,
    VGPU_ADDRESS_MODE_REPEAT = 1,
    VGPU_ADDRESS_MODE_MIRROR_REPEAT = 2,
    VGPU_ADDRESS_MODE_CLAMP_TO_BORDER = 3,
    VGPU_ADDRESS_MODE_FORCE_U32 = 0x7FFFFFFF
} vgpu_address_mode;

typedef enum vgpu_border_color {
    VGPU_BORDER_COLOR_TRANSPARENT_BLACK = 0,
    VGPU_BORDER_COLOR_OPAQUE_BLACK = 1,
    VGPU_BORDER_COLOR_OPAQUE_WHITE = 2,
    VGPU_BORDER_COLOR_FORCE_U32 = 0x7FFFFFFF
} vgpu_border_color;

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

typedef struct vgpu_color {
    float r;
    float g;
    float b;
    float a;
} vgpu_color;

typedef struct vgpu_rect {
    int32_t     x;
    int32_t     y;
    int32_t    width;
    int32_t    height;
} vgpu_rect;

typedef struct VgpuViewport {
    float       x;
    float       y;
    float       width;
    float       height;
    float       minDepth;
    float       maxDepth;
} VgpuViewport;

typedef struct VGPUFeatures {
    bool independentBlend;
    bool computeShader;
    bool geometryShader;
    bool tessellationShader;
    bool multiViewport;
    bool indexUint32;
    bool multiDrawIndirect;
    bool fillModeNonSolid;
    bool samplerAnisotropy;
    bool textureCompressionETC2;
    bool textureCompressionASTC_LDR;
    bool textureCompressionBC;
    bool textureCubeArray;
    bool raytracing;
} VGPUFeatures;

typedef struct VGPULimits {
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
} VGPULimits;

typedef struct VGPUBufferDescriptor {
    VGPUBufferUsageFlags usage;
    uint64_t size;
    const void* content;
    const char* label;
} VGPUBufferDescriptor;

typedef struct vgpu_texture_desc {
    vgpu_texture_type type;
    vgpu_texture_usage_flags usage;
    uint32_t width;
    uint32_t height;
    union {
        uint32_t depth;
        uint32_t layers;
    };
    vgpu_pixel_format format;
    uint32_t mip_levels;
    vgpu_sample_count sample_count;
    /// Initial content to initialize with.
    const void* content;
    /// Pointer to external texture handle
    const void* external_handle;
    const char* label;
} vgpu_texture_desc;

typedef struct vgpu_color_attachment {
    VGPUTexture     texture;
    uint32_t        mipLevel;
    uint32_t        slice;
    VGPULoadOp      loadOp;
    vgpu_color      clear_color;
} vgpu_color_attachment;

typedef struct VGPUDepthStencilAttachment {
    VGPUTexture               texture;
    float clearDepth;
    uint32_t clearStencil;
} VGPUDepthStencilAttachment;

typedef struct VGPURenderPassDescriptor {
    vgpu_color_attachment       color_attachments[VGPU_MAX_COLOR_ATTACHMENTS];
    VGPUDepthStencilAttachment  depth_stencil_attachment;
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

typedef struct VGPUShaderModuleDescriptor {
    VGPUShaderStage stage;
    uint64_t        codeSize;
    const uint8_t*  pCode;
    const char*     source;
    const char*     entryPoint;
} VGPUShaderModuleDescriptor;

typedef struct VgpuShaderStageDescriptor {
    //VgpuShaderModule            shaderModule;
    const char* entryPoint;
    /*const VgpuSpecializationInfo* specializationInfo;*/
} VgpuShaderStageDescriptor;

typedef struct VgpuShaderDescriptor {
    uint32_t                            stageCount;
    const VgpuShaderStageDescriptor* stages;
} VgpuShaderDescriptor;

typedef struct VgpuRenderPipelineDescriptor {
    //VgpuShader                  shader;
    VgpuVertexDescriptor        vertexDescriptor;
    VGPUPrimitiveTopology       primitiveTopology;
} VgpuRenderPipelineDescriptor;

typedef struct VgpuComputePipelineDescriptor {
    uint32_t dummy;
    //VgpuShader                  shader;
} VgpuComputePipelineDescriptor;

typedef struct vgpu_sampler_desc {
    vgpu_address_mode address_mode_u;
    vgpu_address_mode address_mode_v;
    vgpu_address_mode address_mode_w;
    vgpu_filter mag_filter;
    vgpu_filter min_filter;
    vgpu_filter mipmap_filter;
    float lod_min_clamp;
    float lod_max_clamp;
    vgpu_compare_function compare;
    uint32_t max_anisotropy;
    vgpu_border_color border_color;
    const char* label;
} vgpu_sampler_desc;

typedef struct vgpu_platform_handle {
    void* display;
    /*
    * Native window handle (HWND, IUnknown, ANativeWindow, NSWindow)..
    * If null headless device will be created if supported by backend.
    */
    void* window_handle;
} vgpu_platform_handle;

typedef struct vgpu_swapchain_desc {
    /// Native window handle (HWND, IUnknown, ANativeWindow, NSWindow).
    vgpu_platform_handle handle;

    uint32_t            width;
    uint32_t            height;
    bool                fullscreen;
    vgpu_pixel_format   colorFormat;
    vgpu_color          clear_color;
    vgpu_pixel_format   depthStencilFormat;
    VGPUPresentMode     presentMode;
    vgpu_sample_count   sample_count;
} vgpu_swapchain_desc;

typedef struct vgpu_config {
    vgpu_backend preferred_backend;
    /// Enable device for debuging.
    bool debug;

    /// Enable device for profiling.
    bool profile;

    /// The main swapchain description or null for headless.
    const vgpu_swapchain_desc* swapchain;
} vgpu_config;

#ifdef __cplusplus
extern "C"
{
#endif

    /* Log functions */
    /// Get the current log output function.
    VGPU_EXPORT void vgpu_get_log_callback_function(vgpu_log_callback* callback, void** user_data);
    /// Set the current log output function.
    VGPU_EXPORT void vgpu_set_log_callback_function(vgpu_log_callback callback, void* user_data);

    VGPU_EXPORT void vgpu_log(vgpu_log_level level, const char* message);
    VGPU_EXPORT void vgpu_log_format(vgpu_log_level level, const char* format, ...);
    VGPU_EXPORT void vgpu_log_error(const char* message);
    VGPU_EXPORT void vgpu_log_error_format(const char* format, ...);

    VGPU_EXPORT vgpu_backend vgpu_get_default_platform_backend(void);
    VGPU_EXPORT bool vgpu_is_backend_supported(vgpu_backend backend);
    VGPU_EXPORT bool vgpu_init(const vgpu_config* config);
    VGPU_EXPORT void vgpu_shutdown(void);
    VGPU_EXPORT vgpu_backend vgpu_query_backend(void);
    VGPU_EXPORT VGPUFeatures vgpu_query_features(void);
    VGPU_EXPORT VGPULimits vgpu_query_limits(void);
    VGPU_EXPORT VGPURenderPass vgpu_get_default_render_pass(void);
    VGPU_EXPORT vgpu_pixel_format vgpu_get_default_depth_format(void);
    VGPU_EXPORT vgpu_pixel_format vgpu_get_default_depth_stencil_format(void);

    VGPU_EXPORT void vgpu_wait_idle(void);
    VGPU_EXPORT void vgpu_begin_frame(void);
    VGPU_EXPORT void vgpu_end_frame(void);
   
    /* Buffer */
    VGPU_EXPORT VGPUBuffer vgpu_create_buffer(const VGPUBufferDescriptor* desc);
    VGPU_EXPORT void vgpu_destroy_buffer(VGPUBuffer buffer);

    /* Texture */
    VGPU_EXPORT VGPUTexture vgpu_create_texture(const vgpu_texture_desc* desc);
    VGPU_EXPORT void vgpu_destroy_texture(VGPUTexture texture);
    VGPU_EXPORT vgpu_texture_desc vgpu_query_texture_desc(VGPUTexture texture);
    VGPU_EXPORT uint32_t vgpu_get_texture_width(VGPUTexture texture, uint32_t mip_level);
    VGPU_EXPORT uint32_t vgpu_get_texture_height(VGPUTexture texture, uint32_t mip_level);

    /* Sampler */
    VGPU_EXPORT vgpu_sampler vgpu_create_sampler(const vgpu_sampler_desc* desc);
    VGPU_EXPORT void vgpu_destroy_sampler(vgpu_sampler sampler);

    /* RenderPass */
    VGPU_EXPORT VGPURenderPass vgpuCreateRenderPass(const VGPURenderPassDescriptor* descriptor);
    //VGPU_EXPORT VGPURenderPass vgpuRenderPassCreate(const VGPUSwapChainDescriptor* descriptor);
    VGPU_EXPORT void vgpuDestroyRenderPass(VGPURenderPass renderPass);
    VGPU_EXPORT void vgpuRenderPassGetExtent(VGPURenderPass renderPass, uint32_t* width, uint32_t* height);
    VGPU_EXPORT void vgpu_render_pass_set_color_clear_value(VGPURenderPass render_pass, uint32_t attachment_index, const float colorRGBA[4]);
    VGPU_EXPORT void vgpu_render_pass_set_depth_stencil_clear_value(VGPURenderPass render_pass, float depth, uint8_t stencil);

    /* ShaderModule */
    //VGPU_EXPORT VgpuShaderModule vgpuCreateShaderModule(const VgpuShaderModuleDescriptor* descriptor);
    //VGPU_EXPORT void vgpuDestroyShaderModule(VgpuShaderModule shaderModule);

    /* Shader */
    //VGPU_EXPORT VgpuShader vgpuCreateShader(const VgpuShaderDescriptor* descriptor);
    //VGPU_EXPORT void vgpuDestroyShader(VgpuShader shader);

    /* Pipeline */
    //VGPU_EXPORT VgpuPipeline vgpuCreateRenderPipeline(const VgpuRenderPipelineDescriptor* descriptor);
    //VGPU_EXPORT VgpuPipeline vgpuCreateComputePipeline(const VgpuComputePipelineDescriptor* descriptor);
    //VGPU_EXPORT void vgpuDestroyPipeline(VgpuPipeline pipeline);

    /* Commands */
    VGPU_EXPORT void vgpu_cmd_begin_render_pass(VGPURenderPass renderPass);
    VGPU_EXPORT void vgpu_cmd_end_render_pass(void);
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
    VGPU_EXPORT uint32_t vgpu_get_format_bits_per_pixel(vgpu_pixel_format format);
    VGPU_EXPORT uint32_t vgpu_get_format_block_size(vgpu_pixel_format format);

    /// Get the format compression ration along the x-axis.
    VGPU_EXPORT uint32_t vgpuGetFormatBlockWidth(vgpu_pixel_format format);
    /// Get the format compression ration along the y-axis.
    VGPU_EXPORT uint32_t vgpuGetFormatBlockHeight(vgpu_pixel_format format);
    /// Get the format Type.
    VGPU_EXPORT vgpu_pixel_format_type vgpu_get_format_type(vgpu_pixel_format format);

    /// Check if the format has a depth component.
    VGPU_EXPORT bool vgpu_is_depth_format(vgpu_pixel_format format);
    /// Check if the format has a stencil component.
    VGPU_EXPORT bool vgpu_is_stencil_format(vgpu_pixel_format format);
    /// Check if the format has depth or stencil components.
    VGPU_EXPORT bool vgpu_is_depth_stencil_format(vgpu_pixel_format format);
    /// Check if the format is a compressed format.
    VGPU_EXPORT bool vgpu_is_compressed_format(vgpu_pixel_format format);
    /// Get format string name.
    VGPU_EXPORT const char* vgpu_get_format_name(vgpu_pixel_format format);

#ifdef __cplusplus
}
#endif

#endif // VGPU_H_
