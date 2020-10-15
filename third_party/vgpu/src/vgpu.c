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

#include "vgpu_driver.h"
#include <stdio.h>
#include <stdarg.h>
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

/* Logging */
static vgpu_log_callback s_log_function = NULL;
static void* s_log_user_data = NULL;

void vgpu_set_log_callback(vgpu_log_callback callback, void* user_data) {
    s_log_function = callback;
    s_log_user_data = user_data;
}

void vgpu_log(vgpu_log_level level, const char* format, ...) {
    if (s_log_function) {
        va_list args;
        va_start(args, format);
        char message[VGPU_MAX_LOG_MESSAGE_LENGTH];
        vsnprintf(message, VGPU_MAX_LOG_MESSAGE_LENGTH, format, args);
        s_log_function(s_log_user_data, level, message);
        va_end(args);
    }
}

static const vgpu_driver* drivers[] = {
#if VGPU_DRIVER_D3D12
    & d3d12_driver,
#endif
#if VGPU_DRIVER_D3D11
    & D3D11_Driver,
#endif
#if VGPU_DRIVER_METAL
    & metal_driver,
#endif
#if VGPU_DRIVER_VULKAN
    & vulkan_driver,
#endif
#if VGPU_DRIVER_OPENGL
    & GL_Driver,
#endif
    NULL
};

static vgpu_config _vgpu_config_defaults(const vgpu_config* config) {
    vgpu_config def = *config;
    def.adapterPreference = VGPU_DEF(def.adapterPreference, VGPUAdapterType_DiscreteGPU);
    def.swapchain_info.color_format = VGPU_DEF(def.swapchain_info.color_format, VGPUTextureFormat_BGRA8Unorm);
    def.swapchain_info.depth_stencil_format = VGPU_DEF(def.swapchain_info.depth_stencil_format, VGPUTextureFormat_Undefined);
    def.swapchain_info.sample_count = VGPU_DEF(def.swapchain_info.sample_count, 1u);
    return def;
};

static vgpu_backend_type s_backend = VGPU_BACKEND_TYPE_COUNT;
static vgpu_renderer* renderer = NULL;

bool vgpu_set_preferred_backend(vgpu_backend_type backend)
{
    if (renderer != NULL)
        return false;

    s_backend = backend;
    return true;
}

bool vgpu_init(const char* app_name, const vgpu_config* config)
{
    VGPU_ASSERT(config);

    if (renderer) {
        return true;
    }

    if (s_backend == VGPU_BACKEND_TYPE_COUNT)
    {
        for (uint32_t i = 0; i < VGPU_COUNT_OF(drivers); i++)
        {
            if (!drivers[i])
                break;

            if (drivers[i]->is_supported()) {
                renderer = drivers[i]->create_renderer();
                break;
            }
        }
    }
    else
    {
        for (uint32_t i = 0; i < VGPU_COUNT_OF(drivers); i++)
        {
            if (!drivers[i])
                break;

            if (drivers[i]->backend == s_backend && drivers[i]->is_supported())
            {
                renderer = drivers[i]->create_renderer();
                break;
            }
        }
    }

    vgpu_config config_def = _vgpu_config_defaults(config);
    if (!renderer || !renderer->init(app_name, &config_def)) {
        return false;
    }

    return true;
}

void vgpu_shutdown(void) {
    if (renderer == NULL)
        return;

    renderer->shutdown();
    renderer = NULL;
}

void vgpu_query_caps(vgpu_caps* caps)
{
    VGPU_ASSERT(caps);
    VGPU_ASSERT(renderer);

    renderer->query_caps(caps);
}

bool vgpu_begin_frame(void)
{
    return renderer->frame_begin();
}

void vgpu_end_frame(void)
{
    renderer->frame_finish();
}

/* Buffer */
vgpu_buffer vgpu_create_buffer(const vgpu_buffer_info* info) {
    VGPU_ASSERT(info);
    VGPU_ASSERT(info->size > 0);

    if (info->usage == VGPU_BUFFER_USAGE_IMMUTABLE && !info->data)
    {
        vgpu_log(VGPU_LOG_LEVEL_ERROR, "Cannot create immutable buffer without data");
        return NULL;
    }

    return renderer->buffer_create(info);
}

void vgpu_destroy_buffer(vgpu_buffer buffer) {
    VGPU_ASSERT(buffer);

    renderer->buffer_destroy(buffer);
}

/* Shader */
vgpu_shader vgpu_create_shader(const vgpu_shader_info* info) {
    VGPU_ASSERT(info);
    return renderer->shader_create(info);
}

void vgpu_destroy_shader(vgpu_shader shader) {
    VGPU_ASSERT(shader);
    renderer->shader_destroy(shader);
}

/* Texture */
static VGPUTextureDescriptor _vgpu_texture_info_defaults(const VGPUTextureDescriptor* info) {
    VGPUTextureDescriptor def = *info;
    def.type = VGPU_DEF(def.type, VGPUTextureType_2D);
    def.format = VGPU_DEF(def.format, VGPUTextureFormat_RGBA8Unorm);
    def.size.depth = VGPU_DEF(def.size.depth, 1u);
    def.mipLevelCount = VGPU_DEF(def.mipLevelCount, 1u);
    def.sampleCount = VGPU_DEF(def.sampleCount, 1u);
    return def;
};

VGPUTexture vgpuTextureCreate(const VGPUTextureDescriptor* descriptor) {
    VGPU_ASSERT(descriptor);
    VGPUTextureDescriptor def = _vgpu_texture_info_defaults(descriptor);
    return renderer->texture_create(&def);
}

void vgpuTextureDestroy(VGPUTexture handle) {
    renderer->texture_destroy(handle);
}

bool vgpuTextureInitView(VGPUTexture texture, const VGPUTextureViewDescriptor* descriptor) {
    return renderer->textureInitView(texture, descriptor);
}

uint64_t vgpuTextureGetNativeHandle(VGPUTexture handle) {
    return renderer->texture_get_native_handle(handle);
}

/* Pipeline */
vgpu_pipeline vgpu_create_pipeline(const vgpu_pipeline_info* info) {
    VGPU_ASSERT(info);
    return renderer->pipeline_create(info);
}

void vgpu_destroy_pipeline(vgpu_pipeline pipeline) {
    renderer->pipeline_destroy(pipeline);
}

/* Commands */
void vgpuPushDebugGroup(const char* name, const VGPUColor* color) {
    VGPU_ASSERT(color);
    renderer->push_debug_group(name, color);
}

void vgpuPopDebugGroup(void) {
    renderer->pop_debug_group();
}

void vgpuInsertDebugMarker(const char* name, const VGPUColor* color) {
    VGPU_ASSERT(color);
    renderer->insert_debug_marker(name, color);
}

void vgpuBeginRenderPass(const VGPURenderPassDescriptor* descriptor) {
    VGPU_ASSERT(descriptor);
    VGPU_ASSERT(descriptor->colorAttachmentCount || descriptor->depthStencilAttachment.texture);

    renderer->begin_render_pass(descriptor);
}

void vgpuEndRenderPass(void) {
    renderer->end_render_pass();
}

void vgpu_bind_pipeline(vgpu_pipeline pipeline) {
    VGPU_ASSERT(pipeline);
    renderer->bind_pipeline(pipeline);
}

void vgpu_draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex) {
    renderer->draw(vertex_count, instance_count, first_vertex);
}

/* Utility methods */

/* Pixel Format */
typedef struct vgpu_texture_format_desc
{
    VGPUTextureFormat           format;
    VGPUTextureFormatType       type;
    uint8_t                     bitsPerPixel;
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
} vgpu_texture_format_desc;

const vgpu_texture_format_desc k_format_desc[] =
{
    // format                                   type                                    bpp         compression             bits
    { VGPUTextureFormat_Undefined,              VGPUTextureFormatType_Unknown,          0,          {0, 0, 0, 0, 0},        {0, 0, 0, 0, 0, 0}},
    // 8-bit pixel formats
    { VGPUTextureFormat_R8Unorm,                VGPUTextureFormatType_Unorm,            8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    { VGPUTextureFormat_R8Snorm,                VGPUTextureFormatType_Snorm,            8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    { VGPUTextureFormat_R8Uint,                 VGPUTextureFormatType_Uint,             8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
    { VGPUTextureFormat_R8Sint,                 VGPUTextureFormatType_Uint,             8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},

    // 16-bit pixel formats
    { VGPUTextureFormat_R16Unorm,               VGPUTextureFormatType_Unorm,            16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPUTextureFormat_R16Snorm,               VGPUTextureFormatType_Snorm,            16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPUTextureFormat_R16Uint,                VGPUTextureFormatType_Uint,             16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPUTextureFormat_R16Sint,                VGPUTextureFormatType_Sint,             16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPUTextureFormat_R16Float,               VGPUTextureFormatType_Float,            16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
    { VGPUTextureFormat_RG8Unorm,               VGPUTextureFormatType_Unorm,            16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    { VGPUTextureFormat_RG8Snorm,               VGPUTextureFormatType_Snorm,            16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    { VGPUTextureFormat_RG8Uint,                VGPUTextureFormatType_Uint,             16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
    { VGPUTextureFormat_RG8Sint,                VGPUTextureFormatType_Sint,             16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},

    // Packed 16-bit pixel formats
    //{ VGPU_PIXEL_FORMAT_R5G6B5_UNORM,         VGPUTextureFormatType_Unorm,            16,         {1, 1, 2, 1, 1},        {0, 0, 5, 6, 5, 0}},
    //{ VGPU_PIXEL_FORMAT_RGBA4_UNORM,          VGPUTextureFormatType_Unorm,            16,         {1, 1, 2, 1, 1},        {0, 0, 4, 4, 4, 4}},

    // 32-bit pixel formats
    { VGPUTextureFormat_R32Uint,                VGPUTextureFormatType_Uint,             32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
    { VGPUTextureFormat_R32Sint,                VGPUTextureFormatType_Sint,             32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
    { VGPUTextureFormat_R32Float,               VGPUTextureFormatType_Float,            32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
    { VGPUTextureFormat_RG16Unorm,              VGPUTextureFormatType_Unorm,            32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { VGPUTextureFormat_RG16Snorm,              VGPUTextureFormatType_Snorm,            32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { VGPUTextureFormat_RG16Uint,               VGPUTextureFormatType_Uint,             32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { VGPUTextureFormat_RG16Sint,               VGPUTextureFormatType_Sint,             32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { VGPUTextureFormat_RG16Float,              VGPUTextureFormatType_Float,            32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
    { VGPUTextureFormat_RGBA8Unorm,             VGPUTextureFormatType_Unorm,            32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPUTextureFormat_RGBA8UnormSrgb,         VGPUTextureFormatType_UnormSrgb,        32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPUTextureFormat_RGBA8Snorm,             VGPUTextureFormatType_Snorm,            32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPUTextureFormat_RGBA8Uint,              VGPUTextureFormatType_Uint,             32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPUTextureFormat_RGBA8Sint,              VGPUTextureFormatType_Sint,             32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPUTextureFormat_BGRA8Unorm,             VGPUTextureFormatType_Unorm,            32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
    { VGPUTextureFormat_BGRA8UnormSrgb,         VGPUTextureFormatType_UnormSrgb,        32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},

    // Packed 32-Bit Pixel formats
    { VGPUTextureFormat_RGB10A2Unorm,           VGPUTextureFormatType_Unorm,            32,         {1, 1, 4, 1, 1},        {0, 0, 10, 10, 10, 2}},
    { VGPUTextureFormat_RG11B10Ufloat,          VGPUTextureFormatType_Float,            32,         {1, 1, 4, 1, 1},        {0, 0, 11, 11, 10, 0}},
    { VGPUTextureFormat_RGB9E5Ufloat,           VGPUTextureFormatType_Float,            32,         {1, 1, 4, 1, 1},        {0, 0, 9, 9, 9, 5}},

    // 64-Bit Pixel Formats
    { VGPUTextureFormat_RG32Uint,               VGPUTextureFormatType_Uint,             64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
    { VGPUTextureFormat_RG32Sint,               VGPUTextureFormatType_Sint,             64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
    { VGPUTextureFormat_RG32Float,              VGPUTextureFormatType_Float,            64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
    { VGPUTextureFormat_RGBA16Unorm,            VGPUTextureFormatType_Unorm,            64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { VGPUTextureFormat_RGBA16Snorm,            VGPUTextureFormatType_Snorm,            64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { VGPUTextureFormat_RGBA16Uint,             VGPUTextureFormatType_Uint,             64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { VGPUTextureFormat_RGBA16Sint,             VGPUTextureFormatType_Sint,             64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
    { VGPUTextureFormat_RGBA16Float,            VGPUTextureFormatType_Float,            64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},

    // 128-Bit Pixel Formats
    { VGPUTextureFormat_RGBA32Uint,             VGPUTextureFormatType_Uint,             128,          {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},
    { VGPUTextureFormat_RGBA32Sint,             VGPUTextureFormatType_Sint,             128,          {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},
    { VGPUTextureFormat_RGBA32Float,            VGPUTextureFormatType_Float,            128,          {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},

    // Depth-stencil
    { VGPUTextureFormat_Depth16Unorm,           VGPUTextureFormatType_Unorm,            16,          {1, 1, 2, 1, 1},        {16, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_Depth32Float,           VGPUTextureFormatType_Float,            32,          {1, 1, 4, 1, 1},        {32, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_Stencil8,               VGPUTextureFormatType_Unorm,            8,           {1, 1, 1, 1, 1},        {0, 8, 0, 0, 0, 0}},
    { VGPUTextureFormat_Depth24UnormStencil8,   VGPUTextureFormatType_Unorm,            32,          {1, 1, 4, 1, 1},        {24, 8, 0, 0, 0, 0}},
    { VGPUTextureFormat_Depth32FloatStencil8,   VGPUTextureFormatType_Float,            40,          {1, 1, 4, 1, 1},        {32, 8, 0, 0, 0, 0}},

    // Compressed formats
    { VGPUTextureFormat_BC1RGBAUnorm,           VGPUTextureFormatType_Unorm,            4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC1RGBAUnormSrgb,       VGPUTextureFormatType_UnormSrgb,        4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC2RGBAUnorm,           VGPUTextureFormatType_Unorm,            8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC2RGBAUnormSrgb,       VGPUTextureFormatType_UnormSrgb,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC3RGBAUnorm,           VGPUTextureFormatType_Unorm,            8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC3RGBAUnorm,           VGPUTextureFormatType_UnormSrgb,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC4RUnorm,              VGPUTextureFormatType_Unorm,            4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC4RSnorm,              VGPUTextureFormatType_Snorm,            4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC5RGUnorm,             VGPUTextureFormatType_Unorm,            8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC5RGSnorm,             VGPUTextureFormatType_Snorm,            8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC6HRGBUfloat,          VGPUTextureFormatType_Float,            8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC6HRGBFloat,           VGPUTextureFormatType_Float,            8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC7RGBAUnorm,           VGPUTextureFormatType_Unorm,            8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_BC7RGBAUnorm,           VGPUTextureFormatType_UnormSrgb,        8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},

    // Compressed PVRTC Pixel Formats
    { VGPUTextureFormat_PVRTC_RGB2,             VGPUTextureFormatType_Unorm,            2,          {8, 4, 8, 2, 2},        {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_PVRTC_RGBA2,            VGPUTextureFormatType_Unorm,            2,          {8, 4, 8, 2, 2},        {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_PVRTC_RGB4,             VGPUTextureFormatType_Unorm,            4,          {4, 4, 8, 2, 2},        {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_PVRTC_RGBA4,            VGPUTextureFormatType_Unorm,            4,          {4, 4, 8, 2, 2},        {0, 0, 0, 0, 0, 0}},
    // Compressed ETC Pixel Formats
    { VGPUTextureFormat_ETC2_RGB8,              VGPUTextureFormatType_Unorm,            4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_ETC2_RGB8Srgb,          VGPUTextureFormatType_UnormSrgb,        4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_ETC2_RGB8A1,            VGPUTextureFormatType_Unorm,            4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_ETC2_RGB8A1Srgb,        VGPUTextureFormatType_UnormSrgb,        4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
    // Compressed ASTC Pixel Formats
    { VGPUTextureFormat_ASTC_4x4,               VGPUTextureFormatType_Unorm,            8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_ASTC_5x4,               VGPUTextureFormatType_Unorm,            8,          {5, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_ASTC_5x5,               VGPUTextureFormatType_Unorm,            6,          {5, 5, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_ASTC_6x5,               VGPUTextureFormatType_Unorm,            4,          {6, 5, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_ASTC_6x6,               VGPUTextureFormatType_Unorm,            4,          {6, 6, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    { VGPUTextureFormat_ASTC_8x5,               VGPUTextureFormatType_Unorm,            4,          {8, 5, 16, 1, 1},       {0, 0, 0, 0, 0, 0} },
    { VGPUTextureFormat_ASTC_8x6,               VGPUTextureFormatType_Unorm,            3,          {8, 6, 16, 1, 1},       {0, 0, 0, 0, 0, 0} },
    { VGPUTextureFormat_ASTC_8x8,               VGPUTextureFormatType_Unorm,            3,          {8, 8, 16, 1, 1},       {0, 0, 0, 0, 0, 0} },
    { VGPUTextureFormat_ASTC_10x5,              VGPUTextureFormatType_Unorm,            3,          {10, 5, 16, 1, 1},     {0, 0, 0, 0, 0, 0} },
    { VGPUTextureFormat_ASTC_10x6,              VGPUTextureFormatType_Unorm,            3,          {10, 6, 16, 1, 1},     {0, 0, 0, 0, 0, 0} },
    { VGPUTextureFormat_ASTC_10x8,              VGPUTextureFormatType_Unorm,            3,          {10, 8, 16, 1, 1},     {0, 0, 0, 0, 0, 0} },
    { VGPUTextureFormat_ASTC_10x10,             VGPUTextureFormatType_Unorm,            3,          {10, 10, 16, 1, 1},     {0, 0, 0, 0, 0, 0} },
    { VGPUTextureFormat_ASTC_12x10,             VGPUTextureFormatType_Unorm,            3,          {12, 10, 16, 1, 1},     {0, 0, 0, 0, 0, 0} },
    { VGPUTextureFormat_ASTC_12x12,             VGPUTextureFormatType_Unorm,            3,          {12, 12, 16, 1, 1},     {0, 0, 0, 0, 0, 0} },
};

bool vgpuIsDepthFormat(VGPUTextureFormat format) {
    VGPU_ASSERT(k_format_desc[format].format == format);
    return k_format_desc[format].bits.depth > 0;
}

bool vgpuIsStencilFormat(VGPUTextureFormat format) {
    VGPU_ASSERT(k_format_desc[format].format == format);
    return k_format_desc[format].bits.stencil > 0;
}

bool vgpuIsDepthStencilFormat(VGPUTextureFormat format) {
    return vgpuIsDepthFormat(format) || vgpuIsStencilFormat(format);
}

bool vgpuIsCompressedFormat(VGPUTextureFormat format) {
    VGPU_ASSERT(k_format_desc[format].format == format);
    return format >= VGPUTextureFormat_BC1RGBAUnorm && format <= VGPUTextureFormat_ASTC_12x12;
}

bool vgpuIsBcCompressedFormat(VGPUTextureFormat format) {
    VGPU_ASSERT(k_format_desc[format].format == format);
    return format >= VGPUTextureFormat_BC1RGBAUnorm && format <= VGPUTextureFormat_BC7RGBAUnormSrgb;
}

uint32_t vgpu_calculate_mip_levels(uint32_t width, uint32_t height, uint32_t depth)
{
    uint32_t mipLevels = 0u;
    uint32_t size = VGPU_MAX(VGPU_MAX(width, height), depth);
    while (1u << mipLevels <= size) {
        ++mipLevels;
    }

    if (1u << mipLevels < size) {
        ++mipLevels;
    }

    return mipLevels;
}
