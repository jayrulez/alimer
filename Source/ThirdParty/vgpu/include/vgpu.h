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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    typedef struct vgpu_device_t* vgpu_device;
    typedef struct VGPUContext_T* VGPUContext;
    typedef struct VGPUTexture_t* VGPUTexture;

    typedef uint32_t VGPUFlags;

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

    typedef enum VGPUTextureType {
        VGPUTextureType_2D,
        VGPUTextureType_3D,
        VGPUTextureType_Cube,
        VGPUTextureType_Force32 = 0x7FFFFFFF
    } VGPUTextureType;

    typedef enum VGPUTextureUsage {
        VGPUTextureUsage_None = 0,
        VGPUTextureUsage_Sampled = 1 << 0,
        VGPUTextureUsage_Storage = 1 << 1,
        VGPUTextureUsage_RenderTarget = 1 << 2,
        VGPUTextureUsage_Cubemap = 1 << 3,
        VGPUTextureUsage_Force32 = 0x7FFFFFFF
    } VGPUTextureUsage;
    typedef VGPUFlags VGPUTextureUsageFlags;

    typedef enum VGPUTextureSampleCount {
        VGPUTextureSampleCount1 = 1,
        VGPUTextureSampleCount2 = 2,
        VGPUTextureSampleCount4 = 4,
        VGPUTextureSampleCount8 = 8,
        VGPUTextureSampleCount16 = 16,
        VGPUTextureSampleCount32 = 32,
        VGPUTextureSampleCount_Force32 = 0x7FFFFFFF
    } VGPUTextureSampleCount;

    typedef enum VGPUPresentMode {
        VGPUPresentMode_Fifo,
        VGPUPresentMode_Immediate,
        VGPUPresentMode_Mailbox,
        VGPUPresentMode_Force32 = 0x7FFFFFFF
    } VGPUPresentMode;

    typedef void (*vgpu_PFN_log)(void* userData, VGPULogLevel level, const char* message);

    typedef struct vgpu_allocation_callbacks {
        void* user_data;
        void* (*allocate_memory)(void* user_data, size_t size);
        void* (*allocate_cleared_memory)(void* user_data, size_t size);
        void (*free_memory)(void* user_data, void* ptr);
    } vgpu_allocation_callbacks;

    typedef struct VGPUTextureDescription {
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t mipLevels;
        uint32_t arrayLayers;
        VGPUPixelFormat format;
        VGPUTextureType type;
        VGPUTextureUsageFlags usage;
        VGPUTextureSampleCount sampleCount;
        const void* externalHandle;
        const char* label;
    } VGPUTextureDescription;

    typedef struct VGPUSwapchainDescription {
        uintptr_t windowHandle;
        VGPUPresentMode presentMode;
        bool fullscreen;
    } VGPUSwapchainDescription;

    typedef struct VGPUContextDescription {

        uint32_t width;
        uint32_t height;
        VGPUPixelFormat colorFormat;
        VGPUPixelFormat depthStencilFormat;
        VGPUTextureSampleCount sampleCount;
        VGPUSwapchainDescription swapchain;
        const char* label;
    } VGPUContextDescription;

    typedef struct vgpu_device_desc {
        bool debug;
        void* (*get_proc_address)(const char* func_name);
    } vgpu_device_desc;

    /* Logging/Allocation functions */
    VGPU_API void vgpu_log_set_log_callback(vgpu_PFN_log callback, void* user_data);
    VGPU_API void vgpu_set_allocation_callbacks(const vgpu_allocation_callbacks* callbacks);

    /* Device */
    VGPU_API vgpu_device vgpu_create_device(VGPUBackendType backendType, const vgpu_device_desc* desc);
    VGPU_API void vgpuDestroyDevice(vgpu_device device);

    /* Texture */
    VGPU_API VGPUTexture vgpuCreateTexture(vgpu_device device, const VGPUTextureDescription* desc);
    VGPU_API void vgpuDestroyTexture(vgpu_device device, VGPUTexture texture);

    /* Context */
    VGPU_API VGPUContext vgpuCreateContext(vgpu_device device, const VGPUContextDescription* desc);
    VGPU_API void vgpuDestroyContext(vgpu_device device, VGPUContext context);
    VGPU_API void vgpuBeginFrame(VGPUContext context);
    VGPU_API void vgpuEndFrame(VGPUContext context);
    VGPU_API void vgpuBeginRenderPass(VGPUContext context);
    VGPU_API void vgpuEndRenderPass(VGPUContext context);

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

#endif /* VGPU_H */
