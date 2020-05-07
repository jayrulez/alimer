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
static const char* vgpu_log_priority_prefixes[AGPULogLevel_Count] = {
    NULL,
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG",
    "TRACE",
};

static void _s_agpuDefaultLogCallback(void* user_data, AGPULogLevel level, const char* message);

#ifdef _DEBUG
static AGPULogLevel s_log_level = AGPULogLevel_Debug;
#else
static AGPULogLevel s_log_level = AGPULogLevel_Off;
#endif
static GPULogCallback s_log_function = _s_agpuDefaultLogCallback;
static void* s_log_user_data = NULL;

void agpuSetLogLevel(AGPULogLevel level) {
    s_log_level = level;
}

void agpuSetLogCallback(GPULogCallback callback, void* user_data) {
    s_log_function = callback;
    s_log_user_data = user_data;
}

void _s_agpuDefaultLogCallback(void* user_data, AGPULogLevel level, const char* message) {
#if defined(_WIN32)
    size_t length = strlen(vgpu_log_priority_prefixes[level]) + 2 + strlen(message) + 1 + 1 + 1;
    char* output = VGPU_ALLOCA(char, length);
    snprintf(output, length, "%s: %s\r\n", vgpu_log_priority_prefixes[level], message);

    const int buffer_size = MultiByteToWideChar(CP_UTF8, 0, output, (int)length, NULL, 0);
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
    case AGPULogLevel_Error:
    case AGPULogLevel_Warn:
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

void agpuLog(AGPULogLevel level, const char* format, ...) {
    if (s_log_level == AGPULogLevel_Off || level == AGPULogLevel_Off) {
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

agpu_backend_type agpu_get_default_platform_backend(void) {
#if defined(_WIN32) || defined(_WIN64)
    if (agpu_is_backend_supported(AGPU_BACKEND_TYPE_D3D12)) {
        return AGPU_BACKEND_TYPE_D3D12;
    }

    if (agpu_is_backend_supported(AGPU_BACKEND_TYPE_VULKAN)) {
        return AGPU_BACKEND_TYPE_VULKAN;
    }

    if (agpu_is_backend_supported(AGPU_BACKEND_TYPE_D3D11)) {
        return AGPU_BACKEND_TYPE_D3D11;
    }

    if (agpu_is_backend_supported(AGPU_BACKEND_TYPE_OPENGL)) {
        return AGPU_BACKEND_TYPE_OPENGL;
    }

    return AGPU_BACKEND_TYPE_NULL;
#elif defined(__linux__) || defined(__ANDROID__)
    return AGPU_BACKEND_TYPE_VULKAN;
#elif defined(__APPLE__)
    return AGPU_BACKEND_TYPE_VULKAN;
#else
    return AGPU_BACKEND_TYPE_OPENGL;
#endif
}

bool agpu_is_backend_supported(agpu_backend_type backend) {
    if (backend == AGPU_BACKEND_TYPE_DEFAULT) {
        backend = agpu_get_default_platform_backend();
    }

    switch (backend)
    {
    case AGPU_BACKEND_TYPE_NULL:
        return true;
#if defined(GPU_VK_BACKEND) && TODO_VK
    case AGPUBackendType_Vulkan:
        return gpu_vk_supported();
#endif
#if defined(GPU_D3D12_BACKEND) && defined(TODO_D3D12)
    case AGPUBackendType_D3D12:
        return d3d12_driver.supported();
#endif 

#if defined(GPU_D3D11_BACKEND) && defined(TODO_D3D11)
    case AGPUBackendType_D3D11:
        return d3d11_driver.supported();
#endif

#if defined(GPU_GL_BACKEND)
    case AGPU_BACKEND_TYPE_OPENGL:
        return gl_driver.supported();
#endif // defined(GPU_GL_BACKEND)

    default:
        return false;
    }
}

static agpu_renderer* s_renderer = NULL;

bool agpu_init(const agpu_config* config)
{
    AGPU_ASSERT(config);
    if (s_renderer) {
        return true;
    }

    agpu_backend_type backend = config->preferred_backend;
    if (backend == AGPU_BACKEND_TYPE_DEFAULT) {
        backend = agpu_get_default_platform_backend();
    }

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

#if defined(GPU_D3D11_BACKEND) && defined(TODO_D3D11)
    case AGPU_BACKEND_TYPE_D3D11:
        device = d3d11_driver.create_device(info);
        break;
#endif

#if defined(GPU_GL_BACKEND)
    case AGPU_BACKEND_TYPE_OPENGL:
        s_renderer = gl_driver.create_renderer();
        break;
#endif
    }

    if (s_renderer == NULL || !s_renderer->init(config)) {
        s_renderer = NULL;
        return false;
    }

    return true;
}

void agpu_shutdown(void) {
    if (s_renderer == NULL) {
        return;
    }

    s_renderer->shutdown();
    s_renderer = NULL;
}

void agpu_frame_begin(void) {
    s_renderer->frame_wait();
}

void agpu_frame_finish(void) {
    s_renderer->frame_finish();
}

agpu_backend_type agpu_query_backend(void) {
    return s_renderer->query_backend();
}

void  agpu_get_limits(agpu_limits* limits) {
    s_renderer->get_limits(limits);
}

AGPUPixelFormat agpu_get_default_depth_format(void)
{
    return s_renderer->get_default_depth_format();
}

AGPUPixelFormat agpu_get_default_depth_stencil_format(void)
{
    return s_renderer->get_default_depth_stencil_format();
}

/* Texture */
static agpu_texture_info texture_info_default(const agpu_texture_info* info) {
    agpu_texture_info def = *info;
    def.type = _agpu_def(info->type, AGPUTextureType_2D);
    def.format = _agpu_def(info->format, AGPUPixelFormat_RGBA8Unorm);
    def.size.width = _agpu_def(info->size.width, 1);
    def.size.height = _agpu_def(info->size.height, 1);
    def.size.depth = _agpu_def(info->size.depth, 1);
    def.mipLevelCount = _agpu_def(info->mipLevelCount, 1);
    def.sampleCount = _agpu_def(info->sampleCount, 1u);
    return def;
}

agpu_texture agpu_texture_create(const agpu_texture_info* info) {
    AGPU_ASSERT(info);
    agpu_texture_info info_def = texture_info_default(info);
    return s_renderer->create_texture(&info_def);
}

void agpu_texture_destroy(agpu_texture texture) {
    AGPU_ASSERT(texture);
    s_renderer->destroy_texture(texture);
}

/* Buffer */
agpu_buffer agpu_create_buffer(const agpu_buffer_info* info) {
    AGPU_ASSERT(info);
    return s_renderer->create_buffer(info);
}

void agpu_destroy_buffer(agpu_buffer buffer) {
    AGPU_ASSERT(buffer);
    s_renderer->destroy_buffer( buffer);
}

/* Shader */
agpu_shader agpu_create_shader(const agpu_shader_info* info) {
    AGPU_ASSERT(info);
    return s_renderer->create_shader(info);
}

void agpu_destroy_shader(agpu_shader shader) {
    AGPU_ASSERT(shader);
    s_renderer->destroy_shader(shader);
}

/* Pipeline */
static agpu_render_pipeline_info pipeline_info_default(const agpu_render_pipeline_info* info) {
    agpu_render_pipeline_info def = *info;
    def.primitive_topology = _agpu_def(info->primitive_topology, AGPU_PRIMITIVE_TOPOLOGY_TRIANGLES);
    return def;
}

agpu_pipeline agpu_create_render_pipeline(const agpu_render_pipeline_info* info) {
    AGPU_ASSERT(info);
    agpu_render_pipeline_info info_def = pipeline_info_default(info);
    return s_renderer->create_render_pipeline(&info_def);
}

void agpu_destroy_pipeline(agpu_pipeline pipeline) {
    AGPU_ASSERT(pipeline);
    s_renderer->destroy_pipeline(pipeline);
}

/*AGPUSampler agpuDeviceCreateSampler(GPUDevice device, const AGPUSamplerDescriptor* descriptor)
{
    VGPU_ASSERT(device);
    VGPU_ASSERT(descriptor);
    return NULL;
}

void agpuDeviceDestroySampler(GPUDevice device, AGPUSampler sampler)
{
    VGPU_ASSERT(device);
    VGPU_ASSERT(sampler);
}*/

/* CommandBuffer */
void agpu_set_pipeline(agpu_pipeline pipeline) {
    s_renderer->set_pipeline(pipeline);
}

void agpuCmdSetVertexBuffers(uint32_t slot, agpu_buffer buffer, uint64_t offset) {
    s_renderer->cmdSetVertexBuffer(slot, buffer, offset);
}

void agpuCmdSetIndexBuffer(agpu_buffer buffer, uint64_t offset) {
    s_renderer->cmdSetIndexBuffer(buffer, offset);
}

void agpuCmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex) {
    s_renderer->cmdDraw(vertexCount, instanceCount, firstVertex);
}

void agpuCmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex) {
    s_renderer->cmdDrawIndexed(indexCount, instanceCount, firstIndex);
}

#if TODO

/* Texture */

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
    { AGPUPixelFormat_Undefined,                    "Undefined",            AGPUPixelFormatType_Unknown,     0,          {0, 0, 0, 0, 0},        {0, 0, 0, 0, 0, 0}},
    // 8-bit pixel formats
    { AGPUPixelFormat_R8Unorm,                      "R8Unorm",              AGPUPixelFormatType_Unorm,       8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    { AGPUPixelFormat_R8Snorm,                      "R8Snorm",              AGPUPixelFormatType_Snorm,       8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    { AGPUPixelFormat_R8Uint,                       "R8Uint",               AGPUPixelFormatType_Uint,        8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    { AGPUPixelFormat_R8Sint,                       "R8Sint",               AGPUPixelFormatType_Sint,        8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},

    // 16-bit pixel formats
    //{ GPU_TEXTURE_FORMAT_R16_UNORM,               "R16Unorm",             AGPUPixelFormatType_Unorm,       16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    //{ GPU_TEXTURE_FORMAT_R16_SNORM,               "R16Snorm",             AGPUPixelFormatType_Snorm,       16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { AGPUPixelFormat_R16Uint,                      "R16Uint",              AGPUPixelFormatType_Uint,        16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { AGPUPixelFormat_R16Sint,                      "R16Sint",              AGPUPixelFormatType_Sint,        16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { AGPUPixelFormat_R16Float,                     "R16Float",             AGPUPixelFormatType_Float,       16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { AGPUPixelFormat_RG8Unorm,                     "RG8Unorm",             AGPUPixelFormatType_Unorm,       16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    { AGPUPixelFormat_RG8Snorm,                     "RG8Snorm",             AGPUPixelFormatType_Snorm,       16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    { AGPUPixelFormat_RG8Uint,                      "RG8Uint",              AGPUPixelFormatType_Uint,        16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    { AGPUPixelFormat_RG8Sint,                      "RG8Sint",              AGPUPixelFormatType_Sint,        16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},

    // Packed 16-bit pixel formats
    //{ VGPU_PIXEL_FORMAT_R5G6B5_UNORM,             "R5G6B5",               AGPUPixelFormatType_Unorm,       16,         {1, 1, 2, 1, 1},        {0, 0, 5, 6, 5, 0}},
    //{ VGPU_PIXEL_FORMAT_RGBA4_UNORM,              "RGBA4",                AGPUPixelFormatType_Unorm,       16,         {1, 1, 2, 1, 1},        {0, 0, 4, 4, 4, 4}},

    // 32-bit pixel formats
    { AGPUPixelFormat_R32Uint,                      "R32Uint",              AGPUPixelFormatType_Uint,            32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
    { AGPUPixelFormat_R32Sint,                      "R32Sint",              AGPUPixelFormatType_Sint,            32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
    { AGPUPixelFormat_R32Float,                     "R32Float",             AGPUPixelFormatType_Float,           32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
    //{ VGPUTextureFormat_RG16Unorm,                "RG16Unorm",            AGPUPixelFormatType_Unorm,           32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    //{ VGPUTextureFormat_RG16Snorm,                "RG16Snorm",            AGPUPixelFormatType_Snorm,           32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { AGPUPixelFormat_RG16Uint,                     "RG16Uint",             AGPUPixelFormatType_Uint,            32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { AGPUPixelFormat_RG16Sint,                     "RG16Sint",             AGPUPixelFormatType_Sint,            32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { AGPUPixelFormat_RG16Float,                    "RG16Float",            AGPUPixelFormatType_Float,           32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { AGPUPixelFormat_RGBA8Unorm,                   "RGBA8Unorm",           AGPUPixelFormatType_Unorm,           32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { AGPUPixelFormat_RGBA8UnormSrgb,               "RGBA8UnormSrgb",       AGPUPixelFormatType_UnormSrgb,      32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { AGPUPixelFormat_RGBA8Snorm,                   "RGBA8Snorm",           AGPUPixelFormatType_Snorm,           32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { AGPUPixelFormat_RGBA8Uint,                    "RGBA8Uint",            AGPUPixelFormatType_Uint,            32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { AGPUPixelFormat_RGBA8Sint,                    "RGBA8Sint",            AGPUPixelFormatType_Sint,            32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { AGPUPixelFormat_BGRA8Unorm,                   "BGRA8Unorm",           AGPUPixelFormatType_Unorm,           32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { AGPUPixelFormat_BGRA8UnormSrgb,               "BGRA8UnormSrgb",       AGPUPixelFormatType_UnormSrgb,      32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},

    // Packed 32-Bit Pixel formats
    { AGPUPixelFormat_RGB10A2Unorm,                 "RGB10A2Unorm",         AGPUPixelFormatType_Unorm,       32,         {1, 1, 4, 1, 1},        {0, 0, 10, 10, 10, 2}},
    //{ VGPU_PIXEL_FORMAT_RGB10A2_UINT,             "RGB10A2U",             AGPUPixelFormatType_Uint,        32,         {1, 1, 4, 1, 1},        {0, 0, 10, 10, 10, 2}},
    { AGPUPixelFormat_RG11B10Float,                 "RG11B10Float",         AGPUPixelFormatType_Float,       32,         {1, 1, 4, 1, 1},        {0, 0, 11, 11, 10, 0}},
    //{ VGPU_PIXEL_FORMAT_RGB9E5_FLOAT,             "RGB9E5F",              AGPUPixelFormatType_Float,       32,         {1, 1, 4, 1, 1},        {0, 0, 9, 9, 9, 5}},

    // 64-Bit Pixel Formats
    { AGPUPixelFormat_RG32Uint,                     "RG32Uint",             AGPUPixelFormatType_Uint,        64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
    { AGPUPixelFormat_RG32Sint,                     "RG32Sint",             AGPUPixelFormatType_Sint,        64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
    { AGPUPixelFormat_RG32Float,                    "RG32Float",            AGPUPixelFormatType_Float,       64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
    //{ VGPU_PIXEL_FORMAT_RGBA16_UNORM,             "RGBA16",               AGPUPixelFormatType_Unorm,       64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    //{ VGPU_PIXEL_FORMAT_RGBA16_SNORM,             "RGBA16S",              AGPUPixelFormatType_Snorm,       64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { AGPUPixelFormat_RGBA16Uint,                   "RGBA16Uint",           AGPUPixelFormatType_Uint,        64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { AGPUPixelFormat_RGBA16Sint,                   "RGBA16Sint",           AGPUPixelFormatType_Sint,        64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { AGPUPixelFormat_RGBA16Float,                  "RGBA16Float",          AGPUPixelFormatType_Float,       64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},

    // 128-Bit Pixel Formats
    { AGPUPixelFormat_RGBA32Uint,                   "RGBA32Uint",           AGPUPixelFormatType_Uint,        128,        {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},
    { AGPUPixelFormat_RGBA32Sint,                   "RGBA32Sint",           AGPUPixelFormatType_Sint,        128,        {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},
    { AGPUPixelFormat_RGBA32Float,                  "RGBA32Float",          AGPUPixelFormatType_Float,       128,        {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},

    // Depth-stencil
    { AGPUPixelFormat_Depth16Unorm,                 "Depth16Unorm",         AGPUPixelFormatType_Unorm,       16,         {1, 1, 2, 1, 1},        {16, 0, 0, 0, 0, 0}},
    { AGPUPixelFormat_Depth32Float,                 "Depth32Float",         AGPUPixelFormatType_Float,       32,         {1, 1, 4, 1, 1},        {32, 0, 0, 0, 0, 0}},
    { AGPUPixelFormat_Depth24Plus,                  "Depth24Plus",          AGPUPixelFormatType_Unorm,       32,         {1, 1, 4, 1, 1},        {24, 8, 0, 0, 0, 0}},
    { AGPUPixelFormat_Depth24PlusStencil8,          "Depth24PlusStencil8",  AGPUPixelFormatType_Float,       32,         {1, 1, 4, 1, 1},        {32, 8, 0, 0, 0, 0}},

    // Compressed formats
    { AGPUPixelFormat_BC1RGBAUnorm,                 "BC1RGBAUnorm",         AGPUPixelFormatType_Unorm,        4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { AGPUPixelFormat_BC1RGBAUnormSrgb,             "BC1RGBAUnormSrgb",     AGPUPixelFormatType_UnormSrgb,    4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { AGPUPixelFormat_BC2RGBAUnorm,                 "BC2RGBAUnorm",         AGPUPixelFormatType_Unorm,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { AGPUPixelFormat_BC2RGBAUnormSrgb,             "BC2RGBAUnormSrgb",     AGPUPixelFormatType_UnormSrgb,    8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { AGPUPixelFormat_BC3RGBAUnorm,                 "BC3RGBAUnorm",         AGPUPixelFormatType_Unorm,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { AGPUPixelFormat_BC3RGBAUnormSrgb,             "BC3RGBAUnormSrgb",     AGPUPixelFormatType_UnormSrgb,    8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { AGPUPixelFormat_BC4RUnorm,                    "BC4RUnorm",            AGPUPixelFormatType_Unorm,        4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { AGPUPixelFormat_BC4RSnorm,                    "BC4RSnorm",            AGPUPixelFormatType_Snorm,        4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { AGPUPixelFormat_BC5RGUnorm,                   "BC5RGUnorm",           AGPUPixelFormatType_Unorm,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { AGPUPixelFormat_BC5RGSnorm,                   "BC5RGSnorm",           AGPUPixelFormatType_Snorm,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { AGPUPixelFormat_BC6HRGBUFloat,                "BC6HRGBUFloat",        AGPUPixelFormatType_Float,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { AGPUPixelFormat_BC6HRGBSFloat,                "BC6HRGBSFloat",        AGPUPixelFormatType_Float,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { AGPUPixelFormat_BC7RGBAUnorm,                 "BC7RGBAUnorm",         AGPUPixelFormatType_Unorm,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { AGPUPixelFormat_BC7RGBAUnormSrgb,             "BC7RGBAUnormSrgb",     AGPUPixelFormatType_UnormSrgb,    8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},

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
    AGPU_ASSERT(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].bitsPerPixel;
}

uint32_t agpuGetFormatBlockSize(AGPUPixelFormat format)
{
    AGPU_ASSERT(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].compression.blockSize;
}

uint32_t agpuGetFormatBlockWidth(AGPUPixelFormat format)
{
    AGPU_ASSERT(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].compression.blockWidth;
}

uint32_t agpuGetFormatBlockHeight(AGPUPixelFormat format)
{
    AGPU_ASSERT(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].compression.blockHeight;
}

AGPUPixelFormatType agpuGetFormatType(AGPUPixelFormat format)
{
    AGPU_ASSERT(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].type;
}

bool agpuIsDepthFormat(AGPUPixelFormat format)
{
    AGPU_ASSERT(FormatDesc[format].format == format);
    return FormatDesc[format].bits.depth > 0;
}

bool agpuIsStencilFrmat(AGPUPixelFormat format)
{
    AGPU_ASSERT(FormatDesc[format].format == format);
    return FormatDesc[format].bits.stencil > 0;
}

bool agpuIsDepthStencilFormat(AGPUPixelFormat format)
{
    return agpuIsDepthFormat(format) || agpuIsStencilFrmat(format);
}

bool agpuIsCompressedFormat(AGPUPixelFormat format)
{
    AGPU_ASSERT(FormatDesc[format].format == format);
    return format >= AGPUPixelFormat_BC1RGBAUnorm && format <= AGPUPixelFormat_BC7RGBAUnormSrgb;
}

const char* agpuGetFormatName(AGPUPixelFormat format)
{
    AGPU_ASSERT(FormatDesc[(uint32_t)format].format == format);
    return FormatDesc[(uint32_t)format].name;
}

bool agpuIsSrgbFormat(AGPUPixelFormat format)
{
    return (agpuGetFormatType(format) == AGPUPixelFormatType_UnormSrgb);
}

AGPUPixelFormat agpuSrgbToLinearFormat(AGPUPixelFormat format)
{
    switch (format)
    {
    case AGPUPixelFormat_BC1RGBAUnormSrgb:
        return AGPUPixelFormat_BC1RGBAUnorm;
    case AGPUPixelFormat_BC2RGBAUnormSrgb:
        return AGPUPixelFormat_BC2RGBAUnorm;
    case AGPUPixelFormat_BC3RGBAUnormSrgb:
        return AGPUPixelFormat_BC3RGBAUnorm;
    case AGPUPixelFormat_BGRA8UnormSrgb:
        return AGPUPixelFormat_BGRA8Unorm;
    case AGPUPixelFormat_RGBA8UnormSrgb:
        return AGPUPixelFormat_RGBA8Unorm;
    case AGPUPixelFormat_BC7RGBAUnormSrgb:
        return AGPUPixelFormat_BC7RGBAUnorm;
    default:
        AGPU_ASSERT(agpuIsSrgbFormat(format) == false);
        return format;
    }
}

AGPUPixelFormat agpuLinearToSrgbFormat(AGPUPixelFormat format)
{
    switch (format)
    {
    case AGPUPixelFormat_BC1RGBAUnorm:
        return AGPUPixelFormat_BC1RGBAUnormSrgb;
    case AGPUPixelFormat_BC2RGBAUnorm:
        return AGPUPixelFormat_BC2RGBAUnormSrgb;
    case AGPUPixelFormat_BC3RGBAUnorm:
        return AGPUPixelFormat_BC3RGBAUnormSrgb;
    case AGPUPixelFormat_BGRA8Unorm:
        return AGPUPixelFormat_BGRA8UnormSrgb;
    case AGPUPixelFormat_RGBA8Unorm:
        return AGPUPixelFormat_RGBA8UnormSrgb;
    case AGPUPixelFormat_BC7RGBAUnorm:
        return AGPUPixelFormat_BC7RGBAUnormSrgb;
    default:
        return format;
    }
}


uint32_t agpuGetVertexFormatComponentsCount(AGPUVertexFormat format) {
    switch (format) {
    case AGPU_VERTEX_FORMAT_UCHAR4:
    case AGPU_VERTEX_FORMAT_CHAR4:
    case AGPU_VERTEX_FORMAT_UCHAR4NORM:
    case AGPU_VERTEX_FORMAT_CHAR4NORM:
    case AGPU_VERTEX_FORMAT_USHORT4:
    case AGPU_VERTEX_FORMAT_SHORT4:
    case AGPU_VERTEX_FORMAT_USHORT4NORM:
    case AGPU_VERTEX_FORMAT_SHORT4NORM:
    case AGPUVertexFormat_Half4:
    case AGPUVertexFormat_Float4:
    case AGPUVertexFormat_UInt4:
    case AGPUVertexFormat_Int4:
        return 4;
    case AGPUVertexFormat_Float3:
    case AGPUVertexFormat_UInt3:
    case AGPUVertexFormat_Int3:
        return 3;
    case AGPU_VERTEX_FORMAT_UCHAR2:
    case AGPU_VERTEX_FORMAT_CHAR2:
    case AGPU_VERTEX_FORMAT_UCHAR2NORM:
    case AGPU_VERTEX_FORMAT_CHAR2NORM:
    case AGPU_VERTEX_FORMAT_USHORT2:
    case AGPU_VERTEX_FORMAT_SHORT2:
    case AGPU_VERTEX_FORMAT_USHORT2NORM:
    case AGPU_VERTEX_FORMAT_SHORT2NORM:
    case AGPUVertexFormat_Half2:
    case AGPUVertexFormat_Float2:
    case AGPUVertexFormat_UInt2:
    case AGPUVertexFormat_Int2:
        return 2;
    case AGPUVertexFormat_Float:
    case AGPUVertexFormat_UInt:
    case AGPUVertexFormat_Int:
        return 1;
    default:
        AGPU_UNREACHABLE();
    }
}

uint32_t agpuGetVertexFormatComponentSize(AGPUVertexFormat format) {
    switch (format) {
    case AGPU_VERTEX_FORMAT_UCHAR2:
    case AGPU_VERTEX_FORMAT_UCHAR4:
    case AGPU_VERTEX_FORMAT_CHAR2:
    case AGPU_VERTEX_FORMAT_CHAR4:
    case AGPU_VERTEX_FORMAT_UCHAR2NORM:
    case AGPU_VERTEX_FORMAT_UCHAR4NORM:
    case AGPU_VERTEX_FORMAT_CHAR2NORM:
    case AGPU_VERTEX_FORMAT_CHAR4NORM:
        return 1u;
    case AGPU_VERTEX_FORMAT_USHORT2:
    case AGPU_VERTEX_FORMAT_USHORT4:
    case AGPU_VERTEX_FORMAT_USHORT2NORM:
    case AGPU_VERTEX_FORMAT_USHORT4NORM:
    case AGPU_VERTEX_FORMAT_SHORT2:
    case AGPU_VERTEX_FORMAT_SHORT4:
    case AGPU_VERTEX_FORMAT_SHORT2NORM:
    case AGPU_VERTEX_FORMAT_SHORT4NORM:
    case AGPUVertexFormat_Half2:
    case AGPUVertexFormat_Half4:
        return 2u;
    case AGPUVertexFormat_Float:
    case AGPUVertexFormat_Float2:
    case AGPUVertexFormat_Float3:
    case AGPUVertexFormat_Float4:
        return 4u;
    case AGPUVertexFormat_UInt:
    case AGPUVertexFormat_UInt2:
    case AGPUVertexFormat_UInt3:
    case AGPUVertexFormat_UInt4:
    case AGPUVertexFormat_Int:
    case AGPUVertexFormat_Int2:
    case AGPUVertexFormat_Int3:
    case AGPUVertexFormat_Int4:
        return 4u;
    default:
        AGPU_UNREACHABLE();
    }
}

uint32_t agpuGetVertexFormatSize(AGPUVertexFormat format) {
    return agpuGetVertexFormatComponentsCount(format) * agpuGetVertexFormatComponentSize(format);
}
