//
// Copyright (c) 2020 Amer Koleci.
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

typedef uint32_t AGPUFlags;

typedef struct GPUDeviceImpl* GPUDevice;
typedef struct AGPUBufferImpl* AGPUBuffer;
typedef struct AGPUTextureImpl* AGPUTexture;
typedef struct AGPUTextureViewImpl* AGPUTextureView;
typedef struct GPUSamplerImpl* GPUSampler;
typedef struct VGPURenderPassImpl* VGPURenderPass;
typedef struct vgpu_shader_t* vgpu_shader;
typedef struct vgpu_pipeline_t* vgpu_pipeline;

enum {
    AGPU_INVALID_ID = 0xFFFFFFFF,
    AGPU_NUM_INFLIGHT_FRAMES = 2u,
    AGPU_MAX_COLOR_ATTACHMENTS = 8u,
    AGPU_MAX_DEVICE_NAME_SIZE = 256,
    AGPU_MAX_VERTEX_BUFFER_BINDINGS = 8u,
    AGPU_MAX_VERTEX_ATTRIBUTES = 16u,
    AGPU_MAX_VERTEX_ATTRIBUTE_OFFSET = 2047u,
    AGPU_MAX_VERTEX_BUFFER_STRIDE = 2048u,
};

typedef enum GPULogLevel {
    GPULogLevel_Off = 0,
    GPULogLevel_Error = 1,
    GPULogLevel_Warn = 2,
    GPULogLevel_Info = 3,
    GPULogLevel_Debug = 4,
    GPULogLevel_Trace = 5,
    _AGPU_LOG_LEVEL_COUNT,
    _AGPU_LOG_LEVEL_FORCE_U32 = 0x7FFFFFFF
} GPULogLevel;

typedef enum agpu_backend_type {
    AGPU_BACKEND_TYPE_DEFAULT = 0,
    AGPU_BACKEND_TYPE_NULL = 1,
    AGPU_BACKEND_TYPE_VULKAN = 2,
    AGPU_BACKEND_TYPE_D3D12 = 3,
    AGPU_BACKEND_TYPE_D3D11 = 4,
    AGPU_BACKEND_TYPE_OPENGL = 5,
    _AGPU_BACKEND_TYPE_FORCE_U32 = 0x7FFFFFFF
} agpu_backend_type;

/// Defines pixel format.
typedef enum AGPUPixelFormat {
    GPU_TEXTURE_FORMAT_UNDEFINED = 0,
    // 8-bit pixel formats
    GPU_TEXTURE_FORMAT_R8_UNORM,
    GPU_TEXTURE_FORMAT_R8_SNORM,
    GPU_TEXTURE_FORMAT_R8_UINT,
    GPU_TEXTURE_FORMAT_R8_SINT,

    // 16-bit pixel formats
    GPU_TEXTURE_FORMAT_R16_UINT,
    GPU_TEXTURE_FORMAT_R16_SINT,
    GPU_TEXTURE_FORMAT_R16_FLOAT,
    GPU_TEXTURE_FORMAT_RG8_UNORM,
    GPU_TEXTURE_FORMAT_RG8_SNORM,
    GPU_TEXTURE_FORMAT_RG8_UINT,
    GPU_TEXTURE_FORMAT_RG8_SINT,

    // 32-bit pixel formats
    GPU_TEXTURE_FORMAT_R32_UINT,
    GPU_TEXTURE_FORMAT_R32_SINT,
    GPU_TEXTURE_FORMAT_R32_FLOAT,
    GPU_TEXTURE_FORMAT_RG16_UINT,
    GPU_TEXTURE_FORMAT_RG16_SINT,
    GPU_TEXTURE_FORMAT_RG16_FLOAT,

    GPUTextureFormat_RGBA8Unorm,
    GPUTextureFormat_RGBA8UnormSrgb,
    GPU_TEXTURE_FORMAT_RGBA8_SNORM,
    GPU_TEXTURE_FORMAT_RGBA8_UINT,
    GPU_TEXTURE_FORMAT_RGBA8_SINT,
    GPUTextureFormat_BGRA8Unorm,
    GPUTextureFormat_BGRA8UnormSrgb,

    // Packed 32-Bit Pixel formats
    GPU_TEXTURE_FORMAT_RGB10A2_UNORM,
    GPU_TEXTURE_FORMAT_RG11B10_FLOAT,

    // 64-Bit Pixel Formats
    GPU_TEXTURE_FORMAT_RG32_UINT,
    GPU_TEXTURE_FORMAT_RG32_SINT,
    GPU_TEXTURE_FORMAT_RG32_FLOAT,
    GPU_TEXTURE_FORMAT_RGBA16_UINT,
    GPU_TEXTURE_FORMAT_RGBA16_SINT,
    GPU_TEXTURE_FORMAT_RGBA16_FLOAT,

    // 128-Bit Pixel Formats
    GPU_TEXTURE_FORMAT_RGBA32_UINT,
    GPU_TEXTURE_FORMAT_RGBA32_SINT,
    GPU_TEXTURE_FORMAT_RGBA32_FLOAT,

    // Depth-stencil
    GPU_TEXTURE_FORMAT_DEPTH16_UNORM,
    GPU_TEXTURE_FORMAT_DEPTH32_FLOAT,
    GPU_TEXTURE_FORMAT_DEPTH24_PLUS,
    GPU_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8,

    // Compressed formats
    GPU_TEXTURE_FORMAT_BC1RGBA_UNORM,
    GPU_TEXTURE_FORMAT_BC1RGBA_UNORM_SRGB,
    GPU_TEXTURE_FORMAT_BC2RGBA_UNORM,
    GPU_TEXTURE_FORMAT_BC2RGBA_UNORM_SRGB,
    GPU_TEXTURE_FORMAT_BC3RGBA_UNORM,
    GPU_TEXTURE_FORMAT_BC3RGBA_UNORM_SRGB,
    GPU_TEXTURE_FORMAT_BC4R_UNORM,
    GPU_TEXTURE_FORMAT_BC4R_SNORM,
    GPU_TEXTURE_FORMAT_BC5RG_UNORM,
    GPU_TEXTURE_FORMAT_BC5RG_SNORM,
    GPU_TEXTURE_FORMAT_BC6HRGB_UFLOAT,
    GPU_TEXTURE_FORMAT_BC6HRGB_SFLOAT,
    GPU_TEXTURE_FORMAT_BC7RGBA_UNORM,
    GPU_TEXTURE_FORMAT_BC7RGBA_UNORM_SRGB,

    _GPUTextureFormat_Count,
    _GPUTextureFormat_Force32 = 0x7FFFFFFF
} AGPUPixelFormat;

/// Defines pixel format type.
typedef enum AGPUPixelFormatType {
    /// Unknown format Type
    AGPUPixelFormatType_Unknown = 0,
    /// floating-point formats.
    AGPUPixelFormatType_Float,
    /// Unsigned normalized formats.
    AGPUPixelFormatType_Unorm,
    /// Unsigned normalized SRGB formats
    AGPUPixelFormatType_UnormSrgb,
    /// Signed normalized formats.
    AGPUPixelFormatType_Snorm,
    /// Unsigned integer formats.
    AGPUPixelFormatType_Uint,
    /// Signed integer formats.
    AGPUPixelFormatType_Sint,

    AGPUPixelFormatType_Force32 = 0x7FFFFFFF
} AGPUPixelFormatType;

typedef enum AGPUTextureType {
    AGPUTextureType_2D = 0,
    AGPUTextureType_3D,
    AGPUTextureType_Cube,
    AGPUTextureType_Force32 = 0x7FFFFFFF
} AGPUTextureType;

typedef enum AGPUTextureUsage {
    AGPUTextureUsage_None = 0,
    AGPUTextureUsage_Sampled = (1 << 0),
    AGPUTextureUsage_Storage = (1 << 1),
    AGPUTextureUsage_OutputAttachment = (1 << 2),
    AGPUTextureUsage_Force32 = 0x7FFFFFFF
} AGPUTextureUsage;
typedef AGPUFlags AGPUTextureUsageFlags;

typedef enum gpu_buffer_usage {
    GPU_BUFFER_USAGE_NONE = 0,
    GPU_BUFFER_USAGE_VERTEX = (1 << 0),
    GPU_BUFFER_USAGE_INDEX = (1 << 1),
    GPU_BUFFER_USAGE_UNIFORM = (1 << 2),
    GPU_BUFFER_USAGE_STORAGE = (1 << 3),
    GPU_BUFFER_USAGE_INDIRECT = (1 << 4),
    GPU_BUFFER_USAGE_DYNAMIC = (1 << 5),
    GPU_BUFFER_USAGE_STAGING = (1 << 6)
} gpu_buffer_usage;
typedef uint32_t gpu_buffer_usage_flags;

typedef enum vgpu_shader_stage {
    VGPU_SHADER_STAGE_NONE = 0,
    VGPU_SHADER_STAGE_VERTEX = 0x01,
    VGPU_SHADER_STAGE_HULL = 0x02,
    VGPU_SHADER_STAGE_DOMAIN = 0x04,
    VGPU_SHADER_STAGE_GEOMETRY = 0x08,
    VGPU_SHADER_STAGE_FRAGMENT = 0x10,
    VGPU_SHADER_STAGE_COMPUTE = 0x20,
    _VGPU_SHADER_STAGE_FORCE_U32 = 0x7FFFFFFF
} vgpu_shader_stage;
typedef uint32_t vgpu_shader_stage_flags;

typedef enum AGPUVertexFormat {
    AGPUVertexFormat_Invalid = 0,
    AGPUVertexFormat_UChar2,
    AGPUVertexFormat_UChar4,
    AGPUVertexFormat_Char2,
    AGPUVertexFormat_Char4,
    AGPUVertexFormat_UChar2Norm,
    AGPUVertexFormat_UChar4Norm,
    AGPUVertexFormat_Char2Norm,
    AGPUVertexFormat_Char4Norm,
    AGPUVertexFormat_UShort2,
    AGPUVertexFormat_UShort4,
    AGPUVertexFormat_Short2,
    AGPUVertexFormat_Short4,
    AGPUVertexFormat_UShort2Norm,
    AGPUVertexFormat_UShort4Norm,
    AGPUVertexFormat_Short2Norm,
    AGPUVertexFormat_Short4Norm,
    AGPUVertexFormat_Half2,
    AGPUVertexFormat_Half4,
    AGPUVertexFormat_Float,
    AGPUVertexFormat_Float2,
    AGPUVertexFormat_Float3,
    AGPUVertexFormat_Float4,
    AGPUVertexFormat_UInt,
    AGPUVertexFormat_UInt2,
    AGPUVertexFormat_UInt3,
    AGPUVertexFormat_UInt4,
    AGPUVertexFormat_Int,
    AGPUVertexFormat_Int2,
    AGPUVertexFormat_Int3,
    AGPUVertexFormat_Int4,
    AGPUVertexFormat_Force32 = 0x7FFFFFFF
} AGPUVertexFormat;

typedef enum GPUInputStepMode {
    GPUInputStepMode_Vertex = 0,
    GPUInputStepMode_Instance = 1,
    _GPUInputStepMode_Force32 = 0x7FFFFFFF
} GPUInputStepMode;

typedef enum AGPUPrimitiveTopology {
    AGPUPrimitiveTopology_PointList = 0x00000000,
    AGPUPrimitiveTopology_LineList = 0x00000001,
    AGPUPrimitiveTopology_LineStrip = 0x00000002,
    AGPUPrimitiveTopology_TriangleList = 0x00000003,
    AGPUPrimitiveTopology_TriangleStrip = 0x00000004,
    AGPUPrimitiveTopology_Force32 = 0x7FFFFFFF
} AGPUPrimitiveTopology;

typedef enum AGPUIndexFormat {
    AGPUIndexFormat_Uint16 = 0,
    AGPUIndexFormat_Uint32 = 1,
    AGPUIndexFormat_Force32 = 0x7FFFFFFF
} AGPUIndexFormat;

typedef enum GPUCompareFunction {
    GPUCompareFunction_Undefined = 0,
    GPUCompareFunction_Never = 1,
    GPUCompareFunction_Less = 2,
    GPUCompareFunction_LessEqual = 3,
    GPUCompareFunction_Greater = 4,
    GPUCompareFunction_GreaterEqual = 5,
    GPUCompareFunction_Equal = 6,
    GPUCompareFunction_NotEqual = 7,
    GPUCompareFunction_Always = 8,
    _GPUCompareFunction_Force32 = 0x7FFFFFFF
} GPUCompareFunction;

typedef enum GPUFilterMode {
    GPUFilterMode_Nearest = 0,
    GPUFilterMode_Linear = 1,
    _GPUFilterMode_Force32 = 0x7FFFFFFF
} GPUFilterMode;

typedef enum GPUAddressMode {
    GPUAddressMode_Repeat = 0,
    GPUAddressMode_MirrorRepeat = 1,
    GPUAddressMode_ClampToEdge = 2,
    GPUAddressMode_ClampToBorder = 3,
    _GPUAddressMode_Force32 = 0x7FFFFFFF
} GPUAddressMode;

typedef enum GPUBorderColor {
    GPUBorderColor_TransparentBlack = 0,
    GPUBorderColor_OpaqueBlack = 1,
    GPUBorderColor_OpaqueWhite = 2,
    _GPUBorderColor_Force32 = 0x7FFFFFFF
} GPUBorderColor;

typedef enum vgpu_load_action {
    VGPU_LOAD_ACTION_LOAD,
    VGPU_LOAD_ACTION_CLEAR,
    VGPU_LOAD_ACTION_DONT_CARE,
    VGPU_LOAD_ACTION_FORCE_U32 = 0x7FFFFFFF
} vgpu_load_action;

typedef enum vgpu_store_action {
    VGPU_STORE_ACTION_STORE,
    VGPU_STORE_ACTION_DONT_CARE,
    _GPU_STORE_ACTION_FORCE_U32 = 0x7FFFFFFF
} vgpu_store_action;

typedef enum GPUTextureLayout {
    GPUTextureLayout_Undefined = 0,
    GPUTextureLayout_General,
    GPUTextureLayout_RenderTarget,
    GPUTextureLayout_ShaderRead,
    GPUTextureLayout_ShaderWrite,
    GPUTextureLayout_Present,
    _GPUTextureLayout_Force32 = 0x7FFFFFFF
} GPUTextureLayout;

typedef enum GPUPowerPreference {
    GPUPowerPreference_Default = 0,
    GPUPowerPreference_LowPower = 1,
    GPUPowerPreference_HighPerformance = 2,
    _GPUPowerPreference_Force32 = 0x7FFFFFFF
} GPUPowerPreference;

typedef enum agpu_device_flags {
    AGPU_DEVICE_FLAGS_NONE = 0,
    AGPU_DEVICE_FLAGS_DEBUG = (1 << 0),
    AGPU_DEVICE_FLAGS_VSYNC = (1 << 1),
    AGPU_DEVICE_FLAGS_GPU_VALIDATION = (1 << 2),
    _AGPU_DEVICE_FLAGS_FORCE_U32 = 0x7FFFFFFF
} agpu_device_flags;

/* Callbacks */
typedef void(*GPULogCallback)(void* userData, GPULogLevel level, const char* message);

/* Structs */
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
} GPUExtent3D;

typedef struct {
    float r;
    float g;
    float b;
    float a;
} GPUColor;

typedef struct {
    int32_t     x;
    int32_t     y;
    int32_t    width;
    int32_t    height;
} GPURect;

typedef struct {
    float       x;
    float       y;
    float       width;
    float       height;
    float       minDepth;
    float       maxDepth;
} GPUViewport;

typedef struct vgpu_features {
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

typedef struct GPUDeviceCapabilities {
    /// The backend type.
    agpu_backend_type backend;

    /// Selected GPU vendor PCI id.
    uint32_t vendor_id;
    uint32_t device_id;
    char adapter_name[AGPU_MAX_DEVICE_NAME_SIZE];

    vgpu_features features;
    vgpu_limits limits;
} GPUDeviceCapabilities;

typedef struct vgpu_buffer_desc {
    gpu_buffer_usage_flags usage;
    uint32_t size;
    const void* content;
    const char* label;
} vgpu_buffer_desc;

typedef struct AGPUTextureDescriptor {
    AGPUTextureType type;
    AGPUPixelFormat format;
    AGPUTextureUsageFlags usage;
    GPUExtent3D size;
    uint32_t mipLevelCount;
    uint32_t sampleCount;
    const void* externalHandle; /* Pointer to external texture handle */
    const char* label;
} AGPUTextureDescriptor;

typedef struct {
    AGPUTexture texture;
    uint32_t mip_level;
    uint32_t slice;
    vgpu_load_action load_action;
    vgpu_store_action store_action;
    GPUColor clear_color;
} vgpu_color_attachment;

typedef struct vgpu_depth_stencil_attachment {
    AGPUTexture        texture;
    vgpu_load_action    depth_load_action;
    vgpu_store_action   depth_store_action;
    float               clear_depth;
    vgpu_load_action    stencil_load_action;
    vgpu_store_action   stencil_store_action;
    uint8_t             clear_stencil;
} vgpu_depth_stencil_attachment;

typedef struct VGPURenderPassDescriptor {
    vgpu_color_attachment           color_attachments[AGPU_MAX_COLOR_ATTACHMENTS];
    vgpu_depth_stencil_attachment   depth_stencil_attachment;
} VGPURenderPassDescriptor;

typedef struct GPUVertexBufferLayoutDescriptor {
    uint32_t                stride;
    GPUInputStepMode        stepMode;
} GPUVertexBufferLayoutDescriptor;

typedef struct VgpuVertexAttributeDescriptor {
    AGPUVertexFormat format;
    uint32_t offset;
    uint32_t bufferIndex;
} VgpuVertexAttributeDescriptor;

typedef struct VgpuVertexDescriptor {
    GPUVertexBufferLayoutDescriptor     layouts[AGPU_MAX_VERTEX_BUFFER_BINDINGS];
    VgpuVertexAttributeDescriptor       attributes[AGPU_MAX_VERTEX_ATTRIBUTES];
} VgpuVertexDescriptor;

typedef struct vgpu_shader_stage_desc {
    uint64_t        byte_code_size;
    const uint8_t* byte_code;
    const char* source;
    const char* entry_point;
} vgpu_shader_stage_desc;

typedef struct vgpu_shader_desc {
    vgpu_shader_stage_desc vertex;
    vgpu_shader_stage_desc fragment;
} vgpu_shader_desc;

typedef struct vgpu_render_pipeline_desc {
    vgpu_shader                 shader;
    VgpuVertexDescriptor        vertexDescriptor;
    AGPUPrimitiveTopology primitiveTopology;
} vgpu_render_pipeline_desc;

typedef struct VgpuComputePipelineDescriptor {
    uint32_t dummy;
    //VgpuShader                  shader;
} VgpuComputePipelineDescriptor;

typedef struct AGPUSamplerDescriptor {
    GPUAddressMode addressModeU;
    GPUAddressMode addressModeV;
    GPUAddressMode addressModeW;
    GPUFilterMode magFilter;
    GPUFilterMode minFilter;
    GPUFilterMode mipmapFilter;
    float lodMinClamp;
    float lodMaxClamp;
    GPUCompareFunction compare;
    uint32_t maxAnisotropy;
    GPUBorderColor borderColor;
    const char* label;
} AGPUSamplerDescriptor;

typedef struct agpu_swapchain_info {
    uintptr_t native_handle;

    AGPUPixelFormat colorFormat;
    uint32_t width;
    uint32_t height;
} agpu_swapchain_info;

typedef struct agpu_device_info {
    agpu_backend_type preferred_backend;
    uint32_t flags;
    GPUPowerPreference power_preference;
    const agpu_swapchain_info* swapchain_info;
} agpu_device_info;

#ifdef __cplusplus
extern "C"
{
#endif
    /* Log functions */
    VGPU_EXPORT void agpuSetLogLevel(GPULogLevel level);
    VGPU_EXPORT void agpuSetLogCallback(GPULogCallback callback, void* userData);
    VGPU_EXPORT void agpuLog(GPULogLevel level, const char* format, ...);

    /* Backend functions */
    VGPU_EXPORT agpu_backend_type gpu_get_default_platform_backend(void);
    VGPU_EXPORT bool gpu_is_backend_supported(agpu_backend_type backend);

    /* Device functions */
    VGPU_EXPORT GPUDevice agpuCreateDevice(const agpu_device_info* info);
    VGPU_EXPORT void agpu_destroy_device(GPUDevice device);
    VGPU_EXPORT void agpu_frame_begin(GPUDevice device);
    VGPU_EXPORT void agpu_frame_end(GPUDevice device);
    VGPU_EXPORT void agpu_wait_gpu(GPUDevice device);
    VGPU_EXPORT agpu_backend_type gpu_query_backend(GPUDevice device);
    VGPU_EXPORT GPUDeviceCapabilities gpu_query_caps(GPUDevice device);
    VGPU_EXPORT AGPUPixelFormat gpuGetDefaultDepthFormat(GPUDevice device);
    VGPU_EXPORT AGPUPixelFormat gpuGetDefaultDepthStencilFormat(GPUDevice device);

    VGPU_EXPORT AGPUTexture agpuDeviceCreateTexture(GPUDevice device, const AGPUTextureDescriptor* descriptor);
    VGPU_EXPORT void agpuDeviceDestroyTexture(GPUDevice device, AGPUTexture texture);

#if TODO
    //VGPU_EXPORT VGPURenderPass vgpu_get_default_render_pass(void);
    
    /* Buffer */
    VGPU_EXPORT vgpu_buffer vgpu_create_buffer(const vgpu_buffer_desc* desc);
    VGPU_EXPORT void vgpu_destroy_buffer(vgpu_buffer buffer);

    /* Texture */
    VGPU_EXPORT vgpu_texture vgpu_create_texture(const vgpu_texture_desc* desc);
    VGPU_EXPORT vgpu_texture vgpu_create_texture_cube(uint32_t size, vgpu_pixel_format format, uint32_t mip_levels, uint32_t layers, vgpu_texture_usage_flags usage, const void* initial_data);
    VGPU_EXPORT void vgpu_destroy_texture(vgpu_texture texture);
    VGPU_EXPORT vgpu_texture_desc vgpu_query_texture_desc(vgpu_texture texture);
    VGPU_EXPORT uint32_t vgpu_get_texture_width(vgpu_texture texture, uint32_t mip_level);
    VGPU_EXPORT uint32_t vgpu_get_texture_height(vgpu_texture texture, uint32_t mip_level);

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

    /* Shader */
    VGPU_EXPORT vgpu_shader vgpu_create_shader(const vgpu_shader_desc* desc);
    VGPU_EXPORT void vgpu_destroy_shader(vgpu_shader shader);

    /* Pipeline */
    VGPU_EXPORT vgpu_pipeline vgpu_create_render_pipeline(const vgpu_render_pipeline_desc* desc);
    VGPU_EXPORT vgpu_pipeline vgpu_create_compute_pipeline(const VgpuComputePipelineDescriptor* desc);
    VGPU_EXPORT void vgpu_destroy_pipeline(vgpu_pipeline pipeline);

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
#endif // TODO


    /* Helper methods */
    /// Get the number of bits per format
    VGPU_EXPORT uint32_t agpuGetFormatBitsPerPixel(AGPUPixelFormat format);
    VGPU_EXPORT uint32_t agpuGetFormatBlockSize(AGPUPixelFormat format);

    /// Get the format compression ration along the x-axis.
    VGPU_EXPORT uint32_t agpuGetFormatBlockWidth(AGPUPixelFormat format);
    /// Get the format compression ration along the y-axis.
    VGPU_EXPORT uint32_t agpuGetFormatBlockHeight(AGPUPixelFormat format);
    /// Get the format Type.
    VGPU_EXPORT AGPUPixelFormatType agpuGetFormatType(AGPUPixelFormat format);

    /// Check if the format has a depth component.
    VGPU_EXPORT bool agpuIsDepthFormat(AGPUPixelFormat format);
    /// Check if the format has a stencil component.
    VGPU_EXPORT bool agpuIsStencilFrmat(AGPUPixelFormat format);
    /// Check if the format has depth or stencil components.
    VGPU_EXPORT bool agpuIsDepthStencilFormat(AGPUPixelFormat format);
    /// Check if the format is a compressed format.
    VGPU_EXPORT bool agpuIsCompressedFormat(AGPUPixelFormat format);
    /// Get format string name.
    VGPU_EXPORT const char* agpuGetFormatName(AGPUPixelFormat format);
    VGPU_EXPORT bool agpuIsSrgbFormat(AGPUPixelFormat format);

#ifdef __cplusplus
}
#endif
