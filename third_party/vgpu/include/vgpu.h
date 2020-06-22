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
typedef struct vgpu_framebuffer_t* vgpu_framebuffer;

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

/* Framebuffer */
typedef struct vgpu_framebuffer_info {
    uint32_t width;
    uint32_t height;
    uint32_t layers;
    const char* label;
} vgpu_framebuffer_info;

VGPU_API vgpu_framebuffer vgpu_framebuffer_create(const vgpu_framebuffer_info* info);
VGPU_API void vgpu_framebuffer_destroy(vgpu_framebuffer framebuffer);
VGPU_API vgpu_framebuffer vgpu_framebuffer_get_default(void);

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
    static constexpr uint8_t kMaxCommandLists = 16u;

    using CommandList = uint8_t;

    /* Handles */
    struct ContextHandle { uint32_t id; bool isValid() const { return id != kInvalidHandle; } };
    struct TextureHandle { uint32_t id; bool isValid() const { return id != kInvalidHandle; } };
    struct BufferHandle { uint32_t id; bool isValid() const { return id != kInvalidHandle; } };

    const ContextHandle kInvalidContext = { kInvalidHandle };
    const TextureHandle kInvalidTexture = { kInvalidHandle };
    const BufferHandle kInvalidBuffer = { kInvalidHandle };

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

    enum class ResourceMemoryUsage : uint32_t {
        Unknown = 0,
        GpuOnly = 1,
        CpuOnly = 2,
        CpuToGpu = 3,
        GpuToCpu = 4
    };

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
        Count
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

    /* Texture */
    enum class TextureType : uint32_t {
        Type2D,
        Type3D,
        TypeCube
    };

    enum class TextureUsage : uint32_t {
        None = 0,
        Sampled = (1 << 0),
        Storage = (1 << 1),
        RenderTarget = (1 << 2)
    };
    MAKE_ENUM_FLAG(uint32_t, TextureUsage);

    enum BufferUsage : uint32_t {
        None = 0,
        Vertex = (1 << 0),
        Index = (1 << 1),
        Uniform = (1 << 2),
        Storage = (1 << 3),
        Indirect = (1 << 3),
    };
    MAKE_ENUM_FLAG(uint32_t, BufferUsage);

    enum class LoadOp : uint32_t {
        Clear = 0,
        Load = 1,
        Discard = 2,
    };

    /* Structs */
    typedef struct Color {
        float r;
        float g;
        float b;
        float a;
    } Color;

    struct Caps {
        BackendType backendType;

        uint32_t vendorId;
        uint32_t deviceId;

        struct {
            bool independentBlend;
            bool computeShader;
            bool tessellationShader;
            bool logicOp;
            bool multiViewport;
            bool indexTypeUint32;
            bool multiDrawIndirect;
            bool fillModeNonSolid;
            bool samplerAnisotropy;
            bool textureCompressionETC2;
            bool textureCompressionASTC_LDR;
            bool textureCompressionBC;
            bool textureCubeArray;
            bool raytracing;
        } features;

        struct {
            uint32_t        maxVertexAttributes;
            uint32_t        maxVertexBindings;
            uint32_t        maxVertexAttributeOffset;
            uint32_t        maxVertexBindingStride;
            uint32_t        maxTextureDimension2D;
            uint32_t        maxTextureDimension3D;
            uint32_t        maxTextureDimensionCube;
            uint32_t        maxTextureArrayLayers;
            uint32_t        maxColorAttachments;
            uint32_t        maxUniformBufferSize;
            uint32_t        minUniformBufferOffsetAlignment;
            uint32_t        maxStorageBufferSize;
            uint32_t        minStorageBufferOffsetAlignment;
            float           maxSamplerAnisotropy;
            uint32_t        maxViewports;
            uint32_t        maxViewportWidth;
            uint32_t        maxViewportHeight;
            uint32_t        maxTessellationPatchSize;
            float           pointSizeRangeMin;
            float           pointSizeRangeMax;
            float           lineWidthRangeMin;
            float           lineWidthRangeMax;
            uint32_t        maxComputeSharedMemorySize;
            uint32_t        maxComputeWorkGroupCountX;
            uint32_t        maxComputeWorkGroupCountY;
            uint32_t        maxComputeWorkGroupCountZ;
            uint32_t        maxComputeWorkGroupInvocations;
            uint32_t        maxComputeWorkGroupSizeX;
            uint32_t        maxComputeWorkGroupSizeY;
            uint32_t        maxComputeWorkGroupSizeZ;
        } limits;
    };

    struct TextureDesc {
        TextureType textureType = TextureType::Type2D;
        PixelFormat format = PixelFormat::RGBA8Unorm;
        TextureUsage usage = TextureUsage::Sampled;
        uint32_t width = 1;
        uint32_t height = 1;
        uint32_t depth = 1;
        uint32_t mipLevels = 1;
        uint32_t arrayLayers = 1;
        uint32_t sampleCount = 1;
        const void* externalHandle = nullptr;
        const char* label = nullptr;
    };

    struct BufferDesc {
        uint32_t size;
        ResourceMemoryUsage memoryUsage;
        BufferUsage usage;
        uint32_t stride;
        const char* label;
    };

    struct RenderPassColorAttachmentDesc {
        TextureHandle texture;
        uint32_t level;
        uint32_t slice;
        LoadOp loadOp;
        Color clearColor;
    };

    struct RenderPassDesc {
        const char* label;
        RenderPassColorAttachmentDesc colorAttachments[kMaxColorAttachments];
        //WGPURenderPassDepthStencilAttachmentDescriptor const* depthStencilAttachment;
    };

    struct PresentationParameters {
        uint32_t width = 0;
        uint32_t height = 0;
        PixelFormat colorFormat = PixelFormat::BGRA8Unorm;
        PixelFormat depthStencilFormat = PixelFormat::Depth32Float;
        void* windowHandle = nullptr;
        bool vsync = true;
        bool HDR = false;
    };

    /* API calls */
    _VGPU_API_DECL void setPreferredBackend(BackendType backendType);
    _VGPU_API_DECL bool init(const PresentationParameters& presentationParameters, InitFlags flags = InitFlags::None);
    _VGPU_API_DECL void shutdown(void);
    _VGPU_API_DECL const Caps* getCaps();
    _VGPU_API_DECL bool BeginFrame(void);
    _VGPU_API_DECL void EndFrame(void);

    /* Texture methods */
    _VGPU_API_DECL TextureHandle CreateTexture(const TextureDesc& desc);
    _VGPU_API_DECL void DestroyTexture(TextureHandle texture);
    _VGPU_API_DECL uint32_t textureGetWidth(TextureHandle texture, uint32_t mipLevel);
    _VGPU_API_DECL uint32_t textureGetHeight(TextureHandle texture, uint32_t mipLevel);

    /* Buffer methods */
    _VGPU_API_DECL BufferHandle CreateBuffer(const BufferDesc& desc, const void* pInitData = nullptr);
    _VGPU_API_DECL void DestroyBuffer(BufferHandle buffer);

    /* CommandBuffer methods */
    _VGPU_API_DECL void InsertDebugMarker(const char* name, CommandList commandList = 0);
    _VGPU_API_DECL void PushDebugGroup(const char* name, CommandList commandList = 0);
    _VGPU_API_DECL void PopDebugGroup(CommandList commandList = 0);
    _VGPU_API_DECL void BeginRenderPass(const RenderPassDesc* desc, CommandList commandList = 0);
    _VGPU_API_DECL void EndRenderPass(CommandList commandList = 0);

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
