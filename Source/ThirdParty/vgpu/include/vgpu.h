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
typedef struct vgpu_buffer { uint32_t id; } vgpu_buffer;
typedef struct vgpu_framebuffer { uint32_t id; } vgpu_framebuffer;
typedef uint32_t VGPUCommandBuffer;

typedef enum VGPULogLevel {
    VGPULogLevel_Error,
    VGPULogLevel_Warn,
    VGPULogLevel_Info,
    VGPULogLevel_Debug,
    VGPULogLevel_Force32 = 0x7FFFFFFF
} VGPULogLevel;

typedef enum VGPUBackendType {
    VGPUBackendType_Null,
    VGPUBackendType_D3D11,
    VGPUBackendType_D3D12,
    VGPUBackendType_Metal,
    VGPUBackendType_Vulkan,
    VGPUBackendType_OpenGL,
    VGPUBackendType_Count,
    VGPUBackendType_Force32 = 0x7FFFFFFF
} VGPUBackendType;

typedef enum VGPUPixelFormat {
    VGPUTextureFormat_Undefined = 0,
    VGPUTextureFormat_R8UNorm,
    VGPUTextureFormat_R8SNorm,
    VGPUTextureFormat_R8UInt,
    VGPUTextureFormat_R8SInt,
    VGPUTextureFormat_R16UInt,
    VGPUTextureFormat_R16SInt,
    VGPUTextureFormat_R16Float,
    VGPUTextureFormat_RG8UNorm,
    VGPUTextureFormat_RG8SNorm,
    VGPUTextureFormat_RG8UInt,
    VGPUTextureFormat_RG8SInt,
    VGPUTextureFormat_R32Float,
    VGPUTextureFormat_R32UInt,
    VGPUTextureFormat_R32SInt,
    VGPUTextureFormat_RG16UInt,
    VGPUTextureFormat_RG16SInt,
    VGPUTextureFormat_RG16Float,
    VGPUTextureFormat_RGBA8UNorm,
    VGPUTextureFormat_RGBA8UNormSrgb,
    VGPUTextureFormat_RGBA8SNorm,
    VGPUTextureFormat_RGBA8UInt,
    VGPUTextureFormat_RGBA8SInt,
    VGPUTextureFormat_BGRA8UNorm,
    VGPUTextureFormat_BGRA8UNormSrgb,
    VGPUTextureFormat_RGB10A2UNorm,
    VGPUTextureFormat_RG11B10Float,
    VGPUTextureFormat_RG32Float,
    VGPUTextureFormat_RG32UInt,
    VGPUTextureFormat_RG32SInt,
    VGPUTextureFormat_RGBA16UInt,
    VGPUTextureFormat_RGBA16SInt,
    VGPUTextureFormat_RGBA16Float,
    VGPUTextureFormat_RGBA32Float,
    VGPUTextureFormat_RGBA32UInt,
    VGPUTextureFormat_RGBA32SInt,
    VGPUTextureFormat_Depth32Float,
    VGPUTextureFormat_Depth24Plus,
    VGPUTextureFormat_Depth24PlusStencil8,
    VGPUTextureFormat_BC1RGBAUNorm,
    VGPUTextureFormat_BC1RGBAUNormSrgb,
    VGPUTextureFormat_BC2RGBAUNorm,
    VGPUTextureFormat_BC2RGBAUNormSrgb,
    VGPUTextureFormat_BC3RGBAUNorm,
    VGPUTextureFormat_BC3RGBAUNormSrgb,
    VGPUTextureFormat_BC4RUNorm,
    VGPUTextureFormat_BC4RSNorm,
    VGPUTextureFormat_BC5RGUNorm,
    VGPUTextureFormat_BC5RGSNorm,
    VGPUTextureFormat_BC6HRGBUFloat,
    VGPUTextureFormat_BC6HRGBSFloat,
    VGPUTextureFormat_BC7RGBAUNorm,
    VGPUTextureFormat_BC7RGBAUNormSrgb,
    VGPUPixelFormat_Force32 = 0x7FFFFFFF
} VGPUPixelFormat;

typedef enum VGPUPixelFormatAspect {
    VGPUPixelFormatAspect_Color,
    VGPUPixelFormatAspect_Depth,
    VGPUPixelFormatAspect_Stencil,
    VGPUPixelFormatAspect_DepthStencil,
} VGPUPixelFormatAspect;

typedef enum VGPUPixelFormatType {
    VGPUPixelFormatType_Unknown = 0,
    VGPUPixelFormatType_Float,
    VGPUPixelFormatType_UNorm,
    VGPUPixelFormatType_UNormSrgb,
    VGPUPixelFormatType_SNorm,
    VGPUPixelFormatType_SInt,
    VGPUPixelFormatType_UInt,
    VGPUPixelFormatType_Force32 = 0x7FFFFFFF
} VGPUPixelFormatType;

typedef enum VGPUTextureCubemapFace {
    VGPUTextureCubemapFace_PositiveX = 0,
    VGPUTextureCubemapFace_NegativeX = 1,
    VGPUTextureCubemapFace_PositiveY = 2,
    VGPUTextureCubemapFace_NegativeY = 3,
    VGPUTextureCubemapFace_PositiveZ = 4,
    VGPUTextureCubemapFace_NegativeZ = 5,
    VGPUTextureCubemapFace_Force32 = 0x7FFFFFFF
} VGPUTextureCubemapFace;

typedef enum VGPULoadAction {
    VGPULoadAction_Clear = 0,
    VGPULoadAction_Load = 1,
    VGPULoadAction_Force32 = 0x7FFFFFFF
} VGPULoadAction;

typedef enum VGPUPresentMode {
    VGPUPresentMode_Fifo,
    VGPUPresentMode_Immediate,
    VGPUPresentMode_Mailbox,
    VGPUPresentMode_Force32 = 0x7FFFFFFF
} VGPUPresentMode;

typedef void (*vgpu_PFN_log)(void* userData, VGPULogLevel level, const char* message);

/* Structures */
typedef struct vgpu_color {
    float r;
    float g;
    float b;
    float a;
} vgpu_color;

typedef struct VGPUFramebufferAttachment {
    vgpu_texture     texture;
    uint32_t        mipLevel;
    union {
        VGPUTextureCubemapFace face;
        uint32_t slice;
        uint32_t layer;
    };
} VGPUFramebufferAttachment;

typedef struct VGPUFramebufferDescription {
    VGPUFramebufferAttachment colorAttachments[VGPU_MAX_COLOR_ATTACHMENTS];
    VGPUFramebufferAttachment depthStencilAttachment;
    uint32_t width;
    uint32_t height;
    uint32_t layers;
    const char* label;
} VGPUFramebufferDescription;

typedef struct VGPUColorAttachmentAction {
    VGPULoadAction loadAction;
    vgpu_color clear_color;
} VGPUColorAttachmentAction;

typedef struct VGPUDepthStencilAttachmentAction {
    VGPULoadAction depthLoadAction;
    VGPULoadAction stencilLoadAction;
    float clearDepth;
    uint8_t clearStencil;
} VGPUDepthStencilAttachmentAction;

typedef struct VGPURenderPassBeginDescription {
    vgpu_framebuffer framebuffer;
    VGPUColorAttachmentAction colorAttachments[VGPU_MAX_COLOR_ATTACHMENTS];
    VGPUDepthStencilAttachmentAction depthStencilAttachment;
} VGPURenderPassBeginDescription;

typedef struct vgpu_swapchain_info {
    uintptr_t windowHandle;
    uint32_t width;
    uint32_t height;
    VGPUPixelFormat colorFormat;
    VGPUPixelFormat depthStencilFormat;
    VGPUPresentMode presentMode;
    bool fullscreen;
    const char* label;
} vgpu_swapchain_info;

typedef struct VGPUDeviceDescription {
    bool debug;
    vgpu_swapchain_info swapchain;
    void* (*getProcAddress)(const char* funcName);
} VGPUDeviceDescription;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /* Logging/Allocation functions */
    VGPU_API void vgpu_log_set_log_callback(vgpu_PFN_log callback, void* user_data);

    /* Device */
    VGPU_API bool vgpuInit(VGPUBackendType backendType, const VGPUDeviceDescription* desc);
    VGPU_API void vgpuShutdown(void);
    VGPU_API bool vgpuBeginFrame(void);
    VGPU_API void vgpuEndFrame(void);

    /* Texture */
    typedef enum VGPUTextureType {
        VGPU_TEXTURE_TYPE_2D,
        VGPU_TEXTURE_TYPE_3D,
        VGPU_TEXTURE_TYPE_CUBE,
        _VGPU_TEXTURE_FORCE_U32 = 0x7FFFFFFF
    } vgpu_texture_type;

    typedef enum {
        VGPUTextureUsage_None = 0,
        VGPUTextureUsage_Sampled = 1 << 0,
        VGPUTextureUsage_Storage = 1 << 1,
        VGPUTextureUsage_RenderTarget = 1 << 2,
        VGPUTextureUsage_Cubemap = 1 << 3,
        VGPUTextureUsage_Force32 = 0x7FFFFFFF
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
        VGPUPixelFormat format;
        vgpu_texture_type type;
        vgpu_texture_usage_flags usage;
        uint32_t sample_count;
        const void* external_handle;
        const char* label;
    } vgpu_texture_info;

    VGPU_API vgpu_texture vgpu_texture_create(const vgpu_texture_info* info);
    VGPU_API void vgpu_texture_destroy(vgpu_texture texture);
    VGPU_API uint32_t vgpu_texture_get_width(vgpu_texture texture, uint32_t mipLevel);
    VGPU_API uint32_t vgpu_texture_get_height(vgpu_texture texture, uint32_t mipLevel);

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
    VGPU_API void vgpu_buffer_destroy(vgpu_buffer handle);

    /* Framebuffer */
    VGPU_API vgpu_framebuffer vgpu_framebuffer_create(const VGPUFramebufferDescription* desc);
    VGPU_API vgpu_framebuffer vgpu_framebuffer_create_from_window(const vgpu_swapchain_info* info);
    VGPU_API void vgpu_framebuffer_destroy(vgpu_framebuffer framebuffer);
    VGPU_API vgpu_framebuffer vgpu_framebuffer_get_default(void);

    /* CommandBuffer */
    VGPU_API VGPUCommandBuffer vgpuBeginCommandBuffer(const char* name, bool profile);
    VGPU_API void vgpuInsertDebugMarker(VGPUCommandBuffer commandBuffer, const char* name);
    VGPU_API void vgpuPushDebugGroup(VGPUCommandBuffer commandBuffer, const char* name);
    VGPU_API void vgpuPopDebugGroup(VGPUCommandBuffer commandBuffer);
    VGPU_API void vgpuBeginRenderPass(VGPUCommandBuffer commandBuffer, const VGPURenderPassBeginDescription* beginDesc);
    VGPU_API void vgpuEndRenderPass(VGPUCommandBuffer commandBuffer);

    /* Helper methods */
    /// Check if the format is color format.
    VGPU_API bool vgpuIsColorFormat(VGPUPixelFormat format);
    /// Check if the format has a depth component.
    VGPU_API bool vgpuIsDepthFormat(VGPUPixelFormat format);
    /// Check if the format has a stencil component.
    VGPU_API bool vgpuIsStencilFormat(VGPUPixelFormat format);
    /// Check if the format has depth or stencil components.
    VGPU_API bool vgpuIsDepthOrStencilFormat(VGPUPixelFormat format);
    /// Check if the format is a compressed format.
    VGPU_API bool vgpuIsCompressedFormat(VGPUPixelFormat format);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef __cplusplus
namespace vgpu
{
    static constexpr uint32_t kInvalidId = { VGPU_INVALID_ID };
    struct Texture { uint32_t id; bool isValid() const { return id != kInvalidId; } };
    struct Buffer { uint32_t id; bool isValid() const { return id != kInvalidId; } };
    struct Framebuffer { uint32_t id; bool isValid() const { return id != kInvalidId; } };

    static constexpr Texture kInvalidTexture = { kInvalidId };
    static constexpr Buffer kInvalidBuffer = { kInvalidId };
    static constexpr Framebuffer kInvalidFramebuffer = { kInvalidId };

    enum class LogLevel : uint32_t {
        Error = VGPULogLevel_Error,
        Warn = VGPULogLevel_Warn,
        Info = VGPULogLevel_Info,
        Debug = VGPULogLevel_Debug,
    };

    enum class BackendType : uint32_t {
        Null = VGPUBackendType_Null,
        D3D11 = VGPUBackendType_D3D11,
        D3D12 = VGPUBackendType_D3D12,
        Metal = VGPUBackendType_Metal,
        Vulkan = VGPUBackendType_Vulkan,
        OpenGL = VGPUBackendType_OpenGL,
        Count = VGPUBackendType_Count,
    };

    VGPU_API bool Init(BackendType backendType, const VGPUDeviceDescription& desc);
    VGPU_API void Shutdown(void);
    VGPU_API bool BeginFrame(void);
    VGPU_API void EndFrame(void);
}
#endif /* __cplusplus */

#endif /* VGPU_H */
