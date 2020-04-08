//
// Copyright (c) 2019-2020 Amer Koleci and contributors.
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

#include "vgpu_backend.h"
#include <stdio.h> 
#include <stdarg.h>
#include <string.h>

#if defined(__ANDROID__)
#   include <android/log.h>
#elif TARGET_OS_IOS || TARGET_OS_TV
#   include <sys/syslog.h>
#elif TARGET_OS_MAC || defined(__linux__)
#   include <unistd.h>
#elif defined(_WIN32)
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   include <Windows.h>
#   include <strsafe.h>
#elif defined(__EMSCRIPTEN__)
#   include <emscripten.h>
#endif

#define VGPU_MAX_LOG_MESSAGE (4096)
static const char* vgpu_log_priority_prefixes[VGPU_LOG_LEVEL_COUNT] = {
    NULL,
    "VERBOSE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "CRITICAL"
};

static void vgpu_default_log_callback(void* user_data, const char* message, vgpu_log_level level);
static vgpu_log_callback s_log_function = vgpu_default_log_callback;
static void* s_log_user_data = NULL;

void vgpu_get_log_callback_function(vgpu_log_callback* callback, void** user_data) {
    if (callback) {
        *callback = s_log_function;
    }
    if (user_data) {
        *user_data = s_log_user_data;
    }
}

void vgpu_set_log_callback_function(vgpu_log_callback callback, void* user_data) {
    s_log_function = callback;
    s_log_user_data = user_data;
}

void vgpu_default_log_callback(void* user_data, const char* message, vgpu_log_level level) {
#if defined(_WIN32)
    size_t length = strlen(vgpu_log_priority_prefixes[level]) + 2 + strlen(message) + 1 + 1 + 1;
    char* output = _vgpu_alloca(char, length);
    snprintf(output, length, "%s: %s\r\n", vgpu_log_priority_prefixes[level], message);

    const int buffer_size = MultiByteToWideChar(CP_UTF8, 0, output, (int)length, nullptr, 0);
    if (buffer_size == 0)
        return;

    WCHAR* buffer = _vgpu_alloca(WCHAR, buffer_size);
    if (MultiByteToWideChar(CP_UTF8, 0, message, -1, buffer, buffer_size) == 0)
        return;

    OutputDebugStringW(buffer);

#   if !defined(NDEBUG) || defined(DEBUG) || defined(_DEBUG)
    HANDLE handle;
    switch (level)
    {
    case VGPU_LOG_LEVEL_ERROR:
    case VGPU_LOG_LEVEL_WARN:
        handle = GetStdHandle(STD_ERROR_HANDLE);
        break;
    default:
        handle = GetStdHandle(STD_OUTPUT_HANDLE);
        break;
    }

    WriteConsoleW(handle, buffer, (DWORD)wcslen(buffer), NULL, NULL);
#   endif

#endif
}

void vgpu_log(vgpu_log_level level, const char* message) {
    if (s_log_function) {
        s_log_function(s_log_user_data, message, level);
    }
}

void vgpu_log_message_v(vgpu_log_level level, const char* format, va_list args) {
    if (s_log_function) {
        char message[VGPU_MAX_LOG_MESSAGE];
        vsnprintf(message, VGPU_MAX_LOG_MESSAGE, format, args);
        vgpu_log(level, message);
    }
}

void vgpu_log_format(vgpu_log_level level, const char* format, ...) {
    if (s_log_function) {
        va_list args;
        va_start(args, format);
        vgpu_log_message_v(level, format, args);
        va_end(args);
    }
}

void vgpu_log_error(const char* context, const char* message) {
    if (s_log_function) {
        vgpu_log_format(VGPU_LOG_LEVEL_ERROR, "[%s]: %s", context, message);
    }
}

vgpu_backend vgpu_get_default_platform_backend(void) {
#if defined(_WIN32)
    if (vgpu_is_backend_supported(VGPU_BACKEND_DIRECT3D12)) {
        return VGPU_BACKEND_DIRECT3D12;
    }

    if (vgpu_is_backend_supported(VGPU_BACKEND_VULKAN)) {
        return VGPU_BACKEND_VULKAN;
    }

    if (vgpu_is_backend_supported(VGPU_BACKEND_DIRECT3D11)) {
        return VGPU_BACKEND_DIRECT3D11;
    }

    if (vgpu_is_backend_supported(VGPU_BACKEND_OPENGL)) {
        return VGPU_BACKEND_OPENGL;
    }

    return VGPU_BACKEND_NULL;
#elif defined(__linux__) || defined(__ANDROID__)
    return VGPU_BACKEND_OPENGL;
#elif defined(__APPLE__)
    return VGPU_BACKEND_VULKAN;
#else
    return VGPU_BACKEND_OPENGL;
#endif
}

bool vgpu_is_backend_supported(vgpu_backend backend) {
    if (backend == VGPU_BACKEND_DEFAULT) {
        backend = vgpu_get_default_platform_backend();
    }

    switch (backend)
    {
    case VGPU_BACKEND_NULL:
        return true;
#if defined(VGPU_VK_BACKEND)
    case VGPU_BACKEND_VULKAN:
        return vgpu_vk_supported();
#endif
#if defined(VGPU_D3D12_BACKEND)
    case VGPU_BACKEND_DIRECT3D12:
        return vgpu_d3d12_supported();
#endif /* defined(VGPU_D3D12_BACKEND) */

#if defined(VGPU_BACKEND_GL)
    case VGPU_BACKEND_OPENGL:
        return agpu_gl_supported();
#endif // defined(AGPU_BACKEND_GL)

    default:
        return false;
    }
}

static vgpu_renderer* s_renderer = NULL;

bool vgpu_init(const char* app_name, const vgpu_desc* desc) {
    if (s_renderer) {
        return true;
    }

    assert(desc);

    vgpu_backend backend = desc->preferred_backend;
    if (backend == VGPU_BACKEND_DEFAULT) {
        backend = vgpu_get_default_platform_backend();
    }

    vgpu_renderer* renderer = NULL;
    switch (backend)
    {
    case VGPU_BACKEND_NULL:
        break;
#if defined(VGPU_VK_BACKEND)
    case VGPU_BACKEND_VULKAN:
        renderer = vgpuCreateVkBackend();
        if (!renderer) {
            vgpuLogError("Vulkan backend is not supported");
        }
        break;
#endif

#if defined(VGPU_D3D12_BACKEND)
    case VGPU_BACKEND_DIRECT3D12:
        renderer = vgpu_create_d3d12_backend();
        if (!renderer) {
            vgpu_log(VGPU_LOG_LEVEL_ERROR, "Direct3D12 backend is not supported");
        }
        break;
#endif

#if defined(VGPU_D3D11_BACKEND)
    case VGPU_BACKEND_DIRECT3D11:
        renderer = vgpu_create_d3d11_backend();
        if (!renderer) {
            vgpuLogError("Direct3D11 backend is not supported");
        }
        break;
#endif

#if defined(VGPU_BACKEND_GL)
    case VGPU_BACKEND_OPENGL:
        renderer = vgpuCreateOpenGLBackend();
        if (!renderer) {
            vgpuLogError("OpenGL backend is not supported");
        }
        break;
#endif

    default:
        break;
    }

    if (renderer != nullptr) {
        s_renderer = renderer;
        return renderer->init(app_name, desc);
    }

    return true;
}

void vgpu_shutdown(void) {
    if (s_renderer != nullptr) {
        s_renderer->shutdown();
    }
}

void vgpu_wait_idle(void) {
    s_renderer->wait_idle();
}

void vgpu_begin_frame(void) {
    s_renderer->begin_frame();
}

void vgpu_end_frame(void) {
    s_renderer->end_frame();
}

vgpu_backend vgpu_get_backend(void) {
    return s_renderer->get_backend();
}

vgpu_features vgpu_query_features(void) {
    return s_renderer->query_features();
}

vgpu_limits vgpu_query_limits(void) {
    return s_renderer->query_limits();
}

/*vgpu_context* vgpu_create_context(vgpu_device* device, const vgpu_context_desc* desc) {
    return device->create_context(device->renderer, desc);
}

void vgpu_destroy_context(vgpu_device* device, vgpu_context* context) {
    device->destroy_context(device->renderer, context);
}

void vgpu_set_context(vgpu_device* device, vgpu_context* context) {
    device->set_context(device->renderer, context);
}*/

/*vgpu_buffer* vgpu_create_buffer(vgpu_device* device, const vgpu_buffer_desc* desc) {
    return device->create_buffer(device->renderer, desc);
}

void vgpu_destroy_buffer(vgpu_device* device, vgpu_buffer* buffer) {
    device->destroy_buffer(device->renderer, buffer);
}

vgpu_texture* vgpu_create_texture(vgpu_device* device, const vgpu_texture_desc* desc) {
    return device->create_texture(device->renderer, desc);
}


void vgpu_destroy_texture(vgpu_device* device, vgpu_texture* texture) {
    device->destroy_texture(device->renderer, texture);
}
*/

/* Format helpers */
typedef struct vgpu_pixel_format_desc
{
    vgpu_pixel_format       format;
    const char* name;
    vgpu_pixel_format_type  type;
    uint8_t                 bits_per_pixel;
    struct
    {
        uint8_t             block_width;
        uint8_t             block_height;
        uint8_t             block_size;
        uint8_t             min_block_x;
        uint8_t             min_block_y;
    } compression;
    struct
    {
        uint8_t             depth;
        uint8_t             stencil;
        uint8_t             red;
        uint8_t             green;
        uint8_t             blue;
        uint8_t             alpha;
    } bits;
} vgpu_pixel_format_desc;

const vgpu_pixel_format_desc FormatDesc[] =
{
    // format                                   name,                   type                                bpp         compression             bits
    { VGPU_PIXEL_FORMAT_UNDEFINED,              "Undefined",            VGPU_PIXEL_FORMAT_TYPE_UNKNOWN,     0,          {0, 0, 0, 0, 0},        {0, 0, 0, 0, 0, 0}},
    // 8-bit pixel formats
    { VGPU_PIXEL_FORMAT_R8_UNORM,               "R8",                   VGPU_PIXEL_FORMAT_TYPE_UNORM,       8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_R8_SNORM,               "R8S",                  VGPU_PIXEL_FORMAT_TYPE_SNORM,       8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_R8_UINT,                "R8U",                  VGPU_PIXEL_FORMAT_TYPE_UINT,        8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_R8_SINT,                "R8I",                  VGPU_PIXEL_FORMAT_TYPE_SINT,        8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    // 16-bit pixel formats
    { VGPU_PIXEL_FORMAT_R16_UNORM,              "R16",                  VGPU_PIXEL_FORMAT_TYPE_UNORM,       16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_R16_SNORM,              "R16S",                 VGPU_PIXEL_FORMAT_TYPE_SNORM,       16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_R16_UINT,               "R16U",                 VGPU_PIXEL_FORMAT_TYPE_UINT,        16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_R16_SINT,               "R16I",                 VGPU_PIXEL_FORMAT_TYPE_SINT,        16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_R16_FLOAT,              "R16F",                 VGPU_PIXEL_FORMAT_TYPE_FLOAT,       16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_RG8_UNORM,              "RG8",                  VGPU_PIXEL_FORMAT_TYPE_UNORM,       16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    { VGPU_PIXEL_FORMAT_RG8_SNORM,              "RG8S",                 VGPU_PIXEL_FORMAT_TYPE_SNORM,       16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    { VGPU_PIXEL_FORMAT_RG8_UINT,               "RG8U",                 VGPU_PIXEL_FORMAT_TYPE_UINT,        16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    { VGPU_PIXEL_FORMAT_RG8_SINT,               "RG8I",                 VGPU_PIXEL_FORMAT_TYPE_SINT,        16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    // 32-bit pixel formats
    { VGPU_PIXEL_FORMAT_R32_UINT,               "R32U",                 VGPU_PIXEL_FORMAT_TYPE_UINT,        32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_R32_SINT,               "R32I",                 VGPU_PIXEL_FORMAT_TYPE_SINT,        32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_R32_FLOAT,              "R32F",                 VGPU_PIXEL_FORMAT_TYPE_FLOAT,       32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_RG16_UNORM,             "RG16",                 VGPU_PIXEL_FORMAT_TYPE_UNORM,       32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { VGPU_PIXEL_FORMAT_RG16_SNORM,             "RG16S",                VGPU_PIXEL_FORMAT_TYPE_SNORM,       32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { VGPU_PIXEL_FORMAT_RG16_UINT,              "RG16U",                VGPU_PIXEL_FORMAT_TYPE_UINT,        32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { VGPU_PIXEL_FORMAT_RG16_SINT,              "RG16I",                VGPU_PIXEL_FORMAT_TYPE_SINT,        32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { VGPU_PIXEL_FORMAT_RG16_FLOAT,             "RG16F",                VGPU_PIXEL_FORMAT_TYPE_FLOAT,       32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { VGPU_PIXEL_FORMAT_RGBA8_UNORM,            "RGBA8",                VGPU_PIXEL_FORMAT_TYPE_UNORM,       32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPU_PIXEL_FORMAT_RGBA8_UNORM_SRGB,       "RGBA8Srgb",            VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,  32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPU_PIXEL_FORMAT_RGBA8_SNORM,            "RGBA8S",               VGPU_PIXEL_FORMAT_TYPE_SNORM,       32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPU_PIXEL_FORMAT_RGBA8_UINT,             "RGBA8U",               VGPU_PIXEL_FORMAT_TYPE_UINT,        32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPU_PIXEL_FORMAT_RGBA8_SINT,             "RGBA8I",               VGPU_PIXEL_FORMAT_TYPE_SINT,        32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPU_PIXEL_FORMAT_BGRA8_UNORM,            "BGRA8",                VGPU_PIXEL_FORMAT_TYPE_UNORM,       32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPU_PIXEL_FORMAT_BGRA8_UNORM_SRGB,       "BGRA8Srgb",            VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,  32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    // Packed 32-Bit Pixel formats
    { VGPU_PIXEL_FORMAT_RGB10A2_UNORM,          "RGB10A2",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       32,         {1, 1, 4, 1, 1},        {0, 0, 10, 10, 10, 2}},
    { VGPU_PIXEL_FORMAT_RG11B10_FLOAT,          "RG11B10F",             VGPU_PIXEL_FORMAT_TYPE_FLOAT,       32,         {1, 1, 4, 1, 1},        {0, 0, 11, 11, 10, 0}},
    // 64-Bit Pixel Formats
    { VGPU_PIXEL_FORMAT_RG32_UINT,              "RG32U",                VGPU_PIXEL_FORMAT_TYPE_UINT,        64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
    { VGPU_PIXEL_FORMAT_RG32_SINT,              "RG3I",                 VGPU_PIXEL_FORMAT_TYPE_SINT,        64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
    { VGPU_PIXEL_FORMAT_RG32_FLOAT,             "RG3F",                 VGPU_PIXEL_FORMAT_TYPE_FLOAT,       64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
    { VGPU_PIXEL_FORMAT_RGBA16_UNORM,           "RGBA16",               VGPU_PIXEL_FORMAT_TYPE_UNORM,       64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { VGPU_PIXEL_FORMAT_RGBA16_SNORM,           "RGBA16S",              VGPU_PIXEL_FORMAT_TYPE_SNORM,       64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { VGPU_PIXEL_FORMAT_RGBA16_UINT,            "RGBA16U",              VGPU_PIXEL_FORMAT_TYPE_UINT,        64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { VGPU_PIXEL_FORMAT_RGBA16_SINT,            "RGBA16S",              VGPU_PIXEL_FORMAT_TYPE_SINT,        64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { VGPU_PIXEL_FORMAT_RGBA16_FLOAT,           "RGBA16F",              VGPU_PIXEL_FORMAT_TYPE_FLOAT,       64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    // 128-Bit Pixel Formats
    { VGPU_PIXEL_FORMAT_RGBA32_UINT,            "RGBA32U",              VGPU_PIXEL_FORMAT_TYPE_UINT,        128,        {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},
    { VGPU_PIXEL_FORMAT_RGBA32_SINT,            "RGBA32S",              VGPU_PIXEL_FORMAT_TYPE_SINT,        128,        {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},
    { VGPU_PIXEL_FORMAT_RGBA32_FLOAT,           "RGBA32F",              VGPU_PIXEL_FORMAT_TYPE_FLOAT,       128,        {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},
    // Depth-stencil
    { VGPU_PIXEL_FORMAT_D16_UNORM,              "Depth16UNorm",         VGPU_PIXEL_FORMAT_TYPE_UNORM,       16,         {1, 1, 2, 1, 1},        {16, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_D32_FLOAT,              "Depth32Float",         VGPU_PIXEL_FORMAT_TYPE_FLOAT,       32,         {1, 1, 4, 1, 1},        {32, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_D24_UNORM_S8_UINT,      "Depth24UNormStencil8", VGPU_PIXEL_FORMAT_TYPE_UNORM,       32,         {1, 1, 4, 1, 1},        {24, 8, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_D32_FLOAT_S8_UINT,      "Depth32FloatStencil8", VGPU_PIXEL_FORMAT_TYPE_FLOAT,       32,         {1, 1, 4, 1, 1},        {32, 8, 0, 0, 0, 0}},
    // Compressed formats
    { VGPU_PIXEL_FORMAT_BC1_RGBA_UNORM,         "BC1RGBAUNorm",         VGPU_PIXEL_FORMAT_TYPE_UNORM,       4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_BC1_RGBA_UNORM_SRGB,    "BC1RGBAUNormSrgb",     VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,  4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_BC2_RGBA_UNORM,         "BC2",                  VGPU_PIXEL_FORMAT_TYPE_UNORM,       8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_BC2_RGBA_UNORM_SRGB,    "BC2Srgb",              VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,  8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_BC3_RGBA_UNORM,         "BC3",                  VGPU_PIXEL_FORMAT_TYPE_UNORM,       8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_BC3_RGBA_UNORM_SRGB,    "BC3Srgb",              VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,  8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_BC4_R_UNORM,            "BC4",                  VGPU_PIXEL_FORMAT_TYPE_UNORM,       4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_BC4_R_SNORM,            "BC4S",                 VGPU_PIXEL_FORMAT_TYPE_SNORM,       4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_BC5_RG_UNORM,           "BC5",                  VGPU_PIXEL_FORMAT_TYPE_UNORM,       8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_BC5_RG_SNORM,           "BC5S",                 VGPU_PIXEL_FORMAT_TYPE_SNORM,       8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_BC6H_RGB_FLOAT,         "BC6HS16",              VGPU_PIXEL_FORMAT_TYPE_FLOAT,       8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_BC6H_RGB_UFLOAT,        "BC6HU16",              VGPU_PIXEL_FORMAT_TYPE_FLOAT,       8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_BC7_RGBA_UNORM,         "BC7",                  VGPU_PIXEL_FORMAT_TYPE_UNORM,       8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_BC7_RGBA_UNORM_SRGB,    "BC7Srgb",              VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,  8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    // Compressed PVRTC Pixel Formats
    { VGPU_PIXEL_FORMAT_PVRTC_RGB2,             "PVRTC_RGB2",           VGPU_PIXEL_FORMAT_TYPE_UNORM,       2,          {8, 4, 8, 2, 2},        {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_PVRTC_RGBA2,            "PVRTC_RGBA2",          VGPU_PIXEL_FORMAT_TYPE_UNORM,       2,          {8, 4, 8, 2, 2},        {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_PVRTC_RGB4,             "PVRTC_RGB4",           VGPU_PIXEL_FORMAT_TYPE_UNORM,       4,          {4, 4, 8, 2, 2},        {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_PVRTC_RGBA4,            "PVRTC_RGBA4",          VGPU_PIXEL_FORMAT_TYPE_UNORM,       4,          {4, 4, 8, 2, 2},        {0, 0, 0, 0, 0, 0}},
    // Compressed ETC Pixel Formats
    { VGPU_PIXEL_FORMAT_ETC2_RGB8,              "ETC2_RGB8",            VGPU_PIXEL_FORMAT_TYPE_UNORM,       4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_ETC2_RGB8_SRGB,         "ETC2_RGB8_SRGB",       VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,  4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_ETC2_RGB8A1,            "ETC2_RGB8A1",          VGPU_PIXEL_FORMAT_TYPE_UNORM,       4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_ETC2_RGB8A1_SRGB,       "ETC2_RGB8A1_SRGB",     VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,  4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    // Compressed ASTC Pixel Formats
    { VGPU_PIXEL_FORMAT_ASTC4x4,                "ASTC4x4",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_ASTC5x5,                "ASTC5x5",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       6,          {5, 5, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_ASTC6x6,                "ASTC6x6",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       4,          {6, 6, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_ASTC8x5,                "ASTC8x5",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       4,          {8, 5, 16, 1, 1},       {0, 0, 0, 0, 0, 0} },
    { VGPU_PIXEL_FORMAT_ASTC8x6,                "ASTC8x6",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       3,          {8, 6, 16, 1, 1},       {0, 0, 0, 0, 0, 0} },
    { VGPU_PIXEL_FORMAT_ASTC8x8,                "ASTC8x8",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       3,          {8, 8, 16, 1, 1},       {0, 0, 0, 0, 0, 0} },
    { VGPU_PIXEL_FORMAT_ASTC10x10,              "ASTC10x10",            VGPU_PIXEL_FORMAT_TYPE_UNORM,       3,          {10, 10, 16, 1, 1},     {0, 0, 0, 0, 0, 0} },
    { VGPU_PIXEL_FORMAT_ASTC12x12,              "ASTC12x12",            VGPU_PIXEL_FORMAT_TYPE_UNORM,       3,          {12, 12, 16, 1, 1},     {0, 0, 0, 0, 0, 0} },
};

bool vgpu_is_depth_format(vgpu_pixel_format format) {
    assert(FormatDesc[format].format == format);
    return FormatDesc[format].bits.depth > 0;
}

bool vgpu_is_stencil_format(vgpu_pixel_format format) {
    VGPU_ASSERT(FormatDesc[format].format == format);
    return FormatDesc[format].bits.stencil > 0;
}

bool vgpu_is_depth_stencil_format(vgpu_pixel_format format) {
    return vgpu_is_depth_format(format) || vgpu_is_stencil_format(format);
}
