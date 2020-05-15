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

#include "vgpu_backend.h"
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#include <stdio.h>
#include <stdarg.h>

#define VGPU_MAX_LOG_MESSAGE (4096)
static const char* vgpu_log_priority_prefixes[_VGPU_LOG_LEVEL_COUNT] = {
    NULL,
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG",
    "TRACE",
};

static vgpu_log_callback s_log_function = NULL;
static void* s_log_user_data = NULL;

void vgpu_log(vgpu_log_level level, const char* format, ...) {
    if (level == VGPU_LOG_LEVEL_OFF) {
        return;
    }

    if (s_log_function) {
        char msg[VGPU_MAX_LOG_MESSAGE];
        va_list args;
        va_start(args, format);
        vsnprintf(msg, sizeof(msg), format, args);
        s_log_function(s_log_user_data, level, msg);
        va_end(args);
    }
}

vgpu_backend_type vgpuGetDefaultPlatformBackend(void) {
#if defined(_WIN32) || defined(_WIN64)
    if (vgpuIsBackendSupported(VGPU_BACKEND_TYPE_DIRECT3D12)) {
        return VGPU_BACKEND_TYPE_DIRECT3D12;
    }

    if (vgpuIsBackendSupported(VGPU_BACKEND_TYPE_VULKAN)) {
        return VGPU_BACKEND_TYPE_VULKAN;
    }

    if (vgpuIsBackendSupported(VGPU_BACKEND_TYPE_DIRECT3D11)) {
        return VGPU_BACKEND_TYPE_DIRECT3D11;
    }

    if (vgpuIsBackendSupported(VGPU_BACKEND_TYPE_OPENGL)) {
        return VGPU_BACKEND_TYPE_OPENGL;
    }

    return VGPU_BACKEND_TYPE_DIRECT3D11;
#elif defined(__linux__) || defined(__ANDROID__)
    return VGPU_BACKEND_TYPE_OPENGL;
#elif defined(__APPLE__)
    return VGPU_BACKEND_TYPE_VULKAN;
#else
    return VGPU_BACKEND_TYPE_OPENGL;
#endif
}

vgpu_bool vgpuIsBackendSupported(vgpu_backend_type backend) {
    if (backend == VGPU_BACKEND_TYPE_COUNT) {
        backend = vgpuGetDefaultPlatformBackend();
    }

    switch (backend)
    {
#if defined(VGPU_BACKEND_VK)
    case VGPU_BACKEND_TYPE_VULKAN:
        return vk_driver.supported();
#endif

    /*case VGPU_BACKEND_TYPE_DIRECT3D12:
        return vgpu_d3d12_supported();

    case VGPU_BACKEND_TYPE_DIRECT3D11:
        return vgpu_d3d11_supported();*/

#if defined(VGPU_BACKEND_OPENGL)
    case VGPU_BACKEND_TYPE_OPENGL:
        return gl_driver.supported();
#endif

    default:
        return false;
    }
}

static vgpu_renderer* s_gpu_device = NULL;

vgpu_bool vgpu_init(const vgpu_config* config)
{
    if (s_gpu_device) {
        return true;
    }

    s_log_user_data = config->context;
    s_log_function = config->callback;

    vgpu_backend_type backend = config->type;
    if (backend == VGPU_BACKEND_TYPE_COUNT) {
        backend = vgpuGetDefaultPlatformBackend();
    }

    switch (backend)
    {
    /*case VGPU_BACKEND_TYPE_VULKAN:
        s_gpu_device = vgpu_vk_create_device();
        break;

    case VGPU_BACKEND_TYPE_DIRECT3D12:
        s_gpu_device = vgpu_d3d12_create_device();
        break;

    case VGPU_BACKEND_TYPE_DIRECT3D11:
        s_gpu_device = vgpu_d3d11_create_device();
        break;*/

#if defined(VGPU_BACKEND_OPENGL)
    case VGPU_BACKEND_TYPE_OPENGL:
        s_gpu_device = gl_driver.init_renderer();
        break;
    }
#endif

    if (s_gpu_device == NULL) {
        return false;
    }

    if (!s_gpu_device->init(config)) {
        s_gpu_device = NULL;
        return false;
    }

    return true;
}

void vgpu_shutdown(void) {
    if (s_gpu_device == NULL) {
        return;
    }

    s_gpu_device->destroy();
}

vgpu_backend_type vgpu_get_backend(void) {
    VGPU_ASSERT(s_gpu_device);
    return s_gpu_device->get_backend();
}

vgpu_caps vgpu_get_caps(void) {
    VGPU_ASSERT(s_gpu_device);
    return s_gpu_device->get_caps();
}

VGPUTextureFormat vgpuGetDefaultDepthFormat(void) {
    VGPU_ASSERT(s_gpu_device);
    return s_gpu_device->GetDefaultDepthFormat();
}

VGPUTextureFormat vgpuGetDefaultDepthStencilFormat(void) {
    VGPU_ASSERT(s_gpu_device);
    return s_gpu_device->GetDefaultDepthStencilFormat();
}

void vgpu_frame_begin(void) {
    s_gpu_device->begin_frame();
}

void vgpu_frame_end(void) {
    s_gpu_device->end_frame();
}

/* Texture */
static VGPUTextureDescriptor _vgpu_texture_desc_defaults(const VGPUTextureDescriptor* desc) {
    VGPUTextureDescriptor def = *desc;
    def.dimension = _vgpu_def(desc->dimension, VGPUTextureDimension_2D);
    def.size.width = _vgpu_def(desc->size.width, 1);
    def.size.height = _vgpu_def(desc->size.height, 1);
    def.size.depth = _vgpu_def(desc->size.depth, 1);
    def.format = _vgpu_def(def.format, VGPUTextureFormat_RGBA8Unorm);
    def.mipLevelCount = _vgpu_def(def.mipLevelCount, 1);
    def.sampleCount = _vgpu_def(def.sampleCount, 1);
    return def;
}

vgpu_texture vgpuCreateTexture(const VGPUTextureDescriptor* descriptor)
{
    VGPUTextureDescriptor desc_def = _vgpu_texture_desc_defaults(descriptor);
    return s_gpu_device->create_texture(&desc_def);
}

void vgpuDestroyTexture(vgpu_texture texture) {
    s_gpu_device->destroy_texture(texture);
}

/* Buffer */
vgpu_buffer vgpu_create_buffer(const vgpu_buffer_info* info) {
    return s_gpu_device->create_buffer(info);
}

void vgpu_destroy_buffer(vgpu_buffer buffer) {
    s_gpu_device->destroy_buffer(buffer);
}

/* Sampler */
vgpu_sampler vgpu_create_sampler(const vgpu_sampler_info* info)
{
    VGPU_ASSERT(s_gpu_device);
    VGPU_ASSERT(info);
    return s_gpu_device->create_sampler(info);
}

void vgpu_destroy_sampler(vgpu_sampler sampler)
{
    VGPU_ASSERT(s_gpu_device);
    s_gpu_device->destroy_sampler( sampler);
}

/* Shader */
vgpu_shader vgpu_create_shader(const vgpu_shader_info* info) {
    VGPU_ASSERT(s_gpu_device);
    VGPU_ASSERT(info);
    return s_gpu_device->create_shader(info);
}

void vgpu_destroy_shader(vgpu_shader shader) {
    VGPU_ASSERT(s_gpu_device);
    s_gpu_device->destroy_shader(shader);
}

/* Commands */
void vgpuCmdBeginRenderPass(const VGPURenderPassDescriptor* descriptor)
{
    VGPU_ASSERT(descriptor);
    s_gpu_device->cmdBeginRenderPass(descriptor);
}

void vgpuCmdEndRenderPass(void) {
    s_gpu_device->cmdEndRenderPass();
}

/* Pixel Format */
typedef struct VgpuPixelFormatDesc
{
    VGPUTextureFormat       format;
    const char* name;
    VGPUTextureFormatType    type;
    uint8_t                 bitsPerPixel;
    struct
    {
        uint8_t             blockWidth;
        uint8_t             blockHeight;
        uint8_t             blockSize;
        uint8_t             minBlockX;
        uint8_t             minBlockY;
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
} VgpuPixelFormatDesc;

const VgpuPixelFormatDesc FormatDesc[] =
{
    // format                                   name,                   type                                bpp         compression             bits
    { VGPUTextureFormat_Undefined,              "Undefined",            VGPUTextureFormatType_Unknown,     0,          {0, 0, 0, 0, 0},        {0, 0, 0, 0, 0, 0}},
    // 8-bit pixel formats
    { VGPUTextureFormat_R8Unorm,               "R8Unorm",               VGPUTextureFormatType_Unorm,       8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    { VGPUTextureFormat_R8Snorm,               "R8Snorm",               VGPUTextureFormatType_Snorm,       8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    { VGPUTextureFormat_R8Uint,                "R8Uint",                VGPUTextureFormatType_Uint,        8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    { VGPUTextureFormat_R8Sint,                "R8Sint",                VGPUTextureFormatType_Sint,        8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},

    // 16-bit pixel formats
    { VGPUTextureFormat_R16Unorm,              "R16Unorm",              VGPUTextureFormatType_Unorm,       16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPUTextureFormat_R16Snorm,              "R16Snorm",              VGPUTextureFormatType_Snorm,       16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPUTextureFormat_R16Uint,               "R16Uint",               VGPUTextureFormatType_Uint,        16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPUTextureFormat_R16Sint,               "R16Sint",               VGPUTextureFormatType_Sint,        16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPUTextureFormat_R16Float,              "R16Float",              VGPUTextureFormatType_Float,       16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPUTextureFormat_RG8Unorm,              "RG8Unorm",              VGPUTextureFormatType_Unorm,       16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    { VGPUTextureFormat_RG8Snorm,              "RG8Snorm",              VGPUTextureFormatType_Snorm,       16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    { VGPUTextureFormat_RG8Uint,               "RG8Uint",               VGPUTextureFormatType_Uint,        16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    { VGPUTextureFormat_RG8Sint,               "RG8Sint",               VGPUTextureFormatType_Sint,        16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},

    // Packed 16-bit pixel formats
    //{ VGPU_PIXEL_FORMAT_R5G6B5_UNORM,           "R5G6B5",             VGPUTextureFormatType_Unorm,       16,         {1, 1, 2, 1, 1},        {0, 0, 5, 6, 5, 0}},
    //{ VGPU_PIXEL_FORMAT_RGBA4_UNORM,            "RGBA4",              VGPUTextureFormatType_Unorm,       16,         {1, 1, 2, 1, 1},        {0, 0, 4, 4, 4, 4}},

    // 32-bit pixel formats
    { VGPUTextureFormat_R32Uint,               "R32Uint",                 VGPUTextureFormatType_Uint,         32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
    { VGPUTextureFormat_R32Sint,               "R32Sint",                 VGPUTextureFormatType_Sint,         32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
    { VGPUTextureFormat_R32Float,              "R32Float",                 VGPUTextureFormatType_Float,        32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
    //{ VGPUTextureFormat_RG16Unorm,             "RG16Unorm",                 VGPUTextureFormatType_Unorm,        32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    //{ VGPUTextureFormat_RG16Snorm,             "RG16Snorm",                VGPUTextureFormatType_Snorm,        32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { VGPUTextureFormat_RG16Uint,               "RG16Uint",                VGPUTextureFormatType_Uint,         32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { VGPUTextureFormat_RG16Sint,               "RG16Sint",                VGPUTextureFormatType_Sint,         32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { VGPUTextureFormat_RG16Float,              "RG16Float",                VGPUTextureFormatType_Float,        32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { VGPUTextureFormat_RGBA8Unorm,             "RGBA8Unorm",                VGPUTextureFormatType_Unorm,        32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPUTextureFormat_RGBA8UnormSrgb,         "RGBA8UnormSrgb",            VGPUTextureFormatType_UnormSrgb,    32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPUTextureFormat_RGBA8Snorm,             "RGBA8Snorm",               VGPUTextureFormatType_Snorm,        32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPUTextureFormat_RGBA8Uint,              "RGBA8Uint",               VGPUTextureFormatType_Uint,         32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPUTextureFormat_RGBA8Sint,              "RGBA8Sint",               VGPUTextureFormatType_Sint,         32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPUTextureFormat_BGRA8Unorm,             "BGRA8Unorm",                VGPUTextureFormatType_Unorm,        32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPUTextureFormat_BGRA8UnormSrgb,         "BGRA8UnormSrgb",            VGPUTextureFormatType_UnormSrgb,    32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},

    // Packed 32-Bit Pixel formats
    { VGPUTextureFormat_RGB10A2Unorm,          "RGB10A2Unorm",              VGPUTextureFormatType_Unorm,       32,         {1, 1, 4, 1, 1},        {0, 0, 10, 10, 10, 2}},
    //{ VGPU_PIXEL_FORMAT_RGB10A2_UINT,           "RGB10A2U",             VGPUTextureFormatType_Uint,        32,         {1, 1, 4, 1, 1},        {0, 0, 10, 10, 10, 2}},
    { VGPUTextureFormat_RG11B10Float,          "RG11B10Float",             VGPUTextureFormatType_Float,       32,         {1, 1, 4, 1, 1},        {0, 0, 11, 11, 10, 0}},
    //{ VGPU_PIXEL_FORMAT_RGB9E5_FLOAT,           "RGB9E5F",              VGPUTextureFormatType_Float,       32,         {1, 1, 4, 1, 1},        {0, 0, 9, 9, 9, 5}},

    // 64-Bit Pixel Formats
    { VGPUTextureFormat_RG32Uint,              "RG32Uint",              VGPUTextureFormatType_Uint,        64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
    { VGPUTextureFormat_RG32Sint,              "RG32Sint",              VGPUTextureFormatType_Sint,        64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
    { VGPUTextureFormat_RG32Float,             "RG32Float",             VGPUTextureFormatType_Float,       64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
    //{ VGPU_PIXEL_FORMAT_RGBA16_UNORM,         "RGBA16",               VGPUTextureFormatType_Unorm,       64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    //{ VGPU_PIXEL_FORMAT_RGBA16_SNORM,         "RGBA16S",              VGPUTextureFormatType_Snorm,       64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { VGPUTextureFormat_RGBA16Uint,            "RGBA16Uint",              VGPUTextureFormatType_Uint,        64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { VGPUTextureFormat_RGBA16Sint,            "RGBA16Sint",              VGPUTextureFormatType_Sint,        64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { VGPUTextureFormat_RGBA16Float,           "RGBA16F",              VGPUTextureFormatType_Float,       64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},

    // 128-Bit Pixel Formats
    { VGPUTextureFormat_RGBA32Uint,            "RGBA32Uint",              VGPUTextureFormatType_Uint,        128,        {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},
    { VGPUTextureFormat_RGBA32Sint,            "RGBA32Sint",              VGPUTextureFormatType_Sint,        128,        {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},
    { VGPUTextureFormat_RGBA32Float,           "RGBA32Float",              VGPUTextureFormatType_Float,       128,        {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},

    // Depth-stencil
    { VGPUTextureFormat_Depth16Unorm,           "Depth16Unorm",         VGPUTextureFormatType_Unorm,       16,         {1, 1, 2, 1, 1},        {16, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_Depth32Float,           "Depth32Float",         VGPUTextureFormatType_Float,       32,         {1, 1, 4, 1, 1},        {32, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_Depth24Plus,            "Depth24Plus", VGPUTextureFormatType_Unorm,       32,         {1, 1, 4, 1, 1},        {24, 8, 0, 0, 0, 0}},
    { VGPUTextureFormat_Depth24PlusStencil8,    "Depth32FloatStencil8", VGPUTextureFormatType_Float,       32,         {1, 1, 4, 1, 1},        {32, 8, 0, 0, 0, 0}},

    // Compressed formats
    { VGPUTextureFormat_BC1RGBAUnorm,           "BC1RGBAUnorm",         VGPUTextureFormatType_Unorm,        4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC1RGBAUnormSrgb,       "BC1RGBAUnormSrgb",     VGPUTextureFormatType_UnormSrgb,    4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC2RGBAUnorm,           "BC2RGBAUnorm",         VGPUTextureFormatType_Unorm,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC2RGBAUnormSrgb,       "BC2RGBAUnormSrgb",     VGPUTextureFormatType_UnormSrgb,    8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC3RGBAUnorm,           "BC3RGBAUnorm",         VGPUTextureFormatType_Unorm,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC3RGBAUnormSrgb,       "BC3RGBAUnormSrgb",     VGPUTextureFormatType_UnormSrgb,    8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC4RUnorm,              "BC4RUnorm",            VGPUTextureFormatType_Unorm,        4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC4RSnorm,              "BC4RSnorm",            VGPUTextureFormatType_Snorm,        4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC5RGUnorm,             "BC5RGUnorm",           VGPUTextureFormatType_Unorm,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC5RGSnorm,             "BC5RGSnorm",           VGPUTextureFormatType_Snorm,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC6HRGBUfloat,          "BC6HRGBUfloat",        VGPUTextureFormatType_Float,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC6HRGBSfloat,          "BC6HRGBSfloat",        VGPUTextureFormatType_Float,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC7RGBAUnorm,           "BC7RGBAUnorm",         VGPUTextureFormatType_Unorm,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC7RGBAUnormSrgb,       "BC7Srgb",              VGPUTextureFormatType_UnormSrgb,    8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},

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

uint32_t vgpu_get_format_bits_per_pixel(VGPUTextureFormat format)
{
    assert(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].bitsPerPixel;
}

uint32_t vgpu_get_format_block_size(VGPUTextureFormat format)
{
    assert(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].compression.blockSize;
}

uint32_t vgpuGetFormatBlockWidth(VGPUTextureFormat format)
{
    assert(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].compression.blockWidth;
}

uint32_t vgpuGetFormatBlockHeight(VGPUTextureFormat format)
{
    assert(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].compression.blockHeight;
}

VGPUTextureFormatType vgpuGetFormatType(VGPUTextureFormat format)
{
    assert(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].type;
}

vgpu_bool vgpu_is_depth_format(VGPUTextureFormat format)
{
    VGPU_ASSERT(FormatDesc[format].format == format);
    return FormatDesc[format].bits.depth > 0;
}

vgpu_bool vgpu_is_stencil_format(VGPUTextureFormat format)
{
    VGPU_ASSERT(FormatDesc[format].format == format);
    return FormatDesc[format].bits.stencil > 0;
}

vgpu_bool vgpu_is_depth_stencil_format(VGPUTextureFormat format)
{
    return vgpu_is_depth_format(format) || vgpu_is_stencil_format(format);
}

vgpu_bool vgpu_is_compressed_format(VGPUTextureFormat format)
{
    VGPU_ASSERT(FormatDesc[format].format == format);
    return format >= VGPUTextureFormat_BC1RGBAUnorm && format <= VGPUTextureFormat_BC7RGBAUnormSrgb;
}

const char* vgpu_get_format_name(VGPUTextureFormat format)
{
    VGPU_ASSERT(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].name;
}
