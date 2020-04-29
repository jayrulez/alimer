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
static const char* vgpu_log_priority_prefixes[_GPULogLevel_Count] = {
    NULL,
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG",
    "TRACE",
};

static void gpu_default_log_callback(void* user_data, GPULogLevel level, const char* message);

#ifdef _DEBUG
static GPULogLevel s_log_level = GPULogLevel_Debug;
#else
static GPULogLevel s_log_level = GPULogLevel_Off;
#endif
static GPULogCallback s_log_function = gpu_default_log_callback;
static void* s_log_user_data = NULL;

void gpuSetLogLevel(GPULogLevel level) {
    s_log_level = level;
}

void gpuSetLogCallback(GPULogCallback callback, void* user_data) {
    s_log_function = callback;
    s_log_user_data = user_data;
}

void gpu_default_log_callback(void* user_data, GPULogLevel level, const char* message) {
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

void gpuLog(GPULogLevel level, const char* format, ...) {
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

GPUBackendType gpu_get_default_platform_backend(void) {
#if defined(_WIN32) || defined(_WIN64)
    if (gpu_is_backend_supported(GPUBackendType_D3D12)) {
        return GPUBackendType_D3D12;
    }

    if (gpu_is_backend_supported(GPUBackendType_Vulkan)) {
        return GPUBackendType_Vulkan;
    }

    if (gpu_is_backend_supported(GPUBackendType_D3D11)) {
        return GPUBackendType_D3D11;
    }

    if (gpu_is_backend_supported(GPUBackendType_OpenGL)) {
        return GPUBackendType_OpenGL;
    }

    return GPUBackendType_Null;
#elif defined(__linux__) || defined(__ANDROID__)
    return GPUBackendType_Vulkan;
#elif defined(__APPLE__)
    return GPUBackendType_Vulkan;
#else
    return GPUBackendType_OpenGL;
#endif
}

bool gpu_is_backend_supported(GPUBackendType backend) {
    if (backend == GPUBackendType_Default) {
        backend = gpu_get_default_platform_backend();
    }

    switch (backend)
    {
    case GPUBackendType_Null:
        return true;
#if defined(GPU_VK_BACKEND) && TODO_VK
    case GPU_BACKEND_VULKAN:
        return gpu_vk_supported();
#endif
#if defined(GPU_D3D12_BACKEND)
    case GPUBackendType_D3D12:
        return d3d12_driver.supported();
#endif 

#if defined(GPU_D3D11_BACKEND)
    case GPUBackendType_D3D11:
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

bool gpuInit(const GPUInitConfig* config) {
    VGPU_ASSERT(config);
#if defined(GPU_D3D12_BACKEND)
    if (d3d12_driver.supported()) {
        if (!d3d12_driver.init(config)) {
            return false;
        }
    }
#endif

#if defined(GPU_D3D11_BACKEND)
    if (d3d11_driver.supported()) {
        if (!d3d11_driver.init(config)) {
            return false;
        }
    }
#endif

    return true;
}

void gpuShutdown(void) {
#if defined(GPU_D3D12_BACKEND)
    if (d3d12_driver.supported()) {
        d3d12_driver.shutdown();
    }
#endif

#if defined(GPU_D3D11_BACKEND)
    if (d3d11_driver.supported()) {
        d3d11_driver.shutdown();
    }
#endif
}

GPUSurface gpuCreateWin32Surface(void* hinstance, void* hwnd) {
#if defined(_WIN32)
    GPUSurface result = _VGPU_ALLOC_HANDLE(GPUSurfaceImpl);
#if defined(GPU_D3D12_BACKEND)
    result->d3d12 = d3d12_driver.create_surface_from_windows_hwnd(hinstance, hwnd);
#endif

#if defined(GPU_D3D11_BACKEND)
    result->d3d11 = d3d11_driver.create_surface_from_windows_hwnd(hinstance, hwnd);
#endif

#else
    gpu_log_error("Cannot create Win32 surface on non windows OS");
    return nullptr;
#endif

    return result;
}

GPUDevice gpuDeviceCreate(const GPUDeviceDescriptor* desc)
{
    VGPU_ASSERT(desc);

    GPUBackendType backend = desc->preferredBackend;
    if (backend == GPUBackendType_Default) {
        backend = gpu_get_default_platform_backend();
    }

    GPUDevice device = NULL;
    switch (backend)
    {
    case GPUBackendType_Null:
        break;

#if defined(GPU_VK_BACKEND) && TODO_VK
    case GPU_BACKEND_VULKAN:
        s_renderer = vk_gpu_create_renderer();
        break;
#endif

#if defined(GPU_D3D12_BACKEND)
    case GPUBackendType_D3D12:
        device = d3d12_driver.createDevice(desc);
        break;
#endif

#if defined(GPU_D3D11_BACKEND)
    case GPUBackendType_D3D11:
        device = d3d11_driver.createDevice(desc);
        break;
#endif
    }

    if (device == NULL) {
        return NULL;
    }

    return device;
}

void gpuDeviceDestroy(GPUDevice device) {
    if (device == NULL) {
        return;
    }

    device->destroyDevice(device);
}

void gpuDeviceWaitIdle(GPUDevice device) {
    device->waitIdle(device->renderer);
}

GPUBackendType gpuQueryBackend(GPUDevice device) {
    VGPU_ASSERT(device);
    return device->query_caps(device->renderer).backend;
}

GPUDeviceCapabilities gpuQueryCaps(GPUDevice device) {
    VGPU_ASSERT(device);
    return device->query_caps(device->renderer);
}

GPUTextureFormat gpuGetPreferredSwapChainFormat(GPUDevice device, GPUSurface surface)
{
    VGPU_ASSERT(device);
    VGPU_ASSERT(surface);
    return device->getPreferredSwapChainFormat(device->renderer, surface);
}

GPUTextureFormat gpuGetDefaultDepthFormat(GPUDevice device)
{
    VGPU_ASSERT(device);
    return device->getDefaultDepthFormat(device->renderer);
}

GPUTextureFormat gpuGetDefaultDepthStencilFormat(GPUDevice device)
{
    VGPU_ASSERT(device);
    return device->getDefaultDepthStencilFormat(device->renderer);
}

static GPUSwapChainDescriptor SwapChainDescriptor_Default(const GPUSwapChainDescriptor* desc) {
    GPUSwapChainDescriptor def = *desc;
    def.usage = _vgpu_def(desc->usage, GPUTextureUsage_OutputAttachment);
    def.format = _vgpu_def(desc->format, GPUTextureFormat_BGRA8UnormSrgb);
    def.width = _vgpu_def(desc->width, 1);
    def.height = _vgpu_def(desc->height, 1);
    def.presentMode = _vgpu_def(desc->presentMode, GPUPresentMode_Fifo);
    return def;
}

GPUSwapChain gpuDeviceCreateSwapChain(GPUDevice device, GPUSurface surface, const GPUSwapChainDescriptor* descriptor) {
    VGPU_ASSERT(device);
    VGPU_ASSERT(surface);
    VGPU_ASSERT(descriptor);

    GPUSwapChainDescriptor descDefault = SwapChainDescriptor_Default(descriptor);
    return device->createSwapChain(device->renderer, surface, &descDefault);
}

void gpuDeviceDestroySwapChain(GPUDevice device, GPUSwapChain swapChain) {
    VGPU_ASSERT(device);
    VGPU_ASSERT(swapChain);
    device->destroySwapChain(device->renderer, swapChain);
}

GPUTextureView gpuSwapChainGetCurrentTextureView(GPUSwapChain swapChain)
{
    return swapChain->getCurrentTextureView(swapChain->backend);
}

void gpuSwapChainPresent(GPUSwapChain swapChain)
{
    swapChain->present(swapChain->backend);
}


/* Texture */
static GPUTextureDescriptor TextureDescriptor_Default(const GPUTextureDescriptor* desc) {
    GPUTextureDescriptor def = *desc;
    def.type = _vgpu_def(desc->type, GPUTextureType_2D);
    def.format = _vgpu_def(def.format, GPUTextureFormat_RGBA8Unorm);
    def.size.width = _vgpu_def(desc->size.width, 1);
    def.size.height = _vgpu_def(desc->size.height, 1);
    def.size.depth = _vgpu_def(desc->size.depth, 1);
    def.mipLevelCount = _vgpu_def(def.mipLevelCount, 1);
    def.sampleCount = _vgpu_def(def.sampleCount, GPUSampleCount_Count1);
    return def;
}


GPUTexture gpuDeviceCreateTexture(GPUDevice device, const GPUTextureDescriptor* descriptor) {
    VGPU_ASSERT(device);
    VGPU_ASSERT(descriptor);

    GPUTextureDescriptor desc_def = TextureDescriptor_Default(descriptor);
    return device->createTexture(device->renderer, &desc_def);
}

void gpuDeviceDestroyTexture(GPUDevice device, GPUTexture texture) {
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
    GPUTextureFormat format;
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
    // format                                       name,                   type                                bpp         compression             bits
    { GPU_TEXTURE_FORMAT_UNDEFINED,                 "Undefined",            VGPU_PIXEL_FORMAT_TYPE_UNKNOWN,     0,          {0, 0, 0, 0, 0},        {0, 0, 0, 0, 0, 0}},
    // 8-bit pixel formats
    { GPU_TEXTURE_FORMAT_R8_UNORM,                  "R8Unorm",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_R8_SNORM,                  "R8Snorm",              VGPU_PIXEL_FORMAT_TYPE_SNORM,       8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_R8_UINT,                   "R8Uint",               VGPU_PIXEL_FORMAT_TYPE_UINT,        8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_R8_SINT,                   "R8Sint",               VGPU_PIXEL_FORMAT_TYPE_SINT,        8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},

    // 16-bit pixel formats
    //{ GPU_TEXTURE_FORMAT_R16_UNORM,               "R16Unorm",             VGPU_PIXEL_FORMAT_TYPE_UNORM,       16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    //{ GPU_TEXTURE_FORMAT_R16_SNORM,               "R16Snorm",             VGPU_PIXEL_FORMAT_TYPE_SNORM,       16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_R16_UINT,                  "R16Uint",              VGPU_PIXEL_FORMAT_TYPE_UINT,        16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_R16_SINT,                  "R16Sint",              VGPU_PIXEL_FORMAT_TYPE_SINT,        16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_R16_FLOAT,                 "R16Float",             VGPU_PIXEL_FORMAT_TYPE_FLOAT,       16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_RG8_UNORM,                 "RG8Unorm",             VGPU_PIXEL_FORMAT_TYPE_UNORM,       16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    { GPU_TEXTURE_FORMAT_RG8_SNORM,                 "RG8Snorm",             VGPU_PIXEL_FORMAT_TYPE_SNORM,       16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    { GPU_TEXTURE_FORMAT_RG8_UINT,                  "RG8Uint",              VGPU_PIXEL_FORMAT_TYPE_UINT,        16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    { GPU_TEXTURE_FORMAT_RG8_SINT,                  "RG8Sint",              VGPU_PIXEL_FORMAT_TYPE_SINT,        16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},

    // Packed 16-bit pixel formats
    //{ VGPU_PIXEL_FORMAT_R5G6B5_UNORM,             "R5G6B5",             VGPU_PIXEL_FORMAT_TYPE_UNORM,       16,         {1, 1, 2, 1, 1},        {0, 0, 5, 6, 5, 0}},
    //{ VGPU_PIXEL_FORMAT_RGBA4_UNORM,              "RGBA4",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       16,         {1, 1, 2, 1, 1},        {0, 0, 4, 4, 4, 4}},

    // 32-bit pixel formats
    { GPU_TEXTURE_FORMAT_R32_UINT,              "R32Uint",              VGPU_PIXEL_FORMAT_TYPE_UINT,            32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_R32_SINT,              "R32Sint",              VGPU_PIXEL_FORMAT_TYPE_SINT,            32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_R32_FLOAT,             "R32Float",             VGPU_PIXEL_FORMAT_TYPE_FLOAT,           32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
    //{ VGPUTextureFormat_RG16Unorm,            "RG16Unorm",            VGPU_PIXEL_FORMAT_TYPE_UNORM,           32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    //{ VGPUTextureFormat_RG16Snorm,            "RG16Snorm",            VGPU_PIXEL_FORMAT_TYPE_SNORM,           32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { GPU_TEXTURE_FORMAT_RG16_UINT,             "RG16Uint",             VGPU_PIXEL_FORMAT_TYPE_UINT,            32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { GPU_TEXTURE_FORMAT_RG16_SINT,             "RG16Sint",             VGPU_PIXEL_FORMAT_TYPE_SINT,            32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { GPU_TEXTURE_FORMAT_RG16_FLOAT,            "RG16Float",            VGPU_PIXEL_FORMAT_TYPE_FLOAT,           32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { GPUTextureFormat_RGBA8Unorm,              "RGBA8Unorm",           VGPU_PIXEL_FORMAT_TYPE_UNORM,           32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { GPUTextureFormat_RGBA8UnormSrgb,          "RGBA8UnormSrgb",       VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,      32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { GPU_TEXTURE_FORMAT_RGBA8_SNORM,           "RGBA8Snorm",           VGPU_PIXEL_FORMAT_TYPE_SNORM,           32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { GPU_TEXTURE_FORMAT_RGBA8_UINT,            "RGBA8Uint",            VGPU_PIXEL_FORMAT_TYPE_UINT,            32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { GPU_TEXTURE_FORMAT_RGBA8_SINT,            "RGBA8Sint",            VGPU_PIXEL_FORMAT_TYPE_SINT,            32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { GPUTextureFormat_BGRA8Unorm,              "BGRA8Unorm",           VGPU_PIXEL_FORMAT_TYPE_UNORM,           32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { GPUTextureFormat_BGRA8UnormSrgb,          "BGRA8UnormSrgb",       VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,      32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},

    // Packed 32-Bit Pixel formats
    { GPU_TEXTURE_FORMAT_RGB10A2_UNORM,             "RGB10A2Unorm",              VGPU_PIXEL_FORMAT_TYPE_UNORM,       32,         {1, 1, 4, 1, 1},        {0, 0, 10, 10, 10, 2}},
    //{ VGPU_PIXEL_FORMAT_RGB10A2_UINT,             "RGB10A2U",             VGPU_PIXEL_FORMAT_TYPE_UINT,        32,         {1, 1, 4, 1, 1},        {0, 0, 10, 10, 10, 2}},
    { GPU_TEXTURE_FORMAT_RG11B10_FLOAT,             "RG11B10Float",             VGPU_PIXEL_FORMAT_TYPE_FLOAT,       32,         {1, 1, 4, 1, 1},        {0, 0, 11, 11, 10, 0}},
    //{ VGPU_PIXEL_FORMAT_RGB9E5_FLOAT,             "RGB9E5F",              VGPU_PIXEL_FORMAT_TYPE_FLOAT,       32,         {1, 1, 4, 1, 1},        {0, 0, 9, 9, 9, 5}},

    // 64-Bit Pixel Formats
    { GPU_TEXTURE_FORMAT_RG32_UINT,                 "RG32Uint",            VGPU_PIXEL_FORMAT_TYPE_UINT,        64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
    { GPU_TEXTURE_FORMAT_RG32_SINT,                 "RG32Sint",            VGPU_PIXEL_FORMAT_TYPE_SINT,        64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
    { GPU_TEXTURE_FORMAT_RG32_FLOAT,                "RG32Float",           VGPU_PIXEL_FORMAT_TYPE_FLOAT,       64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
    //{ VGPU_PIXEL_FORMAT_RGBA16_UNORM,             "RGBA16",               VGPU_PIXEL_FORMAT_TYPE_UNORM,       64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    //{ VGPU_PIXEL_FORMAT_RGBA16_SNORM,             "RGBA16S",              VGPU_PIXEL_FORMAT_TYPE_SNORM,       64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { GPU_TEXTURE_FORMAT_RGBA16_UINT,               "RGBA16Uint",          VGPU_PIXEL_FORMAT_TYPE_UINT,        64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { GPU_TEXTURE_FORMAT_RGBA16_SINT,               "RGBA16Sint",          VGPU_PIXEL_FORMAT_TYPE_SINT,        64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { GPU_TEXTURE_FORMAT_RGBA16_FLOAT,              "RGBA16Float",         VGPU_PIXEL_FORMAT_TYPE_FLOAT,       64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},

    // 128-Bit Pixel Formats
    { GPU_TEXTURE_FORMAT_RGBA32_UINT,               "RGBA32Uint",           VGPU_PIXEL_FORMAT_TYPE_UINT,        128,        {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},
    { GPU_TEXTURE_FORMAT_RGBA32_SINT,               "RGBA32Sint",           VGPU_PIXEL_FORMAT_TYPE_SINT,        128,        {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},
    { GPU_TEXTURE_FORMAT_RGBA32_FLOAT,              "RGBA32Float",          VGPU_PIXEL_FORMAT_TYPE_FLOAT,       128,        {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},

    // Depth-stencil
    { GPU_TEXTURE_FORMAT_DEPTH16_UNORM,             "Depth16Unorm",         VGPU_PIXEL_FORMAT_TYPE_UNORM,       16,         {1, 1, 2, 1, 1},        {16, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_DEPTH32_FLOAT,             "Depth32Float",         VGPU_PIXEL_FORMAT_TYPE_FLOAT,       32,         {1, 1, 4, 1, 1},        {32, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_DEPTH24_PLUS,              "Depth24Plus",          VGPU_PIXEL_FORMAT_TYPE_UNORM,       32,         {1, 1, 4, 1, 1},        {24, 8, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8,     "Depth32FloatStencil8", VGPU_PIXEL_FORMAT_TYPE_FLOAT,       32,         {1, 1, 4, 1, 1},        {32, 8, 0, 0, 0, 0}},

    // Compressed formats
    { GPU_TEXTURE_FORMAT_BC1RGBA_UNORM,             "BC1RGBAUnorm",         VGPU_PIXEL_FORMAT_TYPE_UNORM,        4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC1RGBA_UNORM_SRGB,        "BC1RGBAUnormSrgb",     VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,    4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC2RGBA_UNORM,             "BC2RGBAUnorm",         VGPU_PIXEL_FORMAT_TYPE_UNORM,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC2RGBA_UNORM_SRGB,        "BC2RGBAUnormSrgb",     VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,    8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC3RGBA_UNORM,             "BC3RGBAUnorm",         VGPU_PIXEL_FORMAT_TYPE_UNORM,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC3RGBA_UNORM_SRGB,        "BC3RGBAUnormSrgb",     VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,    8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC4R_UNORM,                "BC4RUnorm",            VGPU_PIXEL_FORMAT_TYPE_UNORM,        4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC4R_SNORM,                "BC4RSnorm",            VGPU_PIXEL_FORMAT_TYPE_SNORM,        4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC5RG_UNORM,               "BC5RGUnorm",           VGPU_PIXEL_FORMAT_TYPE_UNORM,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC5RG_SNORM,               "BC5RGSnorm",           VGPU_PIXEL_FORMAT_TYPE_SNORM,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC6HRGB_UFLOAT,            "BC6HRGBUfloat",        VGPU_PIXEL_FORMAT_TYPE_FLOAT,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC6HRGB_SFLOAT,            "BC6HRGBSfloat",        VGPU_PIXEL_FORMAT_TYPE_FLOAT,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC7RGBA_UNORM,             "BC7RGBAUnorm",         VGPU_PIXEL_FORMAT_TYPE_UNORM,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { GPU_TEXTURE_FORMAT_BC7RGBA_UNORM_SRGB,        "BC7RGBAUnorm",         VGPU_PIXEL_FORMAT_TYPE_UNORM_SRGB,    8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},

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

uint32_t vgpu_get_format_bits_per_pixel(GPUTextureFormat format)
{
    assert(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].bitsPerPixel;
}

uint32_t vgpu_get_format_block_size(GPUTextureFormat format)
{
    assert(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].compression.blockSize;
}

uint32_t vgpuGetFormatBlockWidth(GPUTextureFormat format)
{
    assert(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].compression.blockWidth;
}

uint32_t vgpuGetFormatBlockHeight(GPUTextureFormat format)
{
    assert(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].compression.blockHeight;
}

vgpu_pixel_format_type vgpu_get_format_type(GPUTextureFormat format)
{
    assert(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].type;
}

bool gpuIsDepthFormat(GPUTextureFormat format)
{
    VGPU_ASSERT(FormatDesc[format].format == format);
    return FormatDesc[format].bits.depth > 0;
}

bool vgpuIsStencilFrmat(GPUTextureFormat format)
{
    VGPU_ASSERT(FormatDesc[format].format == format);
    return FormatDesc[format].bits.stencil > 0;
}

bool gpuIsDepthStencilFormat(GPUTextureFormat format)
{
    return gpuIsDepthFormat(format) || vgpuIsStencilFrmat(format);
}

bool vgpuIsCompressedFormat(GPUTextureFormat format)
{
    VGPU_ASSERT(FormatDesc[format].format == format);
    return format >= GPU_TEXTURE_FORMAT_BC1RGBA_UNORM && format <= GPU_TEXTURE_FORMAT_BC7RGBA_UNORM_SRGB;
}

const char* gpuGetFormatName(GPUTextureFormat format)
{
    VGPU_ASSERT(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].name;
}
