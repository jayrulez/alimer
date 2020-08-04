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

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#if defined(AGPU_SHARED_LIBRARY)
#   if defined(_WIN32)
#       if defined(AGPU_IMPLEMENTATION)
#           define AGPU_API __declspec(dllexport)
#       else
#           define AGPU_API __declspec(dllimport)
#       endif
#   else  // defined(_WIN32)
#       if defined(AGPU_IMPLEMENTATION)
#           define AGPU_API __attribute__((visibility("default")))
#       else
#           define AGPU_API
#       endif
#   endif  // defined(_WIN32)
#else       // defined(AGPU_SHARED_LIBRARY)
#   define AGPU_API
#endif  // defined(AGPU_SHARED_LIBRARY)

#if defined(_WIN32)
#   define AGPU_CALL __cdecl
#else
#   define AGPU_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif
    /* Handles */
    typedef struct agpu_buffer_t* agpu_buffer;
    typedef struct agpu_texture_t* agpu_texture;

    typedef enum agpu_log_level {
        AGPU_LOG_LEVEL_INFO = 0,
        AGPU_LOG_LEVEL_WARN = 1,
        AGPU_LOG_LEVEL_ERROR = 2,
        _AGPU_LOG_LEVEL_ERROR_FORCE_U32 = 0x7FFFFFFF
    } agpu_log_level;

    typedef enum agpu_backend {
        AGPU_BACKEND_DEFAULT = 0,
        AGPU_BACKEND_OPENGL = 1,
        AGPU_BACKEND_D3D11 = 2,
        _AGPU_BACKEND_FORCE_U32 = 0x7FFFFFFF
    } agpu_backend;

    typedef enum agpu_buffer_usage {
        AGPU_BUFFER_USAGE_VERTEX = (1 << 0),
        AGPU_BUFFER_USAGE_INDEX = (1 << 1),
        AGPU_BUFFER_USAGE_UNIFORM = (1 << 2),
        AGPU_BUFFER_USAGE_STORAGE = (1 << 3),
        AGPU_BUFFER_USAGE_INDIRECT = (1 << 4),
        _AGPU_BUFFER_USAGE_FORCE_U32 = 0x7FFFFFFF
    } agpu_buffer_usage;


    /// Defines pixel format.
    typedef enum vgpu_texture_format {
        AGPU_TEXTURE_FORMAT_UNDEFINED = 0,
        // 8-bit pixel formats
        AGPU_TEXTURE_FORMAT_R8UNorm,
        AGPU_TEXTURE_FORMAT_R8SNorm,
        AGPU_TEXTURE_FORMAT_R8UInt,
        AGPU_TEXTURE_FORMAT_R8SInt,

        // 16-bit pixel formats
        AGPU_TEXTURE_FORMAT_R16UNorm,
        AGPU_TEXTURE_FORMAT_R16SNorm,
        AGPU_TEXTURE_FORMAT_R16UInt,
        AGPU_TEXTURE_FORMAT_R16SInt,
        AGPU_TEXTURE_FORMAT_R16Float,
        AGPU_TEXTURE_FORMAT_RG8UNorm,
        AGPU_TEXTURE_FORMAT_RG8SNorm,
        AGPU_TEXTURE_FORMAT_RG8UInt,
        AGPU_TEXTURE_FORMAT_RG8SInt,

        // 32-bit pixel formats
        AGPU_TEXTURE_FORMAT_R32UInt,
        AGPU_TEXTURE_FORMAT_R32SInt,
        AGPU_TEXTURE_FORMAT_R32Float,
        AGPU_TEXTURE_FORMAT_RG16UInt,
        AGPU_TEXTURE_FORMAT_RG16SInt,
        AGPU_TEXTURE_FORMAT_RG16Float,

        AGPU_TEXTURE_FORMAT_RGBA8,
        AGPU_TEXTURE_FORMAT_RGBA8_SRGB,
        AGPU_TEXTURE_FORMAT_RGBA8SNorm,
        AGPU_TEXTURE_FORMAT_RGBA8UInt,
        AGPU_TEXTURE_FORMAT_RGBA8SInt,
        AGPU_TEXTURE_FORMAT_BGRA8,
        AGPU_TEXTURE_FORMAT_BGRA8_SRGB,

        // Packed 32-Bit Pixel formats
        AGPU_TEXTURE_FORMAT_RGB10A2,
        AGPU_TEXTURE_FORMAT_RG11B10F,

        // 64-Bit Pixel Formats
        AGPU_TEXTURE_FORMAT_RG32UInt,
        AGPU_TEXTURE_FORMAT_RG32SInt,
        AGPU_TEXTURE_FORMAT_RG32Float,
        AGPU_TEXTURE_FORMAT_RGBA16UInt,
        AGPU_TEXTURE_FORMAT_RGBA16SInt,
        AGPU_TEXTURE_FORMAT_RGBA16Float,

        // 128-Bit Pixel Formats
        AGPU_TEXTURE_FORMAT_RGBA32UInt,
        AGPU_TEXTURE_FORMAT_RGBA32SInt,
        AGPU_TEXTURE_FORMAT_RGBA32Float,

        // Depth-stencil
        AGPU_TEXTURE_FORMAT_D16,
        AGPU_TEXTURE_FORMAT_D32F,
        AGPU_TEXTURE_FORMAT_D24_PLUS,
        AGPU_TEXTURE_FORMAT_D24S8,

        // Compressed BC formats
        AGPU_TEXTURE_FORMAT_BC1,
        AGPU_TEXTURE_FORMAT_BC1_SRGB,
        AGPU_TEXTURE_FORMAT_BC2,
        AGPU_TEXTURE_FORMAT_BC2_SRGB,
        AGPU_TEXTURE_FORMAT_BC3,
        AGPU_TEXTURE_FORMAT_BC3_SRGB,
        AGPU_TEXTURE_FORMAT_BC4,
        AGPU_TEXTURE_FORMAT_BC4S,
        AGPU_TEXTURE_FORMAT_BC5,
        AGPU_TEXTURE_FORMAT_BC5S,
        AGPU_TEXTURE_FORMAT_BC6H_UFLOAT,
        AGPU_TEXTURE_FORMAT_BC6H_SFLOAT,
        AGPU_TEXTURE_FORMAT_BC7,
        AGPU_TEXTURE_FORMAT_BC7_SRGB,

        _AGPU_TEXTURE_FORMAT_COUNT,
        _AGPU_TEXTURE_FORMAT_FORCE_U32 = 0x7FFFFFFF
    } agpu_texture_format;

    typedef enum agpu_sample_count {
        AGPU_SAMPLE_COUNT_1 = 1,
        AGPU_SAMPLE_COUNT_2 = 2,
        AGPU_SAMPLE_COUNT_4 = 4,
        AGPU_SAMPLE_COUNT_8 = 8,
        AGPU_SAMPLE_COUNT_16 = 16,
        _AGPU_SAMPLE_COUNT_FORCE_U32 = 0x7FFFFFFF
    } agpu_sample_count;

    typedef struct agpu_color {
        float r;
        float g;
        float b;
        float a;
    } agpu_color;

    typedef struct agpu_clear_depth_stencil_value {
        float depth;
        uint8_t stencil;
    } agpu_clear_depth_stencil_value;

    typedef struct agpu_buffer_info {
        uint64_t size;
        uint32_t usage;
        void* data;
        const char* label;
    } agpu_buffer_info;

    typedef struct agpu_swapchain_info {
        void*                               native_handle;
        uint32_t                            width;
        uint32_t                            height;
        uint32_t                            image_count;
        agpu_sample_count                   sample_count;
        agpu_texture_format                 color_format;
        agpu_color                          color_clear_value;
        agpu_texture_format                 depth_stencil_format;
        agpu_clear_depth_stencil_value      depth_stencil_clear_value;
    } agpu_swapchain_info;

    typedef struct agpu_config {
        bool debug;
        const agpu_swapchain_info* swapchain;
    } agpu_config;

    /* Callbacks */
    typedef void(AGPU_CALL* agpu_log_callback)(void* user_data, agpu_log_level level, const char* message);
    typedef void*(AGPU_CALL* agpu_gl_get_proc_address)(const char* name);

    /* Log functions */
    AGPU_API void agpu_set_log_callback(agpu_log_callback callback, void* user_data);
    AGPU_API void agpu_log_info(const char* format, ...);
    AGPU_API void agpu_log_warn(const char* format, ...);
    AGPU_API void agpu_log_error(const char* format, ...);

    /* GL only (for now) */
    AGPU_API void agpu_set_gl_get_proc_address(agpu_gl_get_proc_address callback);

    AGPU_API bool agpu_set_preferred_backend(agpu_backend backend);
    AGPU_API bool agpu_init(const char* app_name, const agpu_config* config);
    AGPU_API void agpu_shutdown(void);
    AGPU_API void agpu_frame_begin(void);
    AGPU_API void agpu_frame_end(void);

    AGPU_API agpu_buffer agpu_create_buffer(const agpu_buffer_info* info);
    AGPU_API void agpu_destroy_buffer(agpu_buffer buffer);

#ifdef __cplusplus
}
#endif
