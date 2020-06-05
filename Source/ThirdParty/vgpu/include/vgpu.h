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
    typedef struct vgpu_texture_t* vgpu_texture;

    typedef enum vgpu_log_level {
        VGPU_LOG_LEVEL_ERROR,
        VGPU_LOG_LEVEL_WARN,
        VGPU_LOG_LEVEL_INFO,
        VGPU_LOG_LEVEL_DEBUG,
        _VGPU_LOG_LEVEL_FORCE_U32 = 0x7FFFFFFF
    } vgpu_log_level;

    typedef enum vgpu_backend_type {
        VGPU_BACKEND_TYPE_NULL,
        VGPU_BACKEND_TYPE_D3D11,
        VGPU_BACKEND_TYPE_D3D12,
        VGPU_BACKEND_TYPE_METAL,
        VGPU_BACKEND_TYPE_VULKAN,
        VGPU_BACKEND_TYPE_OPENGL,
        VGPU_BACKEND_TYPE_COUNT,
        _VGPU_BACKEND_TYPE_FORCE_U32 = 0x7FFFFFFF
    } vgpu_backend_type;

    typedef enum vgpu_texture_format {
        VGPU_TEXTURE_FORMAT_UNDEFINED = 0,
        VGPU_TEXTURE_FORMAT_R8_UNORM,
        VGPU_TEXTURE_FORMAT_R8_SNORM,
        VGPU_TEXTURE_FORMAT_R8_UINT,
        VGPU_TEXTURE_FORMAT_R8_SINT,
        VGPU_TEXTURE_FORMAT_R16_UINT,
        VGPU_TEXTURE_FORMAT_R16_SINT,
        VGPU_TEXTURE_FORMAT_R16_FLOAT,
        VGPU_TEXTURE_FORMAT_RG8_UNORM,
        VGPU_TEXTURE_FORMAT_RG8_SNORM,
        VGPU_TEXTURE_FORMAT_RG8_UINT,
        VGPU_TEXTURE_FORMAT_RG8_SINT,
        VGPU_TEXTURE_FORMAT_R32_FLOAT,
        VGPU_TEXTURE_FORMAT_R32_UINT,
        VGPU_TEXTURE_FORMAT_R32_SINT,
        VGPU_TEXTURE_FORMAT_RG16_UINT,
        VGPU_TEXTURE_FORMAT_RG16_SINT,
        VGPU_TEXTURE_FORMAT_RG16_FLOAT,
        VGPU_TEXTURE_FORMAT_RGBA8_UNORM,
        VGPU_TEXTURE_FORMAT_RGBA8_UNORM_SRGB,
        VGPU_TEXTURE_FORMAT_RGBA8_SNORM,
        VGPU_TEXTURE_FORMAT_RGBA8_UINT,
        VGPU_TEXTURE_FORMAT_RGBA8_SINT,
        VGPU_TEXTURE_FORMAT_BGRA8_UNORM,
        VGPU_TEXTURE_FORMAT_BGRA8_UNORM_SRGB,
        VGPU_TEXTURE_FORMAT_RGB10A2_UNORM,
        VGPU_TEXTURE_FORMAT_RG11B10_FLOAT,
        VGPU_TEXTURE_FORMAT_RG32_FLOAT,
        VGPU_TEXTURE_FORMAT_RG32_UINT,
        VGPU_TEXTURE_FORMAT_RG32_SINT,
        VGPU_TEXTURE_FORMAT_RGBA16_UINT,
        VGPU_TEXTURE_FORMAT_RGBA16_SINT,
        VGPU_TEXTURE_FORMAT_RGBA16_FLOAT,
        VGPU_TEXTURE_FORMAT_RGBA32_FLOAT,
        VGPU_TEXTURE_FORMAT_RGBA32_UINT,
        VGPU_TEXTURE_FORMAT_RGBA32_SINT,
        VGPU_TEXTURE_FORMAT_DEPTH32_FLOAT,
        VGPU_TEXTURE_FORMAT_DEPTH24_PLUS,
        VGPU_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8,
        VGPU_TEXTURE_FORMAT_BC1RGBA_UNORM,
        VGPU_TEXTURE_FORMAT_BC1RGBA_UNORM_SRGB,
        VGPU_TEXTURE_FORMAT_BC2RGBA_UNORM,
        VGPU_TEXTURE_FORMAT_BC2RGBA_UNORM_SRGB,
        VGPU_TEXTURE_FORMAT_BC3RGBA_UNORM,
        VGPU_TEXTURE_FORMAT_BC3RGBA_UNORM_SRGB,
        VGPU_TEXTURE_FORMAT_BC4R_UNORM,
        VGPU_TEXTURE_FORMAT_BC4R_SNORM,
        VGPU_TEXTURE_FORMAT_BC5RG_UNORM,
        VGPU_TEXTURE_FORMAT_BC5RG_SNORM,
        VGPU_TEXTURE_FORMAT_BC6HRGB_UFLOAT,
        VGPU_TEXTURE_FORMAT_BC6HRGB_SFLOAT,
        VGPU_TEXTURE_FORMAT_BC7RGBA_UNORM,
        VGPU_TEXTURE_FORMAT_BC7RGBA_UNORM_SRGB,
        _VGPU_TEXTURE_FORMAT_FORCE_U32 = 0x7FFFFFFF
    } vgpu_texture_format;

    typedef enum vgpu_texture_type {
        VGPU_TEXTURE_TYPE_2D,
        VGPU_TEXTURE_TYPE_3D,
        VGPU_TEXTURE_TYPE_CUBE,
        VGPU_TEXTURE_TYPE_ARRAY,
        _VGPU_TEXTURE_TYPE_FORCE_U32 = 0x7FFFFFFF
    } vgpu_texture_type;

    typedef enum vgpu_sample_count {
        VGPU_SAMPLE_COUNT_1 = 1,
        VGPU_SAMPLE_COUNT_2 = 2,
        VGPU_SAMPLE_COUNT_4 = 4,
        VGPU_SAMPLE_COUNT_8 = 8,
        VGPU_SAMPLE_COUNT_16 = 16,
        VGPU_SAMPLE_COUNT_32 = 32,
        _VGPU_SAMPLE_COUNT_FORCE_U32 = 0x7FFFFFFF
    } vgpu_sample_count;

    typedef enum vgpu_present_interval {
        VGPU_PRESENT_INTERVAL_DEFAULT,
        VGPU_PRESENT_INTERVAL_ONE,
        VGPU_PRESENT_INTERVAL_TWO,
        VGPU_PRESENT_INTERVAL_IMMEDIATE,
        _VGPU_PRESENT_INTERVAL_FORCE_U32 = 0x7FFFFFFF
    } vgpu_present_interval;

    typedef void (*vgpu_PFN_log)(void* user_data, vgpu_log_level level, const char* message);

    typedef struct vgpu_allocation_callbacks {
        void* user_data;
        void* (*allocate_memory)(void* user_data, size_t size);
        void (*free_memory)(void* user_data, void* ptr);
    } vgpu_allocation_callbacks;

    typedef struct vgpu_texture_desc {
        vgpu_texture_type type;
        vgpu_texture_format format;
        uint32_t width;
        uint32_t height;
        uint32_t depth_or_layers;
        uint32_t mip_levels;
        vgpu_sample_count sample_count;
        const void* external_handle;
        const char* label;
    } vgpu_texture_desc;

    typedef struct vgpu_swapchain_desc {
        uintptr_t window_handle;
        uint32_t width;
        uint32_t height;
        vgpu_texture_format color_format;
        vgpu_texture_format depth_stencil_format;
        vgpu_sample_count sample_count;
        vgpu_present_interval present_interval;
        bool fullscreen;
    } vgpu_swapchain_desc;

    typedef struct vgpu_device_desc {
        bool debug;
        vgpu_swapchain_desc swapchain;
        void* (*get_proc_address)(const char* func_name);
    } vgpu_device_desc;

    /* Logging/Allocation functions */
    VGPU_API void vgpu_log_set_log_callback(vgpu_PFN_log callback, void* user_data);
    VGPU_API void vgpu_set_allocation_callbacks(const vgpu_allocation_callbacks* callbacks);

    /* Device */
    VGPU_API vgpu_device vgpu_create_device(vgpu_backend_type backend_type, const vgpu_device_desc* desc);
    VGPU_API void vgpu_destroy_device(vgpu_device device);
    VGPU_API void vgpu_begin_frame(vgpu_device device);
    VGPU_API void vgpu_present_frame(vgpu_device device);

    /* Texture */
    VGPU_API vgpu_texture vgpu_create_texture(vgpu_device device, const vgpu_texture_desc* desc);
    VGPU_API void vgpu_destroy_texture(vgpu_device device, vgpu_texture texture);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* VGPU_H */
