//
// Copyright (c) 2019 Amer Koleci.
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
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#include <stdarg.h>

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

static void vgpu_default_log_callback(void* user_data, vgpu_log_level level, const char* message);
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

void vgpu_default_log_callback(void* user_data, vgpu_log_level level, const char* message) {
#if defined(_WIN32)
    size_t length = strlen(vgpu_log_priority_prefixes[level]) + 2 + strlen(message) + 1 + 1 + 1;
    char* output = VGPU_ALLOCA(char, length);
    snprintf(output, length, "%s: %s\r\n", vgpu_log_priority_prefixes[level], message);

    const int buffer_size = MultiByteToWideChar(CP_UTF8, 0, output, (int)length, nullptr, 0);
    if (buffer_size == 0)
        return;

    WCHAR* buffer = VGPU_ALLOCA(WCHAR, buffer_size);
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
        s_log_function(s_log_user_data, level, message);
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

void vgpu_log_error(const char* message) {
    vgpu_log(VGPU_LOG_LEVEL_ERROR, message);
}

void vgpu_log_error_format(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vgpu_log_message_v(VGPU_LOG_LEVEL_ERROR, format, args);
    va_end(args);
}

vgpu_backend vgpu_get_default_platform_backend(void) {
#if defined(_WIN32) || defined(_WIN64)
    if (vgpu_is_backend_supported(VGPU_BACKEND_D3D12)) {
        return VGPU_BACKEND_D3D12;
    }

    if (vgpu_is_backend_supported(VGPU_BACKEND_VULKAN)) {
        return VGPU_BACKEND_VULKAN;
    }

    if (vgpu_is_backend_supported(VGPU_BACKEND_D3D11)) {
        return VGPU_BACKEND_D3D11;
    }

    if (vgpu_is_backend_supported(VGPU_BACKEND_OPENGL)) {
        return VGPU_BACKEND_OPENGL;
    }

    return VGPU_BACKEND_NULL;
#elif defined(__linux__) || defined(__ANDROID__)
    return VGPUBackendType_OpenGL;
#elif defined(__APPLE__)
    return VGPUBackendType_Vulkan;
#else
    return VGPUBackendType_OpenGL;
#endif
}

bool vgpu_is_backend_supported(vgpu_backend backend) {
    if (backend == VGPU_BACKEND_DEFAULT || backend == VGPU_BACKEND_FORCE_U32) {
        backend = vgpu_get_default_platform_backend();
    }

    switch (backend)
    {
    case VGPU_BACKEND_NULL:
        return true;
#if defined(VGPU_VULKAN)
    case VGPU_BACKEND_VULKAN:
        return vgpu_vk_supported();
#endif
#if defined(VGPU_D3D12)
    case VGPU_BACKEND_DIRECT3D12:
        return vgpu_d3d12_supported();
#endif /* defined(VGPU_D3D12_BACKEND) */

#if defined(VGPU_D3D11)
    case VGPU_BACKEND_D3D11:
        return vgpu_d3d11_supported();
#endif

#if defined(VGPU_BACKEND_GL)
    case VGPU_BACKEND_OPENGL:
        return agpu_gl_supported();
#endif // defined(AGPU_BACKEND_GL)

    default:
        return false;
    }
}

static VGPUDevice s_gpuDevice = nullptr;

bool vgpu_init(const vgpu_config* config)
{
    if (s_gpuDevice != nullptr) {
        return true;
    }

    vgpu_backend backend = config->preferred_backend;
    if (backend == VGPU_BACKEND_DEFAULT || backend == VGPU_BACKEND_FORCE_U32) {
        backend = vgpu_get_default_platform_backend();
    }

    VGPUDevice device = nullptr;
    switch (backend)
    {
    case VGPU_BACKEND_NULL:
        break;

#if defined(VGPU_VULKAN)
    case VGPU_BACKEND_VULKAN:
        device = vk_create_device();
        break;
#endif

#if defined(VGPU_D3D11)
    case VGPU_BACKEND_D3D11:
        device = d3d11_create_device();
        break;
#endif
    }

    if (device == nullptr) {
        return false;
    }

    s_gpuDevice = device;
    if (!s_gpuDevice->init(device, config)) {
        s_gpuDevice = nullptr;
        return false;
    }

    return true;
}

void vgpu_shutdown(void) {
    if (s_gpuDevice == nullptr) {
        return;
    }

    s_gpuDevice->destroy(s_gpuDevice);
}

vgpu_backend vgpu_query_backend(void) {
    VGPU_ASSERT(s_gpuDevice);
    return s_gpuDevice->getBackend();
}

VGPUFeatures vgpu_query_features(void) {
    VGPU_ASSERT(s_gpuDevice);
    return s_gpuDevice->getFeatures(s_gpuDevice->renderer);
}

VGPULimits vgpu_query_limits(void)
{
    VGPU_ASSERT(s_gpuDevice);
    return s_gpuDevice->getLimits(s_gpuDevice->renderer);
}

VGPURenderPass vgpu_get_default_render_pass(void)
{
    VGPU_ASSERT(s_gpuDevice);
    return s_gpuDevice->get_default_render_pass(s_gpuDevice->renderer);
}

vgpu_pixel_format vgpu_get_default_depth_format(void) {
    VGPU_ASSERT(s_gpuDevice);
    return s_gpuDevice->get_default_depth_format(s_gpuDevice->renderer);
}

vgpu_pixel_format vgpu_get_default_depth_stencil_format(void) {
    VGPU_ASSERT(s_gpuDevice);
    return s_gpuDevice->get_default_depth_stencil_format(s_gpuDevice->renderer);
}

void vgpu_wait_idle(void) {
    s_gpuDevice->wait_idle(s_gpuDevice->renderer);
}

void vgpu_begin_frame(void) {
    s_gpuDevice->begin_frame(s_gpuDevice->renderer);
}

void vgpu_end_frame(void) {
    s_gpuDevice->end_frame(s_gpuDevice->renderer);
}

/* Buffer */
VGPUBuffer vgpu_create_buffer(const VGPUBufferDescriptor* descriptor)
{
    return s_gpuDevice->bufferCreate(s_gpuDevice->renderer, descriptor);
}

void vgpu_destroy_buffer(VGPUBuffer buffer)
{
    s_gpuDevice->bufferDestroy(s_gpuDevice->renderer, buffer);
}

/* Texture */
static vgpu_texture_desc _vgpu_texture_desc_defaults(const vgpu_texture_desc* desc) {
    vgpu_texture_desc def = *desc;
    def.type = _vgpu_def(desc->type, VGPU_TEXTURE_TYPE_2D);
    def.width = _vgpu_def(desc->width, 1);
    def.height = _vgpu_def(desc->height, 1);
    def.depth = _vgpu_def(desc->depth, 1);
    def.format = _vgpu_def(def.format, VGPUTextureFormat_RGBA8Unorm);
    def.mip_levels = _vgpu_def(def.mip_levels, 1);
    def.sample_count = _vgpu_def(def.sample_count, 1);
    return def;
}

VGPUTexture vgpu_create_texture(const vgpu_texture_desc* desc)
{
    vgpu_texture_desc desc_def = _vgpu_texture_desc_defaults(desc);
    return s_gpuDevice->create_texture(s_gpuDevice->renderer, &desc_def);
}

void vgpu_destroy_texture(VGPUTexture texture)
{
    s_gpuDevice->destroy_texture(s_gpuDevice->renderer, texture);
}

vgpu_texture_desc vgpu_query_texture_desc(VGPUTexture texture)
{
    return s_gpuDevice->query_texture_desc(texture);
}

uint32_t vgpu_get_texture_width(VGPUTexture texture, uint32_t mip_level)
{
    return _vgpu_max(1u, vgpu_query_texture_desc(texture).width >> mip_level);
}

uint32_t vgpu_get_texture_height(VGPUTexture texture, uint32_t mip_level)
{
    return _vgpu_max(1u, vgpu_query_texture_desc(texture).height >> mip_level);
}

/* Sampler */
static vgpu_sampler_desc _vgpu_vgpu_sampler_desc_defaults(const vgpu_sampler_desc* desc) {
    vgpu_sampler_desc def = *desc;
    def.address_mode_u = _vgpu_def(desc->address_mode_u, VGPU_ADDRESS_MODE_CLAMP_TO_EDGE);
    def.address_mode_v = _vgpu_def(desc->address_mode_v, VGPU_ADDRESS_MODE_CLAMP_TO_EDGE);
    def.address_mode_w = _vgpu_def(desc->address_mode_w, VGPU_ADDRESS_MODE_CLAMP_TO_EDGE);
    def.mag_filter = _vgpu_def(desc->mag_filter, VGPU_FILTER_NEAREST);
    def.min_filter = _vgpu_def(desc->min_filter, VGPU_FILTER_NEAREST);
    def.mipmap_filter = _vgpu_def(desc->mipmap_filter, VGPU_FILTER_NEAREST);
    def.lod_min_clamp = _vgpu_def_flt(def.lod_min_clamp, 0.0f);
    def.lod_max_clamp = _vgpu_def_flt(def.lod_max_clamp, FLT_MAX);
    def.compare = _vgpu_def(def.compare, VGPU_COMPARE_FUNCTION_UNDEFINED);
    def.max_anisotropy = _vgpu_def(def.max_anisotropy, 1);
    def.border_color = _vgpu_def(def.border_color, VGPU_BORDER_COLOR_TRANSPARENT_BLACK);
    return def;
}

vgpu_sampler vgpu_create_sampler(const vgpu_sampler_desc* desc)
{
    vgpu_sampler_desc desc_def = _vgpu_vgpu_sampler_desc_defaults(desc);
    return s_gpuDevice->samplerCreate(s_gpuDevice->renderer, &desc_def);
}

void vgpu_destroy_sampler(vgpu_sampler sampler)
{
    s_gpuDevice->samplerDestroy(s_gpuDevice->renderer, sampler);
}

VGPURenderPass vgpuCreateRenderPass(const VGPURenderPassDescriptor* descriptor)
{
    VGPU_ASSERT(descriptor);
    return s_gpuDevice->renderPassCreate(s_gpuDevice->renderer, descriptor);
}

void vgpuDestroyRenderPass(VGPURenderPass renderPass) {
    s_gpuDevice->renderPassDestroy(s_gpuDevice->renderer, renderPass);
}

void vgpuRenderPassGetExtent(VGPURenderPass renderPass, uint32_t* width, uint32_t* height) {
    s_gpuDevice->renderPassGetExtent(s_gpuDevice->renderer, renderPass, width, height);
}

void vgpu_render_pass_set_color_clear_value(VGPURenderPass render_pass, uint32_t attachment_index, const float colorRGBA[4])
{
    s_gpuDevice->render_pass_set_color_clear_value(render_pass, attachment_index, colorRGBA);
}

void vgpu_render_pass_set_depth_stencil_clear_value(VGPURenderPass render_pass, float depth, uint8_t stencil)
{
    s_gpuDevice->render_pass_set_depth_stencil_clear_value(render_pass, depth, stencil);
}


/* Commands */
VGPU_EXPORT void vgpu_cmd_begin_render_pass(VGPURenderPass renderPass)
{
    VGPU_ASSERT(renderPass);
    s_gpuDevice->cmdBeginRenderPass(s_gpuDevice->renderer, renderPass);
}

void vgpu_cmd_end_render_pass(void) {
    s_gpuDevice->cmdEndRenderPass(s_gpuDevice->renderer);
}

/* Pixel Format */
typedef struct vgpu_pixel_format_desc
{
    vgpu_pixel_format format;
    const char* name;
    vgpu_pixel_format_type type;
    uint8_t bitsPerPixel;
    struct
    {
        uint8_t blockWidth;
        uint8_t blockHeight;
        uint8_t blockSize;
        uint8_t minBlockX;
        uint8_t minBlockY;
    } compression;

    struct
    {
        uint8_t depth;
        uint8_t stencil;
        uint8_t red;
        uint8_t green;
        uint8_t blue;
        uint8_t alpha;
    } bits;
} vgpu_pixel_format_desc;

const vgpu_pixel_format_desc FormatDesc[] =
{
    // format                                   name,                   type                                bpp         compression             bits
    { VGPU_PIXELFORMAT_UNDEFINED,              "Undefined",            VGPU_PIXEL_FORMAT_TYPE_UNKNOWN,     0,          {0, 0, 0, 0, 0},        {0, 0, 0, 0, 0, 0}},
    // 8-bit pixel formats
    { VGPUTextureFormat_R8Unorm,               "R8Unorm",               VGPU_PIXEL_FORMAT_TYPE_UNORM,       8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    { VGPUTextureFormat_R8Snorm,               "R8Snorm",               VGPU_PIXEL_FORMAT_TYPE_SNORM,       8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    { VGPUTextureFormat_R8Uint,                "R8Uint",                VGPU_PIXEL_FORMAT_TYPE_UINT,        8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    { VGPUTextureFormat_R8Sint,                "R8Sint",                VGPU_PIXEL_FORMAT_TYPE_SINT,        8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},

    // 16-bit pixel formats
    { VGPUTextureFormat_R16Unorm,              "R16Unorm",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPUTextureFormat_R16Snorm,              "R16Snorm",              VGPU_PIXEL_FORMAT_TYPE_SNORM,       16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPUTextureFormat_R16Uint,               "R16Uint",               VGPU_PIXEL_FORMAT_TYPE_UINT,        16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPUTextureFormat_R16Sint,               "R16Sint",               VGPU_PIXEL_FORMAT_TYPE_SINT,        16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPUTextureFormat_R16Float,              "R16Float",              VGPU_PIXEL_FORMAT_TYPE_FLOAT,       16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPUTextureFormat_RG8Unorm,              "RG8Unorm",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    { VGPUTextureFormat_RG8Snorm,              "RG8Snorm",              VGPU_PIXEL_FORMAT_TYPE_SNORM,       16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    { VGPUTextureFormat_RG8Uint,               "RG8Uint",               VGPU_PIXEL_FORMAT_TYPE_UINT,        16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    { VGPUTextureFormat_RG8Sint,               "RG8Sint",               VGPU_PIXEL_FORMAT_TYPE_SINT,        16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},

    // Packed 16-bit pixel formats
    //{ VGPU_PIXEL_FORMAT_R5G6B5_UNORM,           "R5G6B5",             VGPU_PIXEL_FORMAT_TYPE_UNORM,       16,         {1, 1, 2, 1, 1},        {0, 0, 5, 6, 5, 0}},
    //{ VGPU_PIXEL_FORMAT_RGBA4_UNORM,            "RGBA4",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       16,         {1, 1, 2, 1, 1},        {0, 0, 4, 4, 4, 4}},

    // 32-bit pixel formats
    { VGPUTextureFormat_R32Uint,               "R32Uint",                 VGPU_PIXEL_FORMAT_TYPE_UINT,         32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
    { VGPUTextureFormat_R32Sint,               "R32Sint",                 VGPU_PIXEL_FORMAT_TYPE_SINT,         32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
    { VGPUTextureFormat_R32Float,              "R32Float",                 VGPU_PIXEL_FORMAT_TYPE_FLOAT,        32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
    //{ VGPUTextureFormat_RG16Unorm,             "RG16Unorm",                 VGPU_PIXEL_FORMAT_TYPE_UNORM,        32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    //{ VGPUTextureFormat_RG16Snorm,             "RG16Snorm",                VGPU_PIXEL_FORMAT_TYPE_SNORM,        32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { VGPUTextureFormat_RG16Uint,               "RG16Uint",                VGPU_PIXEL_FORMAT_TYPE_UINT,         32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { VGPUTextureFormat_RG16Sint,               "RG16Sint",                VGPU_PIXEL_FORMAT_TYPE_SINT,         32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { VGPUTextureFormat_RG16Float,              "RG16Float",                VGPU_PIXEL_FORMAT_TYPE_FLOAT,        32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { VGPUTextureFormat_RGBA8Unorm,             "RGBA8Unorm",                VGPU_PIXEL_FORMAT_TYPE_UNORM,        32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPUTextureFormat_RGBA8UnormSrgb,         "RGBA8UnormSrgb",            VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,    32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPUTextureFormat_RGBA8Snorm,             "RGBA8Snorm",               VGPU_PIXEL_FORMAT_TYPE_SNORM,        32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPUTextureFormat_RGBA8Uint,              "RGBA8Uint",               VGPU_PIXEL_FORMAT_TYPE_UINT,         32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPUTextureFormat_RGBA8Sint,              "RGBA8Sint",               VGPU_PIXEL_FORMAT_TYPE_SINT,         32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPUTextureFormat_BGRA8Unorm,             "BGRA8Unorm",                VGPU_PIXEL_FORMAT_TYPE_UNORM,        32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPUTextureFormat_BGRA8UnormSrgb,         "BGRA8UnormSrgb",            VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,    32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},

    // Packed 32-Bit Pixel formats
    { VGPUTextureFormat_RGB10A2Unorm,          "RGB10A2Unorm",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       32,         {1, 1, 4, 1, 1},        {0, 0, 10, 10, 10, 2}},
    //{ VGPU_PIXEL_FORMAT_RGB10A2_UINT,           "RGB10A2U",             VGPU_PIXEL_FORMAT_TYPE_UINT,        32,         {1, 1, 4, 1, 1},        {0, 0, 10, 10, 10, 2}},
    { VGPUTextureFormat_RG11B10Float,          "RG11B10Float",             VGPU_PIXEL_FORMAT_TYPE_FLOAT,       32,         {1, 1, 4, 1, 1},        {0, 0, 11, 11, 10, 0}},
    //{ VGPU_PIXEL_FORMAT_RGB9E5_FLOAT,           "RGB9E5F",              VGPU_PIXEL_FORMAT_TYPE_FLOAT,       32,         {1, 1, 4, 1, 1},        {0, 0, 9, 9, 9, 5}},

    // 64-Bit Pixel Formats
    { VGPUTextureFormat_RG32Uint,              "RG32Uint",              VGPU_PIXEL_FORMAT_TYPE_UINT,        64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
    { VGPUTextureFormat_RG32Sint,              "RG32Sint",              VGPU_PIXEL_FORMAT_TYPE_SINT,        64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
    { VGPUTextureFormat_RG32Float,             "RG32Float",             VGPU_PIXEL_FORMAT_TYPE_FLOAT,       64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
    //{ VGPU_PIXEL_FORMAT_RGBA16_UNORM,         "RGBA16",               VGPU_PIXEL_FORMAT_TYPE_UNORM,       64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    //{ VGPU_PIXEL_FORMAT_RGBA16_SNORM,         "RGBA16S",              VGPU_PIXEL_FORMAT_TYPE_SNORM,       64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { VGPUTextureFormat_RGBA16Uint,            "RGBA16Uint",              VGPU_PIXEL_FORMAT_TYPE_UINT,        64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { VGPUTextureFormat_RGBA16Sint,            "RGBA16Sint",              VGPU_PIXEL_FORMAT_TYPE_SINT,        64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { VGPUTextureFormat_RGBA16Float,           "RGBA16F",              VGPU_PIXEL_FORMAT_TYPE_FLOAT,       64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},

    // 128-Bit Pixel Formats
    { VGPUTextureFormat_RGBA32Uint,            "RGBA32Uint",              VGPU_PIXEL_FORMAT_TYPE_UINT,        128,        {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},
    { VGPUTextureFormat_RGBA32Sint,            "RGBA32Sint",              VGPU_PIXEL_FORMAT_TYPE_SINT,        128,        {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},
    { VGPUTextureFormat_RGBA32Float,           "RGBA32Float",              VGPU_PIXEL_FORMAT_TYPE_FLOAT,       128,        {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},

    // Depth-stencil
    { VGPU_PIXELFORMAT_DEPTH16_UNORM,           "Depth16Unorm",         VGPU_PIXEL_FORMAT_TYPE_UNORM,       16,         {1, 1, 2, 1, 1},        {16, 0, 0, 0, 0, 0}},
    { VGPU_PIXELFORMAT_DEPTH32_FLOAT,           "Depth32Float",         VGPU_PIXEL_FORMAT_TYPE_FLOAT,       32,         {1, 1, 4, 1, 1},        {32, 0, 0, 0, 0, 0}},
    { VGPU_PIXELFORMAT_DEPTH24_PLUS,            "Depth24Plus",          VGPU_PIXEL_FORMAT_TYPE_UNORM,       32,         {1, 1, 4, 1, 1},        {24, 8, 0, 0, 0, 0}},
    { VGPU_PIXELFORMAT_DEPTH24_PLUS_STENCIL8,    "Depth32FloatStencil8", VGPU_PIXEL_FORMAT_TYPE_FLOAT,       32,         {1, 1, 4, 1, 1},        {32, 8, 0, 0, 0, 0}},

    // Compressed formats
    { VGPUTextureFormat_BC1RGBAUnorm,           "BC1RGBAUnorm",         VGPU_PIXEL_FORMAT_TYPE_UNORM,        4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC1RGBAUnormSrgb,       "BC1RGBAUnormSrgb",     VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,    4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC2RGBAUnorm,           "BC2RGBAUnorm",         VGPU_PIXEL_FORMAT_TYPE_UNORM,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC2RGBAUnormSrgb,       "BC2RGBAUnormSrgb",     VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,    8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC3RGBAUnorm,           "BC3RGBAUnorm",         VGPU_PIXEL_FORMAT_TYPE_UNORM,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC3RGBAUnormSrgb,       "BC3RGBAUnormSrgb",     VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,    8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC4RUnorm,              "BC4RUnorm",            VGPU_PIXEL_FORMAT_TYPE_UNORM,        4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC4RSnorm,              "BC4RSnorm",            VGPU_PIXEL_FORMAT_TYPE_SNORM,        4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC5RGUnorm,             "BC5RGUnorm",           VGPU_PIXEL_FORMAT_TYPE_UNORM,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC5RGSnorm,             "BC5RGSnorm",           VGPU_PIXEL_FORMAT_TYPE_SNORM,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC6HRGBUfloat,          "BC6HRGBUfloat",        VGPU_PIXEL_FORMAT_TYPE_FLOAT,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC6HRGBSfloat,          "BC6HRGBSfloat",        VGPU_PIXEL_FORMAT_TYPE_FLOAT,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC7RGBAUnorm,           "BC7RGBAUnorm",         VGPU_PIXEL_FORMAT_TYPE_UNORM,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC7RGBAUnormSrgb,       "BC7RGBAUnorm",         VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,    8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},

    /*
    // Compressed PVRTC Pixel Formats
    { VGPU_PIXEL_FORMAT_PVRTC_RGB2,             "PVRTC_RGB2",           VGPU_PIXEL_FORMAT_TYPE_UNORM,       2,          {8, 4, 8, 2, 2},        {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_PVRTC_RGBA2,            "PVRTC_RGBA2",          VGPU_PIXEL_FORMAT_TYPE_UNORM,       2,          {8, 4, 8, 2, 2},        {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_PVRTC_RGB4,             "PVRTC_RGB4",           VGPU_PIXEL_FORMAT_TYPE_UNORM,       4,          {4, 4, 8, 2, 2},        {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_PVRTC_RGBA4,            "PVRTC_RGBA4",          VGPU_PIXEL_FORMAT_TYPE_UNORM,       4,          {4, 4, 8, 2, 2},        {0, 0, 0, 0, 0, 0}},

    // Compressed ETC Pixel Formats
    { VGPU_PIXEL_FORMAT_ETC2_RGB8,              "ETC2_RGB8",            VGPU_PIXEL_FORMAT_TYPE_UNORM,       4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_ETC2_RGB8A1,            "ETC2_RGB8A1",          VGPU_PIXEL_FORMAT_TYPE_UNORM,       4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},

    // Compressed ASTC Pixel Formats
    { VGPU_PIXEL_FORMAT_ASTC4x4,                "ASTC4x4",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_ASTC5x5,                "ASTC5x5",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       6,          {5, 5, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_ASTC6x6,                "ASTC6x6",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       4,          {6, 6, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_ASTC8x5,                "ASTC8x5",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       4,          {8, 5, 16, 1, 1},       {0, 0, 0, 0, 0, 0} },
    { VGPU_PIXEL_FORMAT_ASTC8x6,                "ASTC8x6",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       3,          {8, 6, 16, 1, 1},       {0, 0, 0, 0, 0, 0} },
    { VGPU_PIXEL_FORMAT_ASTC8x8,                "ASTC8x8",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       3,          {8, 8, 16, 1, 1},       {0, 0, 0, 0, 0, 0} },
    { VGPU_PIXEL_FORMAT_ASTC10x10,              "ASTC10x10",            VGPU_PIXEL_FORMAT_TYPE_UNORM,       3,          {10, 10, 16, 1, 1},     {0, 0, 0, 0, 0, 0} },
    { VGPU_PIXEL_FORMAT_ASTC12x12,              "ASTC12x12",            VGPU_PIXEL_FORMAT_TYPE_UNORM,       3,          {12, 12, 16, 1, 1},     {0, 0, 0, 0, 0, 0} },*/
};

uint32_t vgpu_get_format_bits_per_pixel(vgpu_pixel_format format)
{
    assert(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].bitsPerPixel;
}

uint32_t vgpu_get_format_block_size(vgpu_pixel_format format)
{
    assert(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].compression.blockSize;
}

uint32_t vgpuGetFormatBlockWidth(vgpu_pixel_format format)
{
    assert(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].compression.blockWidth;
}

uint32_t vgpuGetFormatBlockHeight(vgpu_pixel_format format)
{
    assert(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].compression.blockHeight;
}

vgpu_pixel_format_type vgpu_get_format_type(vgpu_pixel_format format)
{
    assert(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].type;
}

bool vgpu_is_depth_format(vgpu_pixel_format format)
{
    VGPU_ASSERT(FormatDesc[format].format == format);
    return FormatDesc[format].bits.depth > 0;
}

bool vgpu_is_stencil_format(vgpu_pixel_format format)
{
    VGPU_ASSERT(FormatDesc[format].format == format);
    return FormatDesc[format].bits.stencil > 0;
}

bool vgpu_is_depth_stencil_format(vgpu_pixel_format format)
{
    return vgpu_is_depth_format(format) || vgpu_is_stencil_format(format);
}

bool vgpu_is_compressed_format(vgpu_pixel_format format)
{
    VGPU_ASSERT(FormatDesc[format].format == format);
    return format >= VGPUTextureFormat_BC1RGBAUnorm && format <= VGPUTextureFormat_BC7RGBAUnormSrgb;
}

const char* vgpu_get_format_name(vgpu_pixel_format format)
{
    VGPU_ASSERT(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].name;
}
