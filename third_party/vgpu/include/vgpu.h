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
    enum {
        VGPU_NUM_INFLIGHT_FRAMES = 2,
        VGPU_MAX_COLOR_ATTACHMENTS = 8,
    };

    typedef enum vgpu_log_level {
        VGPU_LOG_LEVEL_ERROR = 0,
        VGPU_LOG_LEVEL_WARN = 1,
        VGPU_LOG_LEVEL_INFO = 2,
        VGPU_LOG_LEVEL_DEBUG = 3,
        _VGPU_LOG_LEVEL_COUNT,
        _VGPU_LOG_LEVEL_FORCE_U32 = 0x7FFFFFFF
    } vgpu_log_level;

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
    typedef enum vgpu_pixel_format {
        VGPU_PIXEL_FORMAT_UNDEFINED = 0,
        // 8-bit pixel formats
        VGPU_PIXEL_FORMAT_R8_UNORM,
        VGPU_PIXEL_FORMAT_R8_SNORM,
        VGPU_PIXEL_FORMAT_R8_UINT,
        VGPU_PIXEL_FORMAT_R8_SINT,
        // 16-bit pixel formats
        VGPU_PIXEL_FORMAT_R16_UINT,
        VGPU_PIXEL_FORMAT_R16_SINT,
        VGPU_PIXEL_FORMAT_R16_FLOAT,
        VGPU_PIXEL_FORMAT_RG8_UNORM,
        VGPU_PIXEL_FORMAT_RG8_SNORM,
        VGPU_PIXEL_FORMAT_RG8_UINT,
        VGPU_PIXEL_FORMAT_RG8_SINT,
        // 32-bit pixel formats
        VGPU_PIXEL_FORMAT_R32_UINT,
        VGPU_PIXEL_FORMAT_R32_SINT,
        VGPU_PIXEL_FORMAT_R32_FLOAT,
        VGPU_PIXEL_FORMAT_RG16_UINT,
        VGPU_PIXEL_FORMAT_RG16_SINT,
        VGPU_PIXEL_FORMAT_RG16_FLOAT,
        VGPU_PIXEL_FORMAT_RGBA8_UNORM,
        VGPU_PIXEL_FORMAT_RGBA8_UNORM_SRGB,
        VGPU_PIXEL_FORMAT_RGBA8_SNORM,
        VGPU_PIXEL_FORMAT_RGBA8_UINT,
        VGPU_PIXEL_FORMAT_RGBA8_SINT,
        VGPU_PIXEL_FORMAT_BGRA8_UNORM,
        VGPU_PIXEL_FORMAT_BGRA8_UNORM_SRGB,
        // Packed 32-Bit Pixel formats
        VGPU_PIXEL_FORMAT_RGB10A2_UNORM,
        VGPU_PIXEL_FORMAT_RG11B10_FLOAT,
        // 64-Bit Pixel Formats
        VGPU_PIXEL_FORMAT_RG32_UINT,
        VGPU_PIXEL_FORMAT_RG32_SINT,
        VGPU_PIXEL_FORMAT_RG32_FLOAT,
        VGPU_PIXEL_FORMAT_RGBA16_UINT,
        VGPU_PIXEL_FORMAT_RGBA16_SINT,
        VGPU_PIXEL_FORMAT_RGBA16_FLOAT,
        // 128-Bit Pixel Formats
        VGPU_PIXEL_FORMAT_RGBA32_UINT,
        VGPU_PIXEL_FORMAT_RGBA32_SINT,
        VGPU_PIXEL_FORMAT_RGBA32_FLOAT,
        // Depth-stencil
        VGPU_PIXEL_FORMAT_DEPTH32_FLOAT,
        VGPU_PIXEL_FORMAT_DEPTH24_STENCIL8,
        // Compressed BC formats
        VGPU_PIXEL_FORMAT_BC1,
        VGPU_PIXEL_FORMAT_BC1_SRGB,
        VGPU_PIXEL_FORMAT_BC2,
        VGPU_PIXEL_FORMAT_BC2_SRGB,
        VGPU_PIXEL_FORMAT_BC3,
        VGPU_PIXEL_FORMAT_BC3_SRGB,
        VGPU_PIXEL_FORMAT_BC4R_UNORM,
        VGPU_PIXEL_FORMAT_BC4R_SNORM,
        VGPU_PIXEL_FORMAT_BC5RG_UNORM,
        VGPU_PIXEL_FORMAT_BC5RG_SNORM,
        VGPU_PIXEL_FORMAT_BC6HRGB_UFLOAT,
        VGPU_PIXEL_FORMAT_BC6HRGB_SFLOAT,
        VGPU_PIXEL_FORMAT_BC7RGBA_UNORM,
        VGPU_PIXEL_FORMAT_BC7RGBA_UNORM_SRGB,

        _VGPU_PIXEL_FORMAT_COUNT,
        _VGPU_PIXEL_FORMAT_FORCE_U32 = 0x7FFFFFFF
    } vgpu_pixel_format;

    /// Defines pixel format type.
    typedef enum vgpu_pixel_format_type {
        /// Unknown format Type
        VGPU_PIXEL_FORMAT_TYPE_UNKNOWN = 0,
        /// floating-point formats.
        VGPU_PIXEL_FORMAT_TYPE_FLOAT = 1,
        /// Unsigned normalized formats.
        VGPU_PIXEL_FORMAT_TYPE_UNORM = 2,
        /// Unsigned normalized SRGB formats
        VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB = 3,
        /// Signed normalized formats.
        VGPU_PIXEL_FORMAT_TYPE_SNORM = 4,
        /// Unsigned integer formats.
        VGPU_PIXEL_FORMAT_TYPE_UINT = 5,
        /// Signed integer formats.
        VGPU_PIXEL_FORMAT_TYPE_SINT = 6,
        _VGPU_PIXEL_FORMAT_TYPE_FORCE_U32 = 0x7FFFFFFF
    } vgpu_pixel_format_type;

    typedef enum vgpu_pixel_format_aspect {
        VGPU_PIXEL_FORMAT_ASPECT_COLOR = 0,
        VGPU_PIXEL_FORMAT_ASPECT_DEPTH = 1,
        VGPU_PIXEL_FORMAT_ASPECT_STENCIL = 2,
        VGPU_PIXEL_FORMAT_ASPECT_DEPTH_STENCIL = 3,
        _VGPU_PIXEL_FORMAT_ASPECT_FORCE_U32 = 0x7FFFFFFF
    } vgpu_pixel_format_aspect;

    typedef enum vgpu_texture_type {
        VGPU_TEXTURE_TYPE_2D,
        VGPU_TEXTURE_TYPE_3D,
        VGPU_TEXTURE_TYPE_CUBE,
        VGPU_TEXTURE_TYPE_ARRAY,
        _VGPU_TEXTURE_TYPE_FORCE_U32 = 0x7FFFFFFF
    } vgpu_texture_type;

    typedef enum vgpu_texture_usage {
        VGPU_TEXTURE_USAGE_SAMPLED = (1 << 0),
        VGPU_TEXTURE_USAGE_STORAGE = (1 << 1),
        VGPU_TEXTURE_USAGE_RENDER_TARGET = (1 << 2),
        _VGPU_TEXTURE_USAGE_FORCE_U32 = 0x7FFFFFFF
    } vgpu_texture_usage;

    typedef enum vgpu_load_op {
        VGPU_LOAD_OP_CLEAR = 0,
        VGPU_LOAD_OP_LOAD = 1,
        _VGPU_LOAD_OP_FORCE_U32 = 0x7FFFFFFF
    } vgpu_load_op;

    /* Structs */
    typedef struct vgpu_color {
        float r;
        float g;
        float b;
        float a;
    } vgpu_color;

    typedef struct vgpu_texture_info {
        vgpu_texture_type type;
        vgpu_pixel_format format;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t array_layers;
        uint32_t mip_levels;
        uint32_t usage;
        const void* content;
        const char* label;
        uintptr_t external_handle;
    } vgpu_texture_info;

    typedef struct vgpu_attachment_info {
        vgpu_texture texture;
        uint32_t level;
        uint32_t slice;
    } vgpu_attachment_info;

    typedef struct vgpu_color_attachment_info {
        vgpu_texture texture;
        uint32_t level;
        uint32_t slice;
        vgpu_load_op load_op;
        vgpu_color clear_color;
    } vgpu_color_attachment_info;

    typedef struct vgpu_depth_stencil_pass_info {
        vgpu_load_op depth_load_op;
        float clear_depth;
        vgpu_load_op stencil_load_op;
        uint8_t clear_stencil;
    } vgpu_depth_stencil_pass_info;

    typedef struct vgpu_pass_begin_info {
        vgpu_color_attachment_info color_attachments[VGPU_MAX_COLOR_ATTACHMENTS];
        vgpu_depth_stencil_pass_info depth_stencil;
    } vgpu_pass_begin_info;

    typedef struct vgpu_swapchain_info {
        uintptr_t           native_handle;    /**< HWND, ANativeWindow, NSWindow, etc. */
        uint32_t            width;             /**< Width of swapchain. */
        uint32_t            height;            /**< Width of swapchain. */
        vgpu_pixel_format   color_format;
        vgpu_pixel_format   depth_stencil_format;
        bool                is_fullscreen;
    } vgpu_swapchain_info;

    typedef struct vgpu_config {
        bool debug;
        void (*callback)(void* context, const char* message, vgpu_log_level level);
        void* context;
        vgpu_device_preference device_preference;
        vgpu_swapchain_info swapchain;
    } vgpu_config;

    VGPU_API bool vgpu_set_preferred_backend(vgpu_backend_type backend);
    VGPU_API bool vgpu_init(const vgpu_config* config);
    VGPU_API void vgpu_shutdown(void);
    VGPU_API void vgpu_begin_frame(void);
    VGPU_API void vgpu_end_frame(void);

    VGPU_API vgpu_texture vgpu_create_texture(const vgpu_texture_info* info);
    VGPU_API void vgpu_destroy_texture(vgpu_texture texture);
    VGPU_API vgpu_texture_info vgpu_query_texture_info(vgpu_texture texture);

    /* commands */
    VGPU_API vgpu_texture vgpu_get_backbuffer_texture(void);
    VGPU_API void vgpu_begin_pass(const vgpu_pass_begin_info* info);
    VGPU_API void vgpu_end_pass(void);

    /* pixel format helpers */
    VGPU_API uint32_t vgpu_get_format_bytes_per_block(vgpu_pixel_format format);
    VGPU_API uint32_t vgpu_get_format_pixels_per_block(vgpu_pixel_format format);
    VGPU_API bool vgpu_is_depth_format(vgpu_pixel_format format);
    VGPU_API bool vgpu_is_stencil_format(vgpu_pixel_format format);
    VGPU_API bool vgpu_is_depth_stencil_format(vgpu_pixel_format format);
    VGPU_API bool vgpu_is_compressed_format(vgpu_pixel_format format);

#ifdef __cplusplus
}
#endif 

#endif /* _VGPU_H */
