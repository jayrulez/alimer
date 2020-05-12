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
#include <stdbool.h>

typedef uint32_t VGPUFlags;
typedef uint32_t VGPUBool32;
typedef uint64_t VGPUDeviceSize;
typedef uint32_t VGPUSampleMask;

typedef struct vgpu_buffer vgpu_buffer;
typedef struct VGPUTexture VGPUTexture;
typedef struct vgpu_pass_t* vgpu_pass;
typedef struct vgpu_shader_t* vgpu_shader;
typedef struct agpu_pipeline_t* agpu_pipeline;

enum {
    VGPU_NUM_INFLIGHT_FRAMES = 2u,
    AGPU_MAX_COLOR_ATTACHMENTS = 8u,
    VGPU_MAX_VERTEX_BUFFER_BINDINGS = 8u,
    VGPU_MAX_VERTEX_ATTRIBUTES = 16u,
    VGPU_MAX_VERTEX_ATTRIBUTE_OFFSET = 2047u,
    VGPU_MAX_VERTEX_BUFFER_STRIDE = 2048u,
};

/* Enums */
typedef enum {
    VGPULogLevel_Off = 0,
    VGPULogLevel_Error = 1,
    VGPULogLevel_Warn = 2,
    VGPULogLevel_Info = 3,
    VGPULogLevel_Debug = 4,
    VGPULogLevel_Trace = 5,
    VGPULogLevel_Count,
    VGPULogLevel_Force32 = 0x7FFFFFFF
} VGPULogLevel;

typedef enum {
    VGPUBackendType_Null = 0,
    VGPUBackendType_D3D11 = 1,
    VGPUBackendType_D3D12 = 2,
    VGPUBackendType_Vulkan = 3,
    VGPUBackendType_OpenGL = 4,
    VGPUBackendType_Count,
    VGPUBackendType_Force32 = 0x7FFFFFFF
} VGPUBackendType;

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
    AGPUPixelFormat_RGBA8UNorm,
    AGPUPixelFormat_RGBA8UNormSrgb,
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
    VGPU_COMPARE_FUNCTION_FORCE_U32 = 0x7FFFFFFF
} vgpu_compare_function;

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
    VGPUVertexFormat_Force32 = 0x7FFFFFFF
} VGPUVertexFormat;

typedef enum agpu_primitive_topology {
    AGPU_PRIMITIVE_TOPOLOGY_POINTS = 1,
    AGPU_PRIMITIVE_TOPOLOGY_LINES = 2,
    AGPU_PRIMITIVE_TOPOLOGY_LINE_STRIP = 3,
    AGPU_PRIMITIVE_TOPOLOGY_TRIANGLES = 4,
    AGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP = 5,
    _AGPU_PrimitiveTopology_FORCE_U32 = 0x7FFFFFFF
} agpu_primitive_topology;

typedef enum VGPUIndexType {
    VGPUIndexType_UInt16 = 0x00000000,
    VGPUIndexType_UInt32 = 0x00000001,
    VGPUIndexType_Force32 = 0x7FFFFFFF
} VGPUIndexType;

typedef enum AGPUInputStepMode {
    AGPUInputStepMode_Vertex = 0x00000000,
    AGPUInputStepMode_Instance = 0x00000001,
    AGPUInputStepMode_Force32 = 0x7FFFFFFF
} AGPUInputStepMode;

typedef enum agpu_device_flags {
    AGPU_DEVICE_FLAGS_NONE = 0,
    AGPU_DEVICE_FLAGS_DEBUG = (1 << 0),
    AGPU_DEVICE_FLAGS_VSYNC = (1 << 1),
    AGPU_DEVICE_FLAGS_GPU_VALIDATION = (1 << 2),
    _AGPU_DEVICE_FLAGS_FORCE_U32 = 0x7FFFFFFF
} agpu_device_flags;

/* Callbacks */
typedef void(*GPULogCallback)(void* userData, VGPULogLevel level, const char* message);

/* Structures */
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
} GPUExtent3D;

typedef struct VGPUColor {
    float r;
    float g;
    float b;
    float a;
} VGPUColor;

typedef struct VGPURect {
    int32_t     x;
    int32_t     y;
    int32_t    width;
    int32_t    height;
} VGPURect;

typedef struct VGPUViewport {
    float       x;
    float       y;
    float       width;
    float       height;
    float       minDepth;
    float       maxDepth;
} VGPUViewport;

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
    uint32_t        maxTextureDimension2D;
    uint32_t        maxTextureDimension3D;
    uint32_t        maxTextureDimensionCube;
    uint32_t        maxTextureArrayLayers;
    uint32_t        maxVertexInputAttributes;
    uint32_t        maxVertexInputBindings;
    uint32_t        maxVertexInputAttributeOffset;
    uint32_t        maxVertexInputBindingStride;
    uint32_t        maxColorAttachments;
    uint32_t        max_uniform_buffer_size;
    uint64_t        min_uniform_buffer_offset_alignment;
    uint32_t        max_storage_buffer_size;
    uint64_t        min_storage_buffer_offset_alignment;
    uint32_t        max_sampler_anisotropy;
    uint32_t        maxViewports;
    uint32_t        maxViewportDimensions[2];
    uint32_t        maxTessellationPatchSize;
    float           pointSizeRange[2];
    float           lineWidthRange[2];
    uint32_t        maxComputeSharedMemorySize;
    uint32_t        maxComputeWorkGroupCount[3];
    uint32_t        maxComputeWorkGroupInvocations;
    uint32_t        maxComputeWorkGroupSize[3];
} vgpu_limits;

typedef struct VGPUDeviceCaps {
    /// Selected GPU vendor PCI id.
    uint32_t vendorId;
    uint32_t deviceId;

    vgpu_limits limits;
    vgpu_features features;
} VGPUDeviceCaps;

typedef struct VGPURenderPassColorAttachment {
    VGPUTexture* texture;
    uint32_t mip_level;
    uint32_t slice;
    vgpu_load_action load_action;
    vgpu_store_action store_action;
    VGPUColor clearColor;
} VGPURenderPassColorAttachment;

typedef struct VGPURenderPassDepthStencilAttachment {
    VGPUTexture* texture;
    vgpu_load_action    depth_load_action;
    vgpu_store_action   depth_store_action;
    float               clear_depth;
    vgpu_load_action    stencil_load_action;
    vgpu_store_action   stencil_store_action;
    uint8_t             clear_stencil;
} VGPURenderPassDepthStencilAttachment;

typedef struct VGPURenderPassDescriptor {
    const char* label;
    VGPURenderPassColorAttachment           colorAttachments[AGPU_MAX_COLOR_ATTACHMENTS];
    VGPURenderPassDepthStencilAttachment    depthStencilAttachment;
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
    vgpu_compare_function compare;
    uint32_t maxAnisotropy;
    GPUBorderColor borderColor;
} AGPUSamplerDescriptor;

typedef struct {
    const char* label;
    uintptr_t nativeHandle;

    AGPUPixelFormat colorFormat;
    uint32_t width;
    uint32_t height;
} VGPUSwapChainInfo;

typedef struct VGpuDeviceDescriptor {
    VGPUBackendType preferredBackend;
    uint32_t flags;
    struct {
        void* (*GetProcAddress)(const char*);
    } gl;
    const VGPUSwapChainInfo* swapchain;
} VGpuDeviceDescriptor;

/* Log functions */
VGPU_API void vgpuSetLogLevel(VGPULogLevel level);
VGPU_API void vgpuSetLogCallback(GPULogCallback callback, void* userData);
VGPU_API void vgpuLog(VGPULogLevel level, const char* format, ...);

/* Backend functions */
VGPU_API VGPUBackendType vgpuGetDefaultPlatformBackend(void);
VGPU_API bool vgpuIsBackendSupported(VGPUBackendType backend);

/* Device functions */
VGPU_API VGPUBool32 vgpuInit(const VGpuDeviceDescriptor* descriptor);
VGPU_API void vgpuShutdown(void);
VGPU_API void vgpuFrameBegin(void);
VGPU_API void vgpuFrameFinish(void);
VGPU_API VGPUBackendType vgpuGetBackend(void);
VGPU_API const VGPUDeviceCaps* vgpuGetCaps(void);
VGPU_API AGPUPixelFormat vgpuGetDefaultDepthFormat(void);
VGPU_API AGPUPixelFormat vgpuGetDefaultDepthStencilFormat(void);

/* Texture */
typedef enum {
    VGPUTextureType_2D = 0,
    VGPUTextureType_3D,
    VGPUTextureType_Cube,
    VGPUTextureType_Force32 = 0x7FFFFFFF
} VGPUTextureType;

typedef enum {
    VGPUTextureUsage_None = 0x00000000,
    VGPUTextureUsage_Sampled = 0x04,
    VGPUTextureUsage_Storage = 0x08,
    VGPUTextureUsage_OutputAttachment = 0x10,
    VGPUTextureUsage_Force32 = 0x7FFFFFFF
} VGPUTextureUsage;
typedef VGPUFlags VGPUTextureUsageFlags;

typedef struct {
    VGPUTextureUsage type;
    VGPUTextureUsageFlags usage;
    GPUExtent3D size;
    AGPUPixelFormat format;
    uint32_t mipLevelCount;
    uint32_t sampleCount;
    /// Pointer to external texture handle
    const void* externalHandle;
    const void* data;
    const char* label;
} VGPUTextureInfo;

VGPU_API VGPUTexture* vgpuTextureCreate(const VGPUTextureInfo* info);
VGPU_API void vgpuTextureDestroy(VGPUTexture* texture);

/* Buffer */
typedef enum VGPUBufferUsage {
    VGPUBufferUsage_None = 0,
    VGPUBufferUsage_Vertex = (1 << 0),
    VGPUBufferUsage_Index = (1 << 1),
    VGPUBufferUsage_Uniform = (1 << 2),
    VGPUBufferUsage_Storage = (1 << 3),
    VGPUBufferUsage_Indirect = (1 << 4),
    VGPUBufferUsage_Dynamic = (1 << 5),
    VGPUBufferUsage_CPUAccessible = (1 << 6),
    VGPUBufferUsage_Force32 = 0x7FFFFFFF
} VGPUBufferUsage;

typedef struct {
    VGPUDeviceSize size;
    VGPUFlags usage;
    const void* data;
    const char* label;
} vgpu_buffer_info;

VGPU_API vgpu_buffer* vgpu_buffer_create(const vgpu_buffer_info* info);
VGPU_API void vgpu_buffer_destroy(vgpu_buffer* buffer);
VGPU_API void vgpu_buffer_sub_data(vgpu_buffer* buffer, VGPUDeviceSize offset, VGPUDeviceSize size, const void* pData);

/* Shader */
typedef struct {
    uint64_t codeSize;
    const void* code;
    const char* source;
    const char* entryPoint;
} VGPUShaderSource;

typedef struct {
    const char* label;
    VGPUShaderSource vertex;
    VGPUShaderSource fragment;
    VGPUShaderSource compute;
} vgpu_shader_info;

VGPU_API vgpu_shader vgpuShaderCreate(const vgpu_shader_info* info);
VGPU_API void vgpuShaderDestroy(vgpu_shader shader);

/* Pipeline */
typedef struct {
    VGPUVertexFormat    format;
    uint32_t            offset;
    uint32_t            bufferIndex;
} VGPUVertexAttributeInfo;

typedef struct {
    uint32_t stride;
    AGPUInputStepMode stepMode;
} VGPUVertexBufferLayoutInfo;

typedef struct {
    VGPUVertexBufferLayoutInfo  layouts[VGPU_MAX_VERTEX_BUFFER_BINDINGS];
    VGPUVertexAttributeInfo     attributes[VGPU_MAX_VERTEX_ATTRIBUTES];
} VGPUVertexInfo;

typedef struct {
    bool depth_write_enabled;
    vgpu_compare_function depth_compare;
} vgpu_depth_stencil_state;

typedef struct vgpu_pipeline_info {
    vgpu_shader                 shader;
    VGPUVertexInfo              vertexInfo;
    VGPUIndexType               indexType;
    agpu_primitive_topology     primitive_topology;
    vgpu_depth_stencil_state    depth_stencil;
    const char* label;
} vgpu_pipeline_info;

VGPU_API agpu_pipeline vgpu_create_pipeline(const vgpu_pipeline_info* info);
VGPU_API void vgpu_destroy_pipeline(agpu_pipeline pipeline);

/* Sampler */
//VGPU_EXPORT AGPUSampler agpuDeviceCreateSampler(GPUDevice device, const AGPUSamplerDescriptor* descriptor);
//VGPU_EXPORT void agpuDeviceDestroySampler(GPUDevice device, AGPUSampler sampler);

/* CommandBuffer */
VGPU_API void vgpuCmdBeginRenderPass(const VGPURenderPassDescriptor* descriptor);
VGPU_API void vgpuCmdEndRenderPass(void);
VGPU_API void vgpuSetPipeline(agpu_pipeline pipeline);
VGPU_API void vgpuCmdSetVertexBuffers(uint32_t slot, vgpu_buffer* buffer, uint64_t offset);
VGPU_API void vgpuCmdSetIndexBuffer(vgpu_buffer* buffer, uint64_t offset);
VGPU_API void vgpu_set_uniform_buffer(uint32_t set, uint32_t binding, vgpu_buffer* buffer);
VGPU_API void vgpu_set_uniform_buffer_data(uint32_t set, uint32_t binding, const void* data, VGPUDeviceSize size);
VGPU_API void vgpuCmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex);
VGPU_API void vgpuCmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex);

/// Get the number of bits per format
VGPU_API uint32_t agpuGetFormatBitsPerPixel(AGPUPixelFormat format);
VGPU_API uint32_t agpuGetFormatBlockSize(AGPUPixelFormat format);

/// Get the format compression ration along the x-axis.
VGPU_API uint32_t agpuGetFormatBlockWidth(AGPUPixelFormat format);
/// Get the format compression ration along the y-axis.
VGPU_API uint32_t agpuGetFormatBlockHeight(AGPUPixelFormat format);
/// Get the format Type.
VGPU_API AGPUPixelFormatType agpuGetFormatType(AGPUPixelFormat format);

/// Check if the format has a depth component.
VGPU_API bool vgpuIsDepthFormat(AGPUPixelFormat format);
/// Check if the format has a stencil component.
VGPU_API bool vgpuIsStencilFrmat(AGPUPixelFormat format);
/// Check if the format has depth or stencil components.
VGPU_API bool vgpuIsDepthStencilFormat(AGPUPixelFormat format);
/// Check if the format is a compressed format.
VGPU_API bool vgpuIsCompressedFormat(AGPUPixelFormat format);
/// Get format string name.
VGPU_API const char* vgpuGetFormatName(AGPUPixelFormat format);
VGPU_API bool vgpuIsSrgbFormat(AGPUPixelFormat format);
VGPU_API AGPUPixelFormat vgpuSrgbToLinearFormat(AGPUPixelFormat format);
VGPU_API AGPUPixelFormat vgpuLinearToSrgbFormat(AGPUPixelFormat format);

VGPU_API uint32_t vgpuGetVertexFormatComponentsCount(VGPUVertexFormat format);
VGPU_API uint32_t vgpuGetVertexFormatComponentSize(VGPUVertexFormat format);
VGPU_API uint32_t vgpuGetVertexFormatSize(VGPUVertexFormat format);

#ifdef __cplusplus
namespace vgpu
{
    enum class BufferUsage : uint32_t
    {
        None = VGPUBufferUsage_None,
        Vertex = VGPUBufferUsage_Vertex,
        Index = VGPUBufferUsage_Index,
        Uniform = VGPUBufferUsage_Uniform,
        Storage = VGPUBufferUsage_Storage,
        Indirect = VGPUBufferUsage_Indirect,
        Dynamic = VGPUBufferUsage_Dynamic,
        CPUAccessible = VGPUBufferUsage_CPUAccessible
    };
}
#endif
