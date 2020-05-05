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

static constexpr uint32_t kInvalidHandle = 0xFFFFFFFF;

typedef struct GPUDeviceImpl* GPUDevice;
struct BufferHandle { uint32_t id; bool isValid() const { return id != kInvalidHandle; } };
struct TextureHandle { uint32_t id; bool isValid() const { return id != kInvalidHandle; } };
typedef struct AGPUTextureViewImpl* AGPUTextureView;
typedef struct AGPUSamplerImpl* AGPUSampler;
//typedef struct VGPURenderPassImpl* VGPURenderPass;
//typedef struct vgpu_shader_t* vgpu_shader;
//typedef struct vgpu_pipeline_t* vgpu_pipeline;

static constexpr BufferHandle kInvalidBufferHandle = { kInvalidHandle };
static constexpr TextureHandle kInvalidTextureHandle = { kInvalidHandle };

enum {
    AGPU_NUM_INFLIGHT_FRAMES = 2u,
    AGPU_MAX_COLOR_ATTACHMENTS = 8u,
    AGPU_MAX_DEVICE_NAME_SIZE = 256,
    AGPU_MAX_VERTEX_BUFFER_BINDINGS = 8u,
    AGPU_MAX_VERTEX_ATTRIBUTES = 16u,
    AGPU_MAX_VERTEX_ATTRIBUTE_OFFSET = 2047u,
    AGPU_MAX_VERTEX_BUFFER_STRIDE = 2048u,
};

typedef enum AGPULogLevel {
    AGPULogLevel_Off = 0,
    AGPULogLevel_Error = 1,
    AGPULogLevel_Warn = 2,
    AGPULogLevel_Info = 3,
    AGPULogLevel_Debug = 4,
    AGPULogLevel_Trace = 5,
    AGPULogLevel_Count,
    AGPULogLevel_Force32 = 0x7FFFFFFF
} AGPULogLevel;

typedef enum AGPUBackendType {
    AGPUBackendType_Default = 0,
    AGPUBackendType_Null = 1,
    AGPUBackendType_Vulkan = 2,
    AGPUBackendType_D3D12 = 3,
    AGPUBackendType_D3D11 = 4,
    AGPUBackendType_OpenGL = 5,
    AGPUBackendType_Force32 = 0x7FFFFFFF
} AGPUBackendType;

/// Defines pixel format.
typedef enum AGPUPixelFormat {
    AGPUPixelFormat_Undefined = 0,
    // 8-bit pixel formats
    AGPUPixelFormat_R8Unorm,
    AGPUPixelFormat_R8Snorm,
    AGPUPixelFormat_R8Uint,
    AGPUPixelFormat_R8Sint,
    // 16-bit pixel formats
    AGPUPixelFormat_R16Uint,
    AGPUPixelFormat_R16Sint,
    AGPUPixelFormat_R16Float,
    AGPUPixelFormat_RG8Unorm,
    AGPUPixelFormat_RG8Snorm,
    AGPUPixelFormat_RG8Uint,
    AGPUPixelFormat_RG8Sint,
    // 32-bit pixel formats
    AGPUPixelFormat_R32Uint,
    AGPUPixelFormat_R32Sint,
    AGPUPixelFormat_R32Float,
    AGPUPixelFormat_RG16Uint,
    AGPUPixelFormat_RG16Sint,
    AGPUPixelFormat_RG16Float,
    AGPUPixelFormat_RGBA8Unorm,
    AGPUPixelFormat_RGBA8UnormSrgb,
    AGPUPixelFormat_RGBA8Snorm,
    AGPUPixelFormat_RGBA8Uint,
    AGPUPixelFormat_RGBA8Sint,
    AGPUPixelFormat_BGRA8Unorm,
    AGPUPixelFormat_BGRA8UnormSrgb,
    // Packed 32-Bit Pixel formats
    AGPUPixelFormat_RGB10A2Unorm,
    AGPUPixelFormat_RG11B10Float,
    // 64-Bit Pixel Formats
    AGPUPixelFormat_RG32Uint,
    AGPUPixelFormat_RG32Sint,
    AGPUPixelFormat_RG32Float,
    AGPUPixelFormat_RGBA16Uint,
    AGPUPixelFormat_RGBA16Sint,
    AGPUPixelFormat_RGBA16Float,
    // 128-Bit Pixel Formats
    AGPUPixelFormat_RGBA32Uint,
    AGPUPixelFormat_RGBA32Sint,
    AGPUPixelFormat_RGBA32Float,

    // Depth-stencil
    AGPUPixelFormat_Depth16Unorm,
    AGPUPixelFormat_Depth32Float,
    AGPUPixelFormat_Depth24Plus,
    AGPUPixelFormat_Depth24PlusStencil8,

    // Compressed formats
    AGPUPixelFormat_BC1RGBAUnorm,
    AGPUPixelFormat_BC1RGBAUnormSrgb,
    AGPUPixelFormat_BC2RGBAUnorm,
    AGPUPixelFormat_BC2RGBAUnormSrgb,
    AGPUPixelFormat_BC3RGBAUnorm,
    AGPUPixelFormat_BC3RGBAUnormSrgb,
    AGPUPixelFormat_BC4RUnorm,
    AGPUPixelFormat_BC4RSnorm,
    AGPUPixelFormat_BC5RGUnorm,
    AGPUPixelFormat_BC5RGSnorm,
    AGPUPixelFormat_BC6HRGBUFloat,
    AGPUPixelFormat_BC6HRGBSFloat,
    AGPUPixelFormat_BC7RGBAUnorm,
    AGPUPixelFormat_BC7RGBAUnormSrgb,

    AGPUPixelFormat_Count,
    AGPUPixelFormat_Force32 = 0x7FFFFFFF
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
typedef AGPUFlags AGPUBufferUsageFlags;

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

typedef enum AGPUInputStepMode {
    AGPUInputStepMode_Vertex = 0,
    AGPUInputStepMode_Instance = 1,
    AGPUInputStepMode_Force32 = 0x7FFFFFFF
} AGPUInputStepMode;

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

typedef enum AGPUCompareFunction {
    AGPUCompareFunction_Undefined = 0x00000000,
    AGPUCompareFunction_Never = 0x00000001,
    AGPUCompareFunction_Less = 0x00000002,
    AGPUCompareFunction_LessEqual = 0x00000003,
    AGPUCompareFunction_Greater = 0x00000004,
    AGPUCompareFunction_GreaterEqual = 0x00000005,
    AGPUCompareFunction_Equal = 0x00000006,
    AGPUCompareFunction_NotEqual = 0x00000007,
    AGPUCompareFunction_Always = 0x00000008,
    AGPUCompareFunction_Force32 = 0x7FFFFFFF
} AGPUCompareFunction;

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

typedef enum AGPUPowerPreference {
    AGPUPowerPreference_Default = 0,
    AGPUPowerPreference_LowPower = 1,
    AGPUPowerPreference_HighPerformance = 2,
    AGPUPowerPreference_Force32 = 0x7FFFFFFF
} AGPUPowerPreference;

typedef enum agpu_device_flags {
    AGPU_DEVICE_FLAGS_NONE = 0,
    AGPU_DEVICE_FLAGS_DEBUG = (1 << 0),
    AGPU_DEVICE_FLAGS_VSYNC = (1 << 1),
    AGPU_DEVICE_FLAGS_GPU_VALIDATION = (1 << 2),
    _AGPU_DEVICE_FLAGS_FORCE_U32 = 0x7FFFFFFF
} agpu_device_flags;

/* Callbacks */
typedef void(*GPULogCallback)(void* userData, AGPULogLevel level, const char* message);

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

typedef struct AGPUDeviceCapabilities {
    /// The backend type.
    AGPUBackendType backend;

    /// Selected GPU vendor PCI id.
    uint32_t vendor_id;
    uint32_t device_id;
    char adapter_name[AGPU_MAX_DEVICE_NAME_SIZE];

    vgpu_features features;
    vgpu_limits limits;
} AGPUDeviceCapabilities;

typedef struct AGPUBufferDescriptor {
    AGPUBufferUsageFlags usage;
    uint32_t size;
    uint32_t stride;
    const void* content;
    const void* externalHandle; /* Pointer to external texture handle */
    const char* label;
} AGPUBufferDescriptor;

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
    TextureHandle texture;
    uint32_t mip_level;
    uint32_t slice;
    vgpu_load_action load_action;
    vgpu_store_action store_action;
    GPUColor clear_color;
} vgpu_color_attachment;

typedef struct vgpu_depth_stencil_attachment {
    TextureHandle        texture;
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
    AGPUInputStepMode        stepMode;
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
    //vgpu_shader                 shader;
    VgpuVertexDescriptor        vertexDescriptor;
    AGPUPrimitiveTopology primitiveTopology;
} vgpu_render_pipeline_desc;

typedef struct VgpuComputePipelineDescriptor {
    uint32_t dummy;
    //VgpuShader                  shader;
} VgpuComputePipelineDescriptor;

typedef struct AGPUSamplerDescriptor {
    const char* label;
    GPUAddressMode addressModeU;
    GPUAddressMode addressModeV;
    GPUAddressMode addressModeW;
    GPUFilterMode magFilter;
    GPUFilterMode minFilter;
    GPUFilterMode mipmapFilter;
    float lodMinClamp;
    float lodMaxClamp;
    AGPUCompareFunction compare;
    uint32_t maxAnisotropy;
    GPUBorderColor borderColor;
} AGPUSamplerDescriptor;

typedef struct AGPUSwapChainDescriptor {
    const char* label;
    uintptr_t nativeHandle;

    AGPUPixelFormat colorFormat;
    uint32_t width;
    uint32_t height;
} AGPUSwapChainDescriptor;

typedef struct agpu_device_info {
    AGPUBackendType preferredBackend;
    uint32_t flags;
    AGPUPowerPreference powerPreference;
    const AGPUSwapChainDescriptor* swapchain;
} agpu_device_info;

#ifdef __cplusplus
extern "C"
{
#endif
    /* Log functions */
    VGPU_EXPORT void agpuSetLogLevel(AGPULogLevel level);
    VGPU_EXPORT void agpuSetLogCallback(GPULogCallback callback, void* userData);
    VGPU_EXPORT void agpuLog(AGPULogLevel level, const char* format, ...);

    /* Backend functions */
    VGPU_EXPORT AGPUBackendType gpu_get_default_platform_backend(void);
    VGPU_EXPORT bool gpu_is_backend_supported(AGPUBackendType backend);

    /* Device functions */
    VGPU_EXPORT GPUDevice agpuCreateDevice(const agpu_device_info* info);
    VGPU_EXPORT void agpuDeviceDestroy(GPUDevice device);
    VGPU_EXPORT void agpu_frame_begin(GPUDevice device);
    VGPU_EXPORT void agpu_frame_end(GPUDevice device);
    VGPU_EXPORT void agpu_wait_gpu(GPUDevice device);
    VGPU_EXPORT AGPUBackendType agpuDeviceQueryBackend(GPUDevice device);
    VGPU_EXPORT AGPUDeviceCapabilities agpuDeviceQueryCaps(GPUDevice device);
    VGPU_EXPORT AGPUPixelFormat gpuGetDefaultDepthFormat(GPUDevice device);
    VGPU_EXPORT AGPUPixelFormat gpuGetDefaultDepthStencilFormat(GPUDevice device);

    /* Texture */
    VGPU_EXPORT TextureHandle agpuCreateTexture(const AGPUTextureDescriptor* descriptor);
    VGPU_EXPORT void agpuDestroyTexture(TextureHandle texture);

    /* Buffer */
    VGPU_EXPORT BufferHandle agpuCreateBuffer(const AGPUBufferDescriptor* descriptor);
    VGPU_EXPORT void agpuDestroyBuffer(BufferHandle buffer);

    /* Sampler */
    VGPU_EXPORT AGPUSampler agpuDeviceCreateSampler(GPUDevice device, const AGPUSamplerDescriptor* descriptor);
    VGPU_EXPORT void agpuDeviceDestroySampler(GPUDevice device, AGPUSampler sampler);

#if TODO
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
    VGPU_EXPORT AGPUPixelFormat agpuSrgbToLinearFormat(AGPUPixelFormat format);
    VGPU_EXPORT AGPUPixelFormat agpuLinearToSrgbFormat(AGPUPixelFormat format);

#ifdef __cplusplus
}
#endif
