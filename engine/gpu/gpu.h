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

typedef struct GPUSurfaceImpl* GPUSurface;
typedef struct GPUDeviceImpl* GPUDevice;
typedef struct GPUSwapChainImpl* GPUSwapChain;
typedef struct vgpu_buffer_t* vgpu_buffer;
typedef struct vgpu_texture_t* vgpu_texture;
//typedef struct VGPUTextureViewImpl* VGPUTextureView;
typedef struct GPUSamplerImpl* GPUSampler;
typedef struct VGPURenderPassImpl* VGPURenderPass;
typedef struct vgpu_shader_t* vgpu_shader;
typedef struct vgpu_pipeline_t* vgpu_pipeline;

enum {
    GPU_MAX_COLOR_ATTACHMENTS = 8u,
    GPU_MAX_DEVICE_NAME_SIZE = 256,
    GPU_MAX_VERTEX_BUFFER_BINDINGS = 8u,
    GPU_MAX_VERTEX_ATTRIBUTES = 16u,
    GPU_MAX_VERTEX_ATTRIBUTE_OFFSET = 2047u,
    GPU_MAX_VERTEX_BUFFER_STRIDE = 2048u,
};

typedef enum GPULogLevel {
    GPULogLevel_Off = 0,
    GPULogLevel_Error = 1,
    GPULogLevel_Warn = 2,
    GPULogLevel_Info = 3,
    GPULogLevel_Debug = 4,
    GPULogLevel_Trace = 5,
    _GPULogLevel_Count,
    _GPULogLevel_Force32 = 0x7FFFFFFF
} GPULogLevel;

typedef enum GPUBackendType {
    GPUBackendType_Default = 0,
    GPUBackendType_Null = 1,
    GPUBackendType_Vulkan = 2,
    GPUBackendType_D3D12 = 3,
    GPUBackendType_D3D11 = 4,
    GPUBackendType_OpenGL = 5,
    _GPUBackendType_Force32 = 0x7FFFFFFF
} GPUBackendType;

typedef enum GPUPresentMode {
    GPUPresentMode_Fifo = 0,
    GPUPresentMode_Mailbox = 1,
    GPUPresentMode_Immediate = 2,
    _GPUPresentMode_Force32 = 0x7FFFFFFF
} GPUPresentMode;

/// Defines pixel format.
typedef enum GPUTextureFormat {
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

    GPU_TEXTURE_FORMAT_RGBA8_UNORM,
    GPU_TEXTURE_FORMAT_RGBA8_UNORM_SRGB,
    GPU_TEXTURE_FORMAT_RGBA8_SNORM,
    GPU_TEXTURE_FORMAT_RGBA8_UINT,
    GPU_TEXTURE_FORMAT_RGBA8_SINT,
    GPU_TEXTURE_FORMAT_BGRA8_UNORM,
    GPU_TEXTURE_FORMAT_BGRA8_UNORM_SRGB,

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

    _GPU_PIXEL_FORMAT_COUNT,
    _GPU_PIXEL_FORMAT_FORCE_U32 = 0x7FFFFFFF
} GPUTextureFormat;

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

typedef enum GPUTextureType {
    GPU_TEXTURE_TYPE_2D = 0,
    GPU_TEXTURE_TYPE_3D,
    GPU_TEXTURE_TYPE_CUBE,
    _GPU_TEXTURE_TYPE_FORCE_U32 = 0x7FFFFFFF
} GPUTextureType;

typedef enum GPUTextureUsage {
    GPU_TEXTURE_USAGE_NONE = 0,
    GPU_TEXTURE_USAGE_SAMPLED = 0x01,
    GPU_TEXTURE_USAGE_STORAGE = 0x02,
    GPU_TEXTURE_USAGE_OUTPUT_ATTACHMENT = 0x04,
    _GPU_TEXTURE_USAGE_FORCE_U32 = 0x7FFFFFFF
} GPUTextureUsage;
typedef uint32_t GPUTextureUsageFlags;

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

typedef enum gpu_vertex_format {
    GPU_VERTEX_FORMAT_Invalid = 0,
    GPU_VERTEX_FORMAT_UChar2,
    GPU_VERTEX_FORMAT_UChar4,
    GPU_VERTEX_FORMAT_Char2,
    GPU_VERTEX_FORMAT_Char4,
    GPU_VERTEX_FORMAT_UChar2Norm,
    GPU_VERTEX_FORMAT_UChar4Norm,
    GPU_VERTEX_FORMAT_Char2Norm,
    GPU_VERTEX_FORMAT_Char4Norm,
    GPU_VERTEX_FORMAT_UShort2,
    GPU_VERTEX_FORMAT_UShort4,
    GPU_VERTEX_FORMAT_Short2,
    GPU_VERTEX_FORMAT_Short4,
    GPU_VERTEX_FORMAT_UShort2Norm,
    GPU_VERTEX_FORMAT_UShort4Norm,
    GPU_VERTEX_FORMAT_Short2Norm,
    GPU_VERTEX_FORMAT_Short4Norm,
    GPU_VERTEX_FORMAT_Half2,
    GPU_VERTEX_FORMAT_Half4,
    GPU_VERTEX_FORMAT_Float,
    GPU_VERTEX_FORMAT_Float2,
    GPU_VERTEX_FORMAT_Float3,
    GPU_VERTEX_FORMAT_Float4,
    GPU_VERTEX_FORMAT_UInt,
    GPU_VERTEX_FORMAT_UInt2,
    GPU_VERTEX_FORMAT_UInt3,
    GPU_VERTEX_FORMAT_UInt4,
    GPU_VERTEX_FORMAT_Int,
    GPU_VERTEX_FORMAT_Int2,
    GPU_VERTEX_FORMAT_Int3,
    GPU_VERTEX_FORMAT_Int4,
    _GPU_VERTEX_FORMAT_FORCE_U32 = 0x7FFFFFFF
} gpu_vertex_format;

typedef enum GPUInputStepMode {
    GPUInputStepMode_Vertex = 0,
    GPUInputStepMode_Instance = 1,
    _GPUInputStepMode_Force32 = 0x7FFFFFFF
} GPUInputStepMode;

typedef enum GPUPrimitiveTopology {
    GPUPrimitiveTopology_PointList = 0x00000000,
    GPUPrimitiveTopology_LineList = 0x00000001,
    GPUPrimitiveTopology_LineStrip = 0x00000002,
    GPUPrimitiveTopology_TriangleList = 0x00000003,
    GPUPrimitiveTopology_TriangleStrip = 0x00000004,
    GPUPrimitiveTopology_Force32 = 0x7FFFFFFF
} GPUPrimitiveTopology;

typedef enum GPUIndexFormat {
    GPUIndexFormat_Uint16 = 0,
    GPUIndexFormat_Uint32 = 1,
    _GPUIndexFormat_Force32 = 0x7FFFFFFF
} GPUIndexFormat;

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

typedef enum VGPUTextureLayout {
    VGPUTextureLayout_Undefined = 0,
    VGPUTextureLayout_General,
    VGPUTextureLayout_RenderTarget,
    VGPUTextureLayout_ShaderRead,
    VGPUTextureLayout_ShaderWrite,
    VGPUTextureLayout_Present,
    VGPUTextureLayout_Force32 = 0x7FFFFFFF
} VGPUTextureLayout;

typedef enum GPUPowerPreference {
    GPU_POWER_PREFERENCE_DEFAULT = 0,
    GPU_POWER_PREFERENCE_LOW_POWER = 1,
    GPU_POWER_PREFERENCE_HIGH_PERFORMANCE = 2,
    _GPU_POWER_PREFERENCE_FORCE_U32 = 0x7FFFFFFF
} GPUPowerPreference;

typedef enum GPUDebugUsage {
    GPUDebugFlags_None = 0,
    GPUDebugFlags_Debug = 0x01,
    GPUDebugFlags_GPUBasedValidation = 0x02,
    GPUDebugFlags_RenderDoc = 0x04,
    _GPUDebugFlags_Force32 = 0x7FFFFFFF
} GPUDebugUsage;
typedef uint32_t GPUDebugFlags;

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
    GPUBackendType backend;

    /// Selected GPU vendor PCI id.
    uint32_t vendor_id;
    uint32_t device_id;
    char adapter_name[GPU_MAX_DEVICE_NAME_SIZE];

    vgpu_features features;
    vgpu_limits limits;
} GPUDeviceCapabilities;

typedef struct vgpu_buffer_desc {
    gpu_buffer_usage_flags usage;
    uint32_t size;
    const void* content;
    const char* label;
} vgpu_buffer_desc;

typedef struct GPUTextureDescriptor {
    GPUTextureType type;
    GPUTextureFormat format;
    GPUTextureUsageFlags usage;
    GPUExtent3D size;
    uint32_t mipLevelCount;
    vgpu_sample_count sampleCount;
    /// Pointer to external texture handle
    const void* externalHandle;
    const char* label;
} GPUTextureDescriptor;

typedef struct {
    vgpu_texture texture;
    uint32_t mip_level;
    uint32_t slice;
    vgpu_load_action load_action;
    vgpu_store_action store_action;
    GPUColor clear_color;
} vgpu_color_attachment;

typedef struct vgpu_depth_stencil_attachment {
    vgpu_texture        texture;
    vgpu_load_action    depth_load_action;
    vgpu_store_action   depth_store_action;
    float               clear_depth;
    vgpu_load_action    stencil_load_action;
    vgpu_store_action   stencil_store_action;
    uint8_t             clear_stencil;
} vgpu_depth_stencil_attachment;

typedef struct VGPURenderPassDescriptor {
    vgpu_color_attachment           color_attachments[GPU_MAX_COLOR_ATTACHMENTS];
    vgpu_depth_stencil_attachment   depth_stencil_attachment;
} VGPURenderPassDescriptor;

typedef struct GPUVertexBufferLayoutDescriptor {
    uint32_t                stride;
    GPUInputStepMode        stepMode;
} GPUVertexBufferLayoutDescriptor;

typedef struct VgpuVertexAttributeDescriptor {
    gpu_vertex_format   format;
    uint32_t            offset;
    uint32_t            buffer_index;
} VgpuVertexAttributeDescriptor;

typedef struct VgpuVertexDescriptor {
    GPUVertexBufferLayoutDescriptor     layouts[GPU_MAX_VERTEX_BUFFER_BINDINGS];
    VgpuVertexAttributeDescriptor       attributes[GPU_MAX_VERTEX_ATTRIBUTES];
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
    GPUPrimitiveTopology        primitiveTopology;
} vgpu_render_pipeline_desc;

typedef struct VgpuComputePipelineDescriptor {
    uint32_t dummy;
    //VgpuShader                  shader;
} VgpuComputePipelineDescriptor;

typedef struct GPUSamplerDescriptor {
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
} GPUSamplerDescriptor;

typedef struct GPUSwapChainDescriptor {
    GPUTextureUsageFlags    usage;
    GPUTextureFormat        format;
    uint32_t                width;
    uint32_t                height;
    GPUPresentMode          presentMode;
} GPUSwapChainDescriptor;

typedef struct GPUInitConfig {
    GPUDebugFlags flags;
} GPUInitConfig;

typedef struct GPUDeviceDescriptor {
    GPUBackendType preferredBackend;
    GPUPowerPreference powerPreference;
    GPUSurface compatibleSurface;
} GPUDeviceDescriptor;

#ifdef __cplusplus
extern "C"
{
#endif
    /* Log functions */
    VGPU_EXPORT void gpuSetLogLevel(GPULogLevel level);
    VGPU_EXPORT void gpuSetLogCallback(GPULogCallback callback, void* userData);
    VGPU_EXPORT void gpuLog(GPULogLevel level, const char* format, ...);

    /* Backend functions */
    VGPU_EXPORT GPUBackendType gpu_get_default_platform_backend(void);
    VGPU_EXPORT bool gpu_is_backend_supported(GPUBackendType backend);
    VGPU_EXPORT bool gpuInit(const GPUInitConfig* config);
    VGPU_EXPORT void gpuShutdown(void);
    VGPU_EXPORT GPUSurface gpuCreateWin32Surface(void* hinstance, void* hwnd);

    /* Device functions */
    VGPU_EXPORT GPUDevice gpuDeviceCreate(const GPUDeviceDescriptor* desc);
    VGPU_EXPORT void gpuDeviceDestroy(GPUDevice device);
    VGPU_EXPORT GPUBackendType gpuQueryBackend(GPUDevice device);
    VGPU_EXPORT GPUDeviceCapabilities gpuQueryCaps(GPUDevice device);
    VGPU_EXPORT GPUTextureFormat gpuGetPreferredSwapChainFormat(GPUDevice device, GPUSurface surface);
    VGPU_EXPORT GPUTextureFormat gpuGetDefaultDepthFormat(GPUDevice device);
    VGPU_EXPORT GPUTextureFormat gpuGetDefaultDepthStencilFormat(GPUDevice device);

    VGPU_EXPORT GPUSwapChain gpuDeviceCreateSwapChain(GPUDevice device, GPUSurface surface, const GPUSwapChainDescriptor* descriptor);
    VGPU_EXPORT void gpuDeviceDestroySwapChain(GPUDevice device, GPUSwapChain swapChain);

#if TODO
    //VGPU_EXPORT VGPURenderPass vgpu_get_default_render_pass(void);
    

    VGPU_EXPORT void gpu_wait_idle(void);
    VGPU_EXPORT void gpu_begin_frame(void);
    VGPU_EXPORT void gpu_end_frame(void);


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
    VGPU_EXPORT uint32_t vgpu_get_format_bits_per_pixel(GPUTextureFormat format);
    VGPU_EXPORT uint32_t vgpu_get_format_block_size(GPUTextureFormat format);

    /// Get the format compression ration along the x-axis.
    VGPU_EXPORT uint32_t vgpuGetFormatBlockWidth(GPUTextureFormat format);
    /// Get the format compression ration along the y-axis.
    VGPU_EXPORT uint32_t vgpuGetFormatBlockHeight(GPUTextureFormat format);
    /// Get the format Type.
    VGPU_EXPORT vgpu_pixel_format_type vgpu_get_format_type(GPUTextureFormat format);

    /// Check if the format has a depth component.
    VGPU_EXPORT bool vgpu_is_depth_format(GPUTextureFormat format);
    /// Check if the format has a stencil component.
    VGPU_EXPORT bool vgpu_is_stencil_format(GPUTextureFormat format);
    /// Check if the format has depth or stencil components.
    VGPU_EXPORT bool vgpu_is_depth_stencil_format(GPUTextureFormat format);
    /// Check if the format is a compressed format.
    VGPU_EXPORT bool vgpu_is_compressed_format(GPUTextureFormat format);
    /// Get format string name.
    VGPU_EXPORT const char* vgpu_get_format_name(GPUTextureFormat format);

#ifdef __cplusplus
}
#endif
