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

#include "config.h"
#include "gpu_backend.h"
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

#define GPU_MAX_LOG_MESSAGE (1024)
static const char* vgpu_log_priority_prefixes[_AGPU_LOG_LEVEL_COUNT] = {
    NULL,
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG",
    "TRACE",
};

static void _s_agpuDefaultLogCallback(void* user_data, GPULogLevel level, const char* message);

#ifdef _DEBUG
static GPULogLevel s_log_level = GPULogLevel_Debug;
#else
static GPULogLevel s_log_level = GPULogLevel_Off;
#endif
static GPULogCallback s_log_function = _s_agpuDefaultLogCallback;
static void* s_log_user_data = NULL;

void agpuSetLogLevel(GPULogLevel level) {
    s_log_level = level;
}

void agpuSetLogCallback(GPULogCallback callback, void* user_data) {
    s_log_function = callback;
    s_log_user_data = user_data;
}

void _s_agpuDefaultLogCallback(void* user_data, GPULogLevel level, const char* message) {
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
    case GPULogLevel_Error:
    case GPULogLevel_Warn:
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

void agpuLog(GPULogLevel level, const char* format, ...) {
    if (s_log_level == GPULogLevel_Off || level == GPULogLevel_Off) {
        return;
    }

    if (s_log_level < level) {
        return;
    }

    if (s_log_function) {
        char msg[GPU_MAX_LOG_MESSAGE];
        va_list args;
        va_start(args, format);
        vsnprintf(msg, sizeof(msg), format, args);
        s_log_function(s_log_user_data, level, msg);
        va_end(args);
    }
}

agpu_backend_type gpu_get_default_platform_backend(void) {
#if defined(_WIN32) || defined(_WIN64)
    if (gpu_is_backend_supported(AGPU_BACKEND_TYPE_D3D12)) {
        return AGPU_BACKEND_TYPE_D3D12;
    }

    if (gpu_is_backend_supported(AGPU_BACKEND_TYPE_VULKAN)) {
        return AGPU_BACKEND_TYPE_VULKAN;
    }

    if (gpu_is_backend_supported(AGPU_BACKEND_TYPE_D3D11)) {
        return AGPU_BACKEND_TYPE_D3D11;
    }

    if (gpu_is_backend_supported(AGPU_BACKEND_TYPE_OPENGL)) {
        return AGPU_BACKEND_TYPE_OPENGL;
    }

    return AGPU_BACKEND_TYPE_NULL;
#elif defined(__linux__) || defined(__ANDROID__)
    return GPUBackendType_Vulkan;
#elif defined(__APPLE__)
    return GPUBackendType_Vulkan;
#else
    return GPUBackendType_OpenGL;
#endif
}

bool gpu_is_backend_supported(agpu_backend_type backend) {
    if (backend == AGPU_BACKEND_TYPE_DEFAULT) {
        backend = gpu_get_default_platform_backend();
    }

    switch (backend)
    {
    case AGPU_BACKEND_TYPE_NULL:
        return true;
#if defined(GPU_VK_BACKEND) && TODO_VK
    case AGPU_BACKEND_TYPE_VULKAN:
        return gpu_vk_supported();
#endif
#if defined(GPU_D3D12_BACKEND) && defined(TODO_D3D12)
    case AGPU_BACKEND_TYPE_D3D12:
        return d3d12_driver.supported();
#endif 

#if defined(GPU_D3D11_BACKEND)
    case AGPU_BACKEND_TYPE_D3D11:
        return d3d11_driver.supported();
#endif

#if defined(VGPU_BACKEND_GL)
    case VGPU_BACKEND_OPENGL:
        return agpu_gl_supported();
#endif // defined(AGPU_BACKEND_GL)

    default:
        return false;
    }
}

GPUDevice agpuCreateDevice(const agpu_device_info* info)
{
    VGPU_ASSERT(info);

    agpu_backend_type backend = info->preferred_backend;
    if (backend == AGPU_BACKEND_TYPE_DEFAULT) {
        backend = gpu_get_default_platform_backend();
    }

    GPUDevice device = NULL;
    switch (backend)
    {
    case AGPU_BACKEND_TYPE_NULL:
        break;

#if defined(GPU_VK_BACKEND) && TODO_VK
    case AGPU_BACKEND_TYPE_VULKAN:
        s_renderer = vk_gpu_create_renderer();
        break;
#endif

#if defined(GPU_D3D12_BACKEND) && defined(TODO_D3D12)
    case AGPU_BACKEND_TYPE_D3D12:
        device = d3d12_driver.create_device(info);
        break;
#endif

#if defined(GPU_D3D11_BACKEND) 
    case AGPU_BACKEND_TYPE_D3D11:
        device = d3d11_driver.create_device(info);
        break;
#endif
    }

    if (device == NULL) {
        return NULL;
    }

    return device;
}

void agpu_destroy_device(GPUDevice device) {
    if (device == NULL) {
        return;
    }

    device->destroyDevice(device);
}

void agpu_frame_begin(GPUDevice device) {
    device->beginFrame(device->renderer);
}

void agpu_frame_end(GPUDevice device) {
    device->presentFrame(device->renderer);
}

void agpu_wait_gpu(GPUDevice device) {
    device->waitForGPU(device->renderer);
}

agpu_backend_type gpu_query_backend(GPUDevice device) {
    VGPU_ASSERT(device);
    return device->query_caps(device->renderer).backend;
}

GPUDeviceCapabilities gpu_query_caps(GPUDevice device) {
    VGPU_ASSERT(device);
    return device->query_caps(device->renderer);
}

AGPUPixelFormat gpuGetDefaultDepthFormat(GPUDevice device)
{
    VGPU_ASSERT(device);
    return device->getDefaultDepthFormat(device->renderer);
}

AGPUPixelFormat gpuGetDefaultDepthStencilFormat(GPUDevice device)
{
    VGPU_ASSERT(device);
    return device->getDefaultDepthStencilFormat(device->renderer);
}

/* Texture */
static AGPUTextureDescriptor texture_info_default(const AGPUTextureDescriptor* info) {
    AGPUTextureDescriptor def = *info;
    def.type = _vgpu_def(info->type, AGPUTextureType_2D);
    def.format = _vgpu_def(info->format, GPUTextureFormat_RGBA8Unorm);
    def.size.width = _vgpu_def(info->size.width, 1);
    def.size.height = _vgpu_def(info->size.height, 1);
    def.size.depth = _vgpu_def(info->size.depth, 1);
    def.mipLevelCount = _vgpu_def(info->mipLevelCount, 1);
    def.sampleCount = _vgpu_def(info->sampleCount, 1u);
    return def;
}


AGPUTexture agpuDeviceCreateTexture(GPUDevice device, const AGPUTextureDescriptor* descriptor) {
    VGPU_ASSERT(device);
    VGPU_ASSERT(descriptor);

    AGPUTextureDescriptor info_def = texture_info_default(descriptor);
    return device->createTexture(device->renderer, &info_def);
}

void agpuDeviceDestroyTexture(GPUDevice device, AGPUTexture texture) {
    VGPU_ASSERT(device);
    VGPU_ASSERT(texture);
    device->destroyTexture(device->renderer, texture);
}

#if TODO

/* Buffer */
vgpu_buffer vgpu_create_buffer(const vgpu_buffer_desc* descriptor)
{
    return s_gpuDevice->create_buffer(s_gpuDevice->renderer, descriptor);
}

void vgpu_destroy_buffer(vgpu_buffer buffer)
{
    s_gpuDevice->destroy_buffer(s_gpuDevice->renderer, buffer);
}

/* Texture */

void vgpu_destroy_texture(vgpu_texture texture)
{
    s_gpuDevice->destroy_texture(s_gpuDevice->renderer, texture);
}

vgpu_texture_desc vgpu_query_texture_desc(vgpu_texture texture)
{
    return s_gpuDevice->query_texture_desc(texture);
}

uint32_t vgpu_get_texture_width(vgpu_texture texture, uint32_t mip_level)
{
    return _vgpu_max(1u, vgpu_query_texture_desc(texture).width >> mip_level);
}

uint32_t vgpu_get_texture_height(vgpu_texture texture, uint32_t mip_level)
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

/* Shader */
vgpu_shader vgpu_create_shader(const vgpu_shader_desc* desc)
{
    VGPU_ASSERT(desc);
    return s_gpuDevice->create_shader(s_gpuDevice->renderer, desc);
}

void vgpu_destroy_shader(vgpu_shader shader)
{
    s_gpuDevice->destroy_shader(s_gpuDevice->renderer, shader);
}

/* Pipeline */
vgpu_pipeline vgpu_create_render_pipeline(const vgpu_render_pipeline_desc* desc)
{
    VGPU_ASSERT(desc);
    return s_gpuDevice->create_render_pipeline(s_gpuDevice->renderer, desc);
}

vgpu_pipeline vgpu_create_compute_pipeline(const VgpuComputePipelineDescriptor* desc)
{
    VGPU_ASSERT(desc);
    return s_gpuDevice->create_compute_pipeline(s_gpuDevice->renderer, desc);
}

void vgpu_destroy_pipeline(vgpu_pipeline pipeline)
{
    s_gpuDevice->destroy_pipeline(s_gpuDevice->renderer, pipeline);
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
#endif // TODO


/* Pixel Format */
typedef struct vgpu_pixel_format_desc
{
    AGPUPixelFormat format;
    const char* name;
    AGPUPixelFormatType type;
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
    // format                                       name,                   type                                bpp         compression             bits
    { GPU_TEXTURE_FORMAT_UNDEFINED,                 "Undefined",            AGPUPixelFormatType_Unknown,     0,          {0, 0, 0, 0, 0},        {0, 0, 0, 0, 0, 0}},
    // 8-bit pixel formats
    { GPU_TEXTURE_FORMAT_R8_UNORM,                  "R8Unorm",              AGPUPixelFormatType_Unorm,       8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_R8_SNORM,                  "R8Snorm",              AGPUPixelFormatType_Snorm,       8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_R8_UINT,                   "R8Uint",               AGPUPixelFormatType_Uint,        8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_R8_SINT,                   "R8Sint",               AGPUPixelFormatType_Sint,        8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},

    // 16-bit pixel formats
    //{ GPU_TEXTURE_FORMAT_R16_UNORM,               "R16Unorm",             AGPUPixelFormatType_Unorm,       16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    //{ GPU_TEXTURE_FORMAT_R16_SNORM,               "R16Snorm",             AGPUPixelFormatType_Snorm,       16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_R16_UINT,                  "R16Uint",              AGPUPixelFormatType_Uint,        16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_R16_SINT,                  "R16Sint",              AGPUPixelFormatType_Sint,        16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_R16_FLOAT,                 "R16Float",             AGPUPixelFormatType_Float,       16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_RG8_UNORM,                 "RG8Unorm",             AGPUPixelFormatType_Unorm,       16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    { GPU_TEXTURE_FORMAT_RG8_SNORM,                 "RG8Snorm",             AGPUPixelFormatType_Snorm,       16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    { GPU_TEXTURE_FORMAT_RG8_UINT,                  "RG8Uint",              AGPUPixelFormatType_Uint,        16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    { GPU_TEXTURE_FORMAT_RG8_SINT,                  "RG8Sint",              AGPUPixelFormatType_Sint,        16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},

    // Packed 16-bit pixel formats
    //{ VGPU_PIXEL_FORMAT_R5G6B5_UNORM,             "R5G6B5",               AGPUPixelFormatType_Unorm,       16,         {1, 1, 2, 1, 1},        {0, 0, 5, 6, 5, 0}},
    //{ VGPU_PIXEL_FORMAT_RGBA4_UNORM,              "RGBA4",                AGPUPixelFormatType_Unorm,       16,         {1, 1, 2, 1, 1},        {0, 0, 4, 4, 4, 4}},

    // 32-bit pixel formats
    { GPU_TEXTURE_FORMAT_R32_UINT,                  "R32Uint",              AGPUPixelFormatType_Uint,            32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_R32_SINT,                  "R32Sint",              AGPUPixelFormatType_Sint,            32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_R32_FLOAT,                 "R32Float",             AGPUPixelFormatType_Float,           32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
    //{ VGPUTextureFormat_RG16Unorm,                "RG16Unorm",            AGPUPixelFormatType_Unorm,           32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    //{ VGPUTextureFormat_RG16Snorm,                "RG16Snorm",            AGPUPixelFormatType_Snorm,           32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { GPU_TEXTURE_FORMAT_RG16_UINT,                 "RG16Uint",             AGPUPixelFormatType_Uint,            32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { GPU_TEXTURE_FORMAT_RG16_SINT,                 "RG16Sint",             AGPUPixelFormatType_Sint,            32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { GPU_TEXTURE_FORMAT_RG16_FLOAT,                "RG16Float",            AGPUPixelFormatType_Float,           32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { GPUTextureFormat_RGBA8Unorm,                  "RGBA8Unorm",           AGPUPixelFormatType_Unorm,           32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { GPUTextureFormat_RGBA8UnormSrgb,              "RGBA8UnormSrgb",       AGPUPixelFormatType_UnormSrgb,      32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { GPU_TEXTURE_FORMAT_RGBA8_SNORM,               "RGBA8Snorm",           AGPUPixelFormatType_Snorm,           32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { GPU_TEXTURE_FORMAT_RGBA8_UINT,                "RGBA8Uint",            AGPUPixelFormatType_Uint,            32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { GPU_TEXTURE_FORMAT_RGBA8_SINT,                "RGBA8Sint",            AGPUPixelFormatType_Sint,            32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { GPUTextureFormat_BGRA8Unorm,                  "BGRA8Unorm",           AGPUPixelFormatType_Unorm,           32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { GPUTextureFormat_BGRA8UnormSrgb,              "BGRA8UnormSrgb",       AGPUPixelFormatType_UnormSrgb,      32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},

    // Packed 32-Bit Pixel formats
    { GPU_TEXTURE_FORMAT_RGB10A2_UNORM,             "RGB10A2Unorm",         AGPUPixelFormatType_Unorm,       32,         {1, 1, 4, 1, 1},        {0, 0, 10, 10, 10, 2}},
    //{ VGPU_PIXEL_FORMAT_RGB10A2_UINT,             "RGB10A2U",             AGPUPixelFormatType_Uint,        32,         {1, 1, 4, 1, 1},        {0, 0, 10, 10, 10, 2}},
    { GPU_TEXTURE_FORMAT_RG11B10_FLOAT,             "RG11B10Float",         AGPUPixelFormatType_Float,       32,         {1, 1, 4, 1, 1},        {0, 0, 11, 11, 10, 0}},
    //{ VGPU_PIXEL_FORMAT_RGB9E5_FLOAT,             "RGB9E5F",              AGPUPixelFormatType_Float,       32,         {1, 1, 4, 1, 1},        {0, 0, 9, 9, 9, 5}},

    // 64-Bit Pixel Formats
    { GPU_TEXTURE_FORMAT_RG32_UINT,                 "RG32Uint",             AGPUPixelFormatType_Uint,        64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
    { GPU_TEXTURE_FORMAT_RG32_SINT,                 "RG32Sint",             AGPUPixelFormatType_Sint,        64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
    { GPU_TEXTURE_FORMAT_RG32_FLOAT,                "RG32Float",            AGPUPixelFormatType_Float,       64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
    //{ VGPU_PIXEL_FORMAT_RGBA16_UNORM,             "RGBA16",               AGPUPixelFormatType_Unorm,       64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    //{ VGPU_PIXEL_FORMAT_RGBA16_SNORM,             "RGBA16S",              AGPUPixelFormatType_Snorm,       64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { GPU_TEXTURE_FORMAT_RGBA16_UINT,               "RGBA16Uint",           AGPUPixelFormatType_Uint,        64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { GPU_TEXTURE_FORMAT_RGBA16_SINT,               "RGBA16Sint",           AGPUPixelFormatType_Sint,        64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { GPU_TEXTURE_FORMAT_RGBA16_FLOAT,              "RGBA16Float",          AGPUPixelFormatType_Float,       64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},

    // 128-Bit Pixel Formats
    { GPU_TEXTURE_FORMAT_RGBA32_UINT,               "RGBA32Uint",           AGPUPixelFormatType_Uint,        128,        {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},
    { GPU_TEXTURE_FORMAT_RGBA32_SINT,               "RGBA32Sint",           AGPUPixelFormatType_Sint,        128,        {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},
    { GPU_TEXTURE_FORMAT_RGBA32_FLOAT,              "RGBA32Float",          AGPUPixelFormatType_Float,       128,        {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},

    // Depth-stencil
    { GPU_TEXTURE_FORMAT_DEPTH16_UNORM,             "Depth16Unorm",         AGPUPixelFormatType_Unorm,       16,         {1, 1, 2, 1, 1},        {16, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_DEPTH32_FLOAT,             "Depth32Float",         AGPUPixelFormatType_Float,       32,         {1, 1, 4, 1, 1},        {32, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_DEPTH24_PLUS,              "Depth24Plus",          AGPUPixelFormatType_Unorm,       32,         {1, 1, 4, 1, 1},        {24, 8, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8,     "Depth32FloatStencil8", AGPUPixelFormatType_Float,       32,         {1, 1, 4, 1, 1},        {32, 8, 0, 0, 0, 0}},

    // Compressed formats
    { GPU_TEXTURE_FORMAT_BC1RGBA_UNORM,             "BC1RGBAUnorm",         AGPUPixelFormatType_Unorm,        4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC1RGBA_UNORM_SRGB,        "BC1RGBAUnormSrgb",     AGPUPixelFormatType_UnormSrgb,    4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC2RGBA_UNORM,             "BC2RGBAUnorm",         AGPUPixelFormatType_Unorm,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC2RGBA_UNORM_SRGB,        "BC2RGBAUnormSrgb",     AGPUPixelFormatType_UnormSrgb,    8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC3RGBA_UNORM,             "BC3RGBAUnorm",         AGPUPixelFormatType_Unorm,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC3RGBA_UNORM_SRGB,        "BC3RGBAUnormSrgb",     AGPUPixelFormatType_UnormSrgb,    8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC4R_UNORM,                "BC4RUnorm",            AGPUPixelFormatType_Unorm,        4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC4R_SNORM,                "BC4RSnorm",            AGPUPixelFormatType_Snorm,        4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC5RG_UNORM,               "BC5RGUnorm",           AGPUPixelFormatType_Unorm,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC5RG_SNORM,               "BC5RGSnorm",           AGPUPixelFormatType_Snorm,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC6HRGB_UFLOAT,            "BC6HRGBUfloat",        AGPUPixelFormatType_Float,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC6HRGB_SFLOAT,            "BC6HRGBSfloat",        AGPUPixelFormatType_Float,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC7RGBA_UNORM,             "BC7RGBAUnorm",         AGPUPixelFormatType_Unorm,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC7RGBA_UNORM_SRGB,        "BC7RGBAUnorm",         AGPUPixelFormatType_UnormSrgb,    8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},

    /*
    // Compressed PVRTC Pixel Formats
    { VGPU_PIXEL_FORMAT_PVRTC_RGB2,                 "PVRTC_RGB2",           AGPUPixelFormatType_Unorm,       2,          {8, 4, 8, 2, 2},        {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_PVRTC_RGBA2,                "PVRTC_RGBA2",          AGPUPixelFormatType_Unorm,       2,          {8, 4, 8, 2, 2},        {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_PVRTC_RGB4,                 "PVRTC_RGB4",           AGPUPixelFormatType_Unorm,       4,          {4, 4, 8, 2, 2},        {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_PVRTC_RGBA4,                "PVRTC_RGBA4",          AGPUPixelFormatType_Unorm,       4,          {4, 4, 8, 2, 2},        {0, 0, 0, 0, 0, 0}},

    // Compressed ETC Pixel Formats
    { VGPU_PIXEL_FORMAT_ETC2_RGB8,                  "ETC2_RGB8",            AGPUPixelFormatType_Unorm,       4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_ETC2_RGB8A1,                "ETC2_RGB8A1",          AGPUPixelFormatType_Unorm,       4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},

    // Compressed ASTC Pixel Formats
    { VGPU_PIXEL_FORMAT_ASTC4x4,                    "ASTC4x4",              AGPUPixelFormatType_Unorm,       8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_ASTC5x5,                    "ASTC5x5",              AGPUPixelFormatType_Unorm,       6,          {5, 5, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_ASTC6x6,                    "ASTC6x6",              AGPUPixelFormatType_Unorm,       4,          {6, 6, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPU_PIXEL_FORMAT_ASTC8x5,                    "ASTC8x5",              AGPUPixelFormatType_Unorm,       4,          {8, 5, 16, 1, 1},       {0, 0, 0, 0, 0, 0} },
    { VGPU_PIXEL_FORMAT_ASTC8x6,                    "ASTC8x6",              AGPUPixelFormatType_Unorm,       3,          {8, 6, 16, 1, 1},       {0, 0, 0, 0, 0, 0} },
    { VGPU_PIXEL_FORMAT_ASTC8x8,                    "ASTC8x8",              AGPUPixelFormatType_Unorm,       3,          {8, 8, 16, 1, 1},       {0, 0, 0, 0, 0, 0} },
    { VGPU_PIXEL_FORMAT_ASTC10x10,                  "ASTC10x10",            AGPUPixelFormatType_Unorm,       3,          {10, 10, 16, 1, 1},     {0, 0, 0, 0, 0, 0} },
    { VGPU_PIXEL_FORMAT_ASTC12x12,                  "ASTC12x12",            AGPUPixelFormatType_Unorm,       3,          {12, 12, 16, 1, 1},     {0, 0, 0, 0, 0, 0} },*/
};

uint32_t agpuGetFormatBitsPerPixel(AGPUPixelFormat format)
{
    VGPU_ASSERT(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].bitsPerPixel;
}

uint32_t agpuGetFormatBlockSize(AGPUPixelFormat format)
{
    VGPU_ASSERT(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].compression.blockSize;
}

uint32_t agpuGetFormatBlockWidth(AGPUPixelFormat format)
{
    VGPU_ASSERT(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].compression.blockWidth;
}

uint32_t agpuGetFormatBlockHeight(AGPUPixelFormat format)
{
    VGPU_ASSERT(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].compression.blockHeight;
}

AGPUPixelFormatType agpuGetFormatType(AGPUPixelFormat format)
{
    VGPU_ASSERT(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].type;
}

bool agpuIsDepthFormat(AGPUPixelFormat format)
{
    VGPU_ASSERT(FormatDesc[format].format == format);
    return FormatDesc[format].bits.depth > 0;
}

bool agpuIsStencilFrmat(AGPUPixelFormat format)
{
    VGPU_ASSERT(FormatDesc[format].format == format);
    return FormatDesc[format].bits.stencil > 0;
}

bool agpuIsDepthStencilFormat(AGPUPixelFormat format)
{
    return agpuIsDepthFormat(format) || agpuIsStencilFrmat(format);
}

bool agpuIsCompressedFormat(AGPUPixelFormat format)
{
    VGPU_ASSERT(FormatDesc[format].format == format);
    return format >= GPU_TEXTURE_FORMAT_BC1RGBA_UNORM && format <= GPU_TEXTURE_FORMAT_BC7RGBA_UNORM_SRGB;
}

const char* agpuGetFormatName(AGPUPixelFormat format)
{
    VGPU_ASSERT(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].name;
}

bool agpuIsSrgbFormat(AGPUPixelFormat format) {
    return (agpuGetFormatType(format) == AGPUPixelFormatType_UnormSrgb);
}
