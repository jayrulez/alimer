//
// Copyright (c) 2020 Amer Koleci and contributors.
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

#ifndef _VGPU_H
#define _VGPU_H

#if defined(VGPU_SHARED_LIBRARY)
#   if defined(_WIN32)
#       if defined(VGPU_IMPLEMENTATION)
#           define VGPU_API __declspec(dllexport)
#       else
#           define VGPU_API __declspec(dllimport)
#       endif
#   else  // defined(_WIN32)
#       if defined(VGPU_IMPLEMENTATION)
#           define VGPU_API __attribute__((visibility("default")))
#       else
#           define VGPU_API
#       endif
#   endif  // defined(_WIN32)
#else       // defined(VGPU_SHARED_LIBRARY)
#   define VGPU_API
#endif  // defined(VGPU_SHARED_LIBRARY)

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
    /* Handles*/
    typedef struct vgpu_texture_t* vgpu_texture;

    /* Enums */
    typedef enum vgpu_backend_type {
        VGPU_BACKEND_TYPE_DEFAULT,
        VGPU_BACKEND_TYPE_NULL,
        VGPU_BACKEND_TYPE_D3D12,
        VGPU_BACKEND_TYPE_VULKAN,
        _VGPU_BACKEND_TYPE_FORCE_U32 = 0x7FFFFFFF
    } vgpu_backend_type;

    typedef enum vgpu_device_preference {
        VGPU_DEVICE_PREFERENCE_HIGH_PERFORMANCE = 0,
        VGPU_DEVICE_PREFERENCE_LOW_POWER = 1,
        VGPU_DEVICE_PREFERENCE_DONT_CARE = 2,
        _VGPU_DEVICE_PREFERENCE_FORCE_U32 = 0x7FFFFFFF
    } vgpu_device_preference;

    /// Defines pixel format.
    typedef enum vgpu_texture_format {
        VGPU_TEXTURE_FORMAT_UNDEFINED = 0,
        // 8-bit pixel formats
        VGPU_TEXTURE_FORMAT_R8_UNORM,
        VGPU_TEXTURE_FORMAT_R8_SNORM,
        VGPU_TEXTURE_FORMAT_R8_UINT,
        VGPU_TEXTURE_FORMAT_R8_SINT,
        // 16-bit pixel formats
        VGPU_TEXTURE_FORMAT_R16_UINT,
        VGPU_TEXTURE_FORMAT_R16_SINT,
        VGPU_TEXTURE_FORMAT_R16_FLOAT,
        VGPU_TEXTURE_FORMAT_RG8_UNORM,
        VGPU_TEXTURE_FORMAT_RG8_SNORM,
        VGPU_TEXTURE_FORMAT_RG8_UINT,
        VGPU_TEXTURE_FORMAT_RG8_SINT,
        // 32-bit pixel formats
        VGPU_TEXTURE_FORMAT_R32_UINT,
        VGPU_TEXTURE_FORMAT_R32_SINT,
        VGPU_TEXTURE_FORMAT_R32_FLOAT,
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

        // Packed 32-Bit Pixel formats
        VGPU_TEXTURE_FORMAT_RGB10A2_UNORM,
        VGPU_TEXTURE_FORMAT_RG11B10_FLOAT,

        // 64-Bit Pixel Formats
        VGPU_TEXTURE_FORMAT_RG32_UINT,
        VGPU_TEXTURE_FORMAT_RG32_SINT,
        VGPU_TEXTURE_FORMAT_RG32_FLOAT,
        VGPU_TEXTURE_FORMAT_RGBA16_UINT,
        VGPU_TEXTURE_FORMAT_RGBA16_SINT,
        VGPU_TEXTURE_FORMAT_RGBA16_FLOAT,

        // 128-Bit Pixel Formats
        VGPU_TEXTURE_FORMAT_RGBA32_UINT,
        VGPU_TEXTURE_FORMAT_RGBA32_SINT,
        VGPU_TEXTURE_FORMAT_RGBA32_FLOAT,

        // Depth-stencil
        VGPU_TEXTURE_FORMAT_DEPTH32_FLOAT,
        VGPU_TEXTURE_FORMAT_DEPTH24_PLUS,
        VGPU_TEXTURE_FORMAT_DEPTH24_STENCIL8,

        // Compressed BC formats
        VGPU_TEXTURE_FORMAT_BC1,
        VGPU_TEXTURE_FORMAT_BC1_SRGB,
        VGPU_TEXTURE_FORMAT_BC2,
        VGPU_TEXTURE_FORMAT_BC2_SRGB,
        VGPU_TEXTURE_FORMAT_BC3,
        VGPU_TEXTURE_FORMAT_BC3_SRGB,
        VGPU_TEXTURE_FORMAT_BC4,
        VGPU_TEXTURE_FORMAT_BC4S,
        VGPU_TEXTURE_FORMAT_BC5,
        VGPU_TEXTURE_FORMAT_BC5S,
        VGPU_TEXTURE_FORMAT_BC6H_UFLOAT,
        VGPU_TEXTURE_FORMAT_BC6H_SFLOAT,
        VGPU_TEXTURE_FORMAT_BC7,
        VGPU_TEXTURE_FORMAT_BC7_SRGB,

        _VGPU_TEXTURE_FORMAT_COUNT,
        _VGPU_TEXTURE_FORMAT_FORCE_U32 = 0x7FFFFFFF
    } vgpu_texture_format;

    typedef enum vgpu_texture_type {
        VGPU_TEXTURE_TYPE_2D,
        VGPU_TEXTURE_TYPE_3D,
        VGPU_TEXTURE_TYPE_CUBE,
        VGPU_TEXTURE_TYPE_ARRAY,
        _VGPU_TEXTURE_TYPE_FORCE_U32 = 0x7FFFFFFF
    } vgpu_texture_type;

    /* Structs */
    typedef struct vgpu_texture_info {
        vgpu_texture_type type;
        vgpu_texture_format format;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t layers;
        uint32_t mipmaps;
        uint32_t usage;
        const char* label;
        uintptr_t external_handle;
    } vgpu_texture_info;

    typedef struct vgpu_swapchain_info {
        uintptr_t native_handle;    /**< HWND, ANativeWindow, NSWindow, etc. */
        uint32_t width;             /**< Width of swapchain. */
        uint32_t height;            /**< Width of swapchain. */
        vgpu_texture_format color_format;
        vgpu_texture_format depth_stencil_format;
        bool is_fullscreen;
    } vgpu_swapchain_info;

    typedef struct vgpu_config {
        vgpu_backend_type preferred_backend;
        bool debug;
        void* (*get_proc_ddress)(const char*);
        void (*callback)(void* context, const char* message, int level);
        void* context;
        vgpu_device_preference device_preference;
        vgpu_swapchain_info swapchain;
    } vgpu_config;

    VGPU_API bool vgpu_init(const vgpu_config* config);
    VGPU_API void vgpu_shutdown(void);
    VGPU_API void vgpu_begin_frame(void);
    VGPU_API void vgpu_end_frame(void);

    VGPU_API vgpu_texture vgpu_texture_create(const vgpu_texture_info* info);
    VGPU_API void vgpu_texture_destroy(vgpu_texture texture);

#ifdef __cplusplus
}
#endif 

#endif /* _VGPU_H */
