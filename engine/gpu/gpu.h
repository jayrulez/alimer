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

typedef struct agpu_buffer_t* agpu_buffer;
typedef struct agpu_texture_t* agpu_texture;
typedef struct agpu_pass_t* agpu_pass;
typedef struct agpu_shader_t* agpu_shader;
typedef struct agpu_pipeline_t* agpu_pipeline;

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

typedef struct agpu_limits {
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

typedef struct AGPUDeviceCapabilities {
    /// The backend type.
    agpu_backend_type backend;

    /// Selected GPU vendor PCI id.
    uint32_t vendor_id;
    uint32_t device_id;
    char adapter_name[AGPU_MAX_DEVICE_NAME_SIZE];

    vgpu_features features;
} AGPUDeviceCapabilities;

typedef struct {
    agpu_texture texture;
    uint32_t mip_level;
    uint32_t slice;
    vgpu_load_action load_action;
    vgpu_store_action store_action;
    GPUColor clear_color;
} vgpu_color_attachment;

typedef struct vgpu_depth_stencil_attachment {
    agpu_texture texture;
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

typedef struct agpu_config {
    agpu_backend_type preferred_backend;
    uint32_t flags;
    void (*callback)(void* context, const char* message, AGPULogLevel level);
    void* context;
    struct {
        void* (*get_proc_address)(const char*);
    } gl;
    const AGPUSwapChainDescriptor* swapchain;
} agpu_config;

#ifdef __cplusplus
extern "C"
{
#endif
    /* Log functions */
    VGPU_EXPORT void agpuSetLogLevel(AGPULogLevel level);
    VGPU_EXPORT void agpuSetLogCallback(GPULogCallback callback, void* userData);
    VGPU_EXPORT void agpuLog(AGPULogLevel level, const char* format, ...);

    /* Backend functions */
    VGPU_EXPORT agpu_backend_type agpu_get_default_platform_backend(void);
    VGPU_EXPORT bool agpu_is_backend_supported(agpu_backend_type backend);

    /* Device functions */
    VGPU_EXPORT bool agpu_init(const agpu_config* config);
    VGPU_EXPORT void agpu_shutdown(void);
    VGPU_EXPORT void agpu_frame_begin(void);
    VGPU_EXPORT void agpu_frame_finish(void);
    VGPU_EXPORT agpu_backend_type agpu_query_backend(void);
    VGPU_EXPORT void agpu_get_limits(agpu_limits* limits);
    VGPU_EXPORT AGPUPixelFormat agpu_get_default_depth_format(void);
    VGPU_EXPORT AGPUPixelFormat agpu_get_default_depth_stencil_format(void);

    /* Texture */
    typedef struct {
        AGPUTextureType type;
        AGPUPixelFormat format;
        AGPUTextureUsageFlags usage;
        GPUExtent3D size;
        uint32_t mipLevelCount;
        uint32_t sampleCount;
        const void* externalHandle; /* Pointer to external texture handle */
        const char* label;
    } agpu_texture_info;

    VGPU_EXPORT agpu_texture agpu_texture_create(const agpu_texture_info* info);
    VGPU_EXPORT void agpu_texture_destroy(agpu_texture texture);

    /* Buffer */
    typedef enum agpu_buffer_usage {
        GPU_BUFFER_USAGE_NONE = 0,
        GPU_BUFFER_USAGE_VERTEX = (1 << 0),
        GPU_BUFFER_USAGE_INDEX = (1 << 1),
        GPU_BUFFER_USAGE_UNIFORM = (1 << 2),
        GPU_BUFFER_USAGE_STORAGE = (1 << 3),
        GPU_BUFFER_USAGE_INDIRECT = (1 << 4),
    } agpu_buffer_usage;
    typedef uint32_t agpu_buffer_usage_flags;

    typedef struct agpu_buffer_info {
        uint32_t size;
        agpu_buffer_usage_flags usage;
        const void* content;
        const char* label;
    } agpu_buffer_info;

    VGPU_EXPORT agpu_buffer agpu_create_buffer(const agpu_buffer_info* info);
    VGPU_EXPORT void agpu_destroy_buffer(agpu_buffer buffer);

    /* Shader */
    typedef struct {
        uint64_t code_size;
        const void* code;
        const char* source;
        const char* entry_point;
    } agpu_shader_source;

    typedef struct {
        agpu_shader_source vertex;
        agpu_shader_source fragment;
        agpu_shader_source compute;
        const char* label;
    } agpu_shader_info;

    VGPU_EXPORT agpu_shader agpu_create_shader(const agpu_shader_info* info);
    VGPU_EXPORT void agpu_destroy_shader(agpu_shader shader);

    /* Pipeline */
    typedef enum AGPUVertexFormat {
        AGPU_VERTEX_FORMAT_UCHAR2,
        AGPU_VERTEX_FORMAT_UCHAR4,
        AGPU_VERTEX_FORMAT_CHAR2,
        AGPU_VERTEX_FORMAT_CHAR4,
        AGPU_VERTEX_FORMAT_UCHAR2NORM,
        AGPU_VERTEX_FORMAT_UCHAR4NORM,
        AGPU_VERTEX_FORMAT_CHAR2NORM,
        AGPU_VERTEX_FORMAT_CHAR4NORM,
        AGPU_VERTEX_FORMAT_USHORT2,
        AGPU_VERTEX_FORMAT_USHORT4,
        AGPU_VERTEX_FORMAT_SHORT2,
        AGPU_VERTEX_FORMAT_SHORT4,
        AGPU_VERTEX_FORMAT_USHORT2NORM,
        AGPU_VERTEX_FORMAT_USHORT4NORM,
        AGPU_VERTEX_FORMAT_SHORT2NORM,
        AGPU_VERTEX_FORMAT_SHORT4NORM,
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

    typedef enum agpu_primitive_topology {
        AGPU_PRIMITIVE_TOPOLOGY_POINTS = 1,
        AGPU_PRIMITIVE_TOPOLOGY_LINES = 2,
        AGPU_PRIMITIVE_TOPOLOGY_LINE_STRIP = 3,
        AGPU_PRIMITIVE_TOPOLOGY_TRIANGLES = 4,
        AGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP = 5,
        _AGPU_PrimitiveTopology_FORCE_U32 = 0x7FFFFFFF
    } agpu_primitive_topology;

    typedef enum AGPUIndexFormat {
        AGPUIndexFormat_Uint16 = 0x00000000,
        AGPUIndexFormat_Uint32 = 0x00000001,
        AGPUIndexFormat_Force32 = 0x7FFFFFFF
    } AGPUIndexFormat;

    typedef enum AGPUInputStepMode {
        AGPUInputStepMode_Vertex = 0x00000000,
        AGPUInputStepMode_Instance = 0x00000001,
        AGPUInputStepMode_Force32 = 0x7FFFFFFF
    } AGPUInputStepMode;

    typedef struct AGPUVertexAttributeDescriptor {
        AGPUVertexFormat  format;
        uint32_t            shaderLocation;
    } AGPUVertexAttributeDescriptor;

    typedef struct AGPUVertexBufferLayoutDescriptor {
        AGPUInputStepMode stepMode;
        uint32_t attributeCount;
        const AGPUVertexAttributeDescriptor* attributes;
    } AGPUVertexBufferLayoutDescriptor;

    typedef struct AGPUVertexStateDescriptor {
        AGPUIndexFormat indexFormat;
        uintptr_t vertexBufferCount;
        const AGPUVertexBufferLayoutDescriptor* vertexBuffers;
    } AGPUVertexStateDescriptor;

    typedef struct agpu_render_pipeline_info {
        agpu_shader                 shader;
        AGPUVertexStateDescriptor   vertexState;
        agpu_primitive_topology     primitive_topology;
        const char*                 label;
    } agpu_render_pipeline_info;

    VGPU_EXPORT agpu_pipeline agpu_create_render_pipeline(const agpu_render_pipeline_info* info);
    VGPU_EXPORT void agpu_destroy_pipeline(agpu_pipeline pipeline);

    /* Sampler */
    //VGPU_EXPORT AGPUSampler agpuDeviceCreateSampler(GPUDevice device, const AGPUSamplerDescriptor* descriptor);
    //VGPU_EXPORT void agpuDeviceDestroySampler(GPUDevice device, AGPUSampler sampler);

    /* CommandBuffer */
    void agpu_set_pipeline(agpu_pipeline pipeline);
    void agpuCmdSetVertexBuffers(uint32_t slot, agpu_buffer buffer, uint64_t offset);
    void agpuCmdSetIndexBuffer(agpu_buffer buffer, uint64_t offset);
    void agpuCmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex);
    void agpuCmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex);

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

    VGPU_EXPORT uint32_t agpuGetVertexFormatComponentsCount(AGPUVertexFormat format);
    VGPU_EXPORT uint32_t agpuGetVertexFormatComponentSize(AGPUVertexFormat format);
    VGPU_EXPORT uint32_t agpuGetVertexFormatSize(AGPUVertexFormat format);

#ifdef __cplusplus
}
#endif
