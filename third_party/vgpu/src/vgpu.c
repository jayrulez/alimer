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

#include "vgpu_driver.h"
#include <stdlib.h>

/* Allocation */
void* vgpu_default_allocate_memory(void* user_data, size_t size) {
    VGPU_UNUSED(user_data);
    return malloc(size);
}

void* vgpu_default_allocate_cleard_memory(void* user_data, size_t size) {
    VGPU_UNUSED(user_data);
    void* mem = malloc(size);
    memset(mem, 0, size);
    return mem;
}

void vgpu_default_free_memory(void* user_data, void* ptr) {
    VGPU_UNUSED(user_data);
    free(ptr);
}

const vgpu_allocation_callbacks vgpu_default_alloc_cb = {
    NULL,
    vgpu_default_allocate_memory,
    vgpu_default_allocate_cleard_memory,
    vgpu_default_free_memory
};

const vgpu_allocation_callbacks* vgpu_alloc_cb = &vgpu_default_alloc_cb;
void* vgpu_allocation_user_data = NULL;

void vgpu_set_allocation_callbacks(const vgpu_allocation_callbacks* callbacks) {
    if (callbacks == NULL) {
        vgpu_alloc_cb = &vgpu_default_alloc_cb;
    }
    else {
        vgpu_alloc_cb = callbacks;
    }
}

/* Log */
static void vgpu_log_default_callback(void* user_data, vgpu_log_level level, const char* message) {

}

static vgpu_PFN_log vgpu_log_callback = vgpu_log_default_callback;
static void* vgpu_log_user_data = NULL;

void vgpu_log_set_log_callback(vgpu_PFN_log callback, void* user_data) {
    vgpu_log_callback = callback;
    vgpu_log_user_data = user_data;
}

static const vgpu_driver* drivers[] = {
#if defined(VGPU_DRIVER_D3D11) 
    &d3d11_driver,
#endif
#if defined(VGPU_DRIVER_D3D12) && defined(TODO_D3D12)
    &d3d12_driver,
#endif
#if defined(VGPU_DRIVER_VULKAN)
    &vulkan_driver,
#endif
#if defined(VGPU_DRIVER_OPENGL)
    &gl_driver,
#endif
    NULL
};

static vgpu_context* s_gpu_context = NULL;

bool vgpu_init(const vgpu_config* config)
{
    VGPU_ASSERT(config);

    if (s_gpu_context) {
        return true;
    }

    if (config->backend_type == VGPU_BACKEND_TYPE_DEFAULT || config->backend_type == VGPU_BACKEND_TYPE_COUNT) {
        for (uint32_t i = 0; _vgpu_count_of(drivers); i++) {
            if (drivers[i]->is_supported()) {
                s_gpu_context = drivers[i]->create_context();
                break;
            }
        }
    }
    else {
        for (uint32_t i = 0; _vgpu_count_of(drivers); i++) {
            if (drivers[i]->backendType == config->backend_type && drivers[i]->is_supported()) {
                s_gpu_context = drivers[i]->create_context();
                break;
            }
        }
    }

    if (s_gpu_context == NULL) {
        return false;
    }

    if (!s_gpu_context->init(config)) {
        s_gpu_context = NULL;
        return false;
    }

    return true;
}

void vgpu_shutdown(void) {
    if (!s_gpu_context) {
        return;
    }

    s_gpu_context->shutdown();
    s_gpu_context = NULL;
}

bool vgpu_frame_begin(void) {
    return s_gpu_context->frame_begin();
}

void vgpu_frame_finish(void) {
    s_gpu_context->frame_end();
}

void vgpuInsertDebugMarker(const char* name) {
    VGPU_ASSERT(name);
    s_gpu_context->insertDebugMarker(name);
}

void vgpuPushDebugGroup(const char* name) {
    VGPU_ASSERT(name);
    s_gpu_context->pushDebugGroup(name);
}

void vgpuPopDebugGroup(void) {
    s_gpu_context->popDebugGroup();
}

void vgpu_render_begin(vgpu_framebuffer framebuffer) {
    s_gpu_context->beginRenderPass(framebuffer);
}

void vgpu_render_finish(void) {
    s_gpu_context->render_finish();
}

/* Texture */
static vgpu_texture_info vgpu_texture_info_def(const vgpu_texture_info* info) {
    vgpu_texture_info def = *info;
    def.type = _vgpu_def(info->type, VGPU_TEXTURE_TYPE_2D);
    def.format = _vgpu_def(info->format, VGPU_TEXTURE_FORMAT_RGBA8);
    def.width = _vgpu_def(info->width, 1u);
    def.height = _vgpu_def(info->height, 1u);
    def.depth = _vgpu_def(info->depth, 1u);
    def.mip_levels = _vgpu_def(info->mip_levels, 1u);
    def.sample_count = _vgpu_def(info->sample_count, 1u);
    return def;
}

vgpu_texture vgpu_texture_create(const vgpu_texture_info* info) {
    VGPU_ASSERT(s_gpu_context);
    VGPU_ASSERT(info);

    vgpu_texture_info info_def = vgpu_texture_info_def(info);
    return s_gpu_context->texture_create(&info_def);
}

void vgpu_texture_destroy(vgpu_texture texture) {
    VGPU_ASSERT(s_gpu_context);
    VGPU_ASSERT(texture);

    s_gpu_context->texture_destroy(texture);
}

uint32_t vgpu_texture_get_width(vgpu_texture texture, uint32_t mip_level) {
    return s_gpu_context->texture_get_width(texture, mip_level);
}

uint32_t vgpu_texture_get_height(vgpu_texture texture, uint32_t mip_level) {
    return s_gpu_context->texture_get_height(texture, mip_level);
}

/* Framebuffer */
static vgpu_framebuffer_info vgpu_framebuffer_info_def(const vgpu_framebuffer_info* info) {
    vgpu_framebuffer_info def = *info;
    uint32_t width = info->width;
    uint32_t height = info->height;

    if (width == 0 || height == 0) {
        width = UINT32_MAX;
        height = UINT32_MAX;

        for (uint32_t i = 0; i < VGPU_MAX_COLOR_ATTACHMENTS; i++)
        {
            if (!info->color_attachments[i].texture)
                continue;

            uint32_t mip_level = info->color_attachments[i].level;
            width = _vgpu_min(width, vgpu_texture_get_width(info->color_attachments[i].texture, mip_level));
            height = _vgpu_min(height, vgpu_texture_get_height(info->color_attachments[i].texture, mip_level));
        }

        if (info->depth_stencil_attachment.texture)
        {
            uint32_t mip_level = info->depth_stencil_attachment.level;
            width = _vgpu_min(width, vgpu_texture_get_width(info->depth_stencil_attachment.texture, mip_level));
            height = _vgpu_min(height, vgpu_texture_get_height(info->depth_stencil_attachment.texture, mip_level));
        }
    }

    def.width = width;
    def.height = height;
    def.layers = _vgpu_def(info->layers, 1u);
    return def;
}

vgpu_framebuffer vgpu_framebuffer_create(const vgpu_framebuffer_info* info) {
    VGPU_ASSERT(s_gpu_context);
    VGPU_ASSERT(info);

    vgpu_framebuffer_info info_def = vgpu_framebuffer_info_def(info);
    return s_gpu_context->framebuffer_create(&info_def);
}

void vgpu_framebuffer_destroy(vgpu_framebuffer framebuffer) {
    VGPU_ASSERT(s_gpu_context);
    VGPU_ASSERT(framebuffer);

    s_gpu_context->framebuffer_destroy(framebuffer);
}

vgpu_framebuffer vgpu_framebuffer_get_default(void) {
    return s_gpu_context->getDefaultFramebuffer();
}

/* Buffer */
vgpu_buffer vgpu_buffer_create(const vgpu_buffer_info* info) {
    VGPU_ASSERT(s_gpu_context);
    VGPU_ASSERT(info);

    return s_gpu_context->buffer_create(info);
}

void vgpu_buffer_destroy(vgpu_buffer buffer) {
    VGPU_ASSERT(s_gpu_context);
    VGPU_ASSERT(buffer);

    s_gpu_context->buffer_destroy(buffer);
}

vgpu_swapchain vgpu_swapchain_create(uintptr_t window_handle, vgpu_texture_format color_format, vgpu_texture_format depth_stencil_format) {
    VGPU_ASSERT(s_gpu_context);
    VGPU_ASSERT(window_handle);

    return s_gpu_context->swapchain_create(window_handle, color_format, depth_stencil_format);
}

void vgpu_swapchain_destroy(vgpu_swapchain swapchain) {
    VGPU_ASSERT(s_gpu_context);
    VGPU_ASSERT(swapchain);

    s_gpu_context->swapchain_destroy(swapchain);
}

void vgpu_swapchain_resize(vgpu_swapchain swapchain, uint32_t width, uint32_t height) {
    VGPU_ASSERT(s_gpu_context);
    VGPU_ASSERT(swapchain);

    s_gpu_context->swapchain_resize(swapchain, width, height);
}

void vgpu_swapchain_present(vgpu_swapchain swapchain) {
    s_gpu_context->swapchain_present(swapchain);
}


/* PixelFormat */
typedef struct VGPUPixelFormatDescription {
    vgpu_texture_format format;
    const char* name;
    bool renderable;
    bool compressed;
    // A format can be known but not supported because it is part of a disabled extension.
    bool supported;
    bool supportsStorageUsage;
    VGPUPixelFormatAspect aspect;

    VGPUPixelFormatType type;
    uint32_t blockByteSize;
    uint32_t blockWidth;
    uint32_t blockHeight;
} VGPUPixelFormatDescription;

#define VGPU_DEFINE_COLOR_FORMAT(format, renderable, supportsStorageUsage, byteSize, type) \
     { VGPUTextureFormat_##format##, #format, renderable, false, true, supportsStorageUsage, VGPUPixelFormatAspect_Color, type, byteSize, 1, 1 }

#define VGPU_DEFINE_COLOR_FORMAT_NEW(format, renderable, supportsStorageUsage, byteSize, type) \
     { VGPU_TEXTURE_FORMAT_##format##, #format, renderable, false, true, supportsStorageUsage, VGPUPixelFormatAspect_Color, type, byteSize, 1, 1 }

#define VGPU_DEFINE_DEPTH_FORMAT(format, byteSize, type) \
     { VGPU_TEXTURE_FORMAT_##format##, #format, true, false, true, false, VGPUPixelFormatAspect_Depth, type, byteSize, 1, 1 }

#define VGPU_DEFINE_DEPTH_STENCIL_FORMAT(format, aspect, byteSize) \
     { VGPUTextureFormat_##format##, #format, true, false, true, false, aspect, VGPUPixelFormatType_Unknown, byteSize, 1, 1 }

#define VGPU_DEFINE_COMPRESSED_FORMAT(format, type, byteSize, width, height) \
     { VGPU_TEXTURE_FORMAT_##format##, #format, false, true, true, false, VGPUPixelFormatAspect_Color, type, byteSize, width, height }

const VGPUPixelFormatDescription FormatDesc[] =
{
    { VGPU_TEXTURE_FORMAT_UNDEFINED,  "Undefined", false, false, false, false, VGPUPixelFormatAspect_Color, VGPUPixelFormatType_Unknown, 0, 0, 0},
    // 1 byte color formats

    VGPU_DEFINE_COLOR_FORMAT(R8UNorm, true, false, 1, VGPUPixelFormatType_UNorm),
    VGPU_DEFINE_COLOR_FORMAT(R8SNorm, false, false, 1, VGPUPixelFormatType_SNorm),
    VGPU_DEFINE_COLOR_FORMAT(R8UInt, true, false, 1, VGPUPixelFormatType_UInt),
    VGPU_DEFINE_COLOR_FORMAT(R8SInt, true, false, 1, VGPUPixelFormatType_SInt),
    // 2 bytes color formats
    VGPU_DEFINE_COLOR_FORMAT(R16UInt, true, false, 2, VGPUPixelFormatType_UInt),
    VGPU_DEFINE_COLOR_FORMAT(R16SInt, true, false, 2, VGPUPixelFormatType_SInt),
    VGPU_DEFINE_COLOR_FORMAT(R16Float, true, false, 2, VGPUPixelFormatType_Float),
    VGPU_DEFINE_COLOR_FORMAT(RG8UNorm, true, false, 2, VGPUPixelFormatType_UNorm),
    VGPU_DEFINE_COLOR_FORMAT(RG8SNorm, false, false, 2, VGPUPixelFormatType_SNorm),
    VGPU_DEFINE_COLOR_FORMAT(RG8UInt, true, false, 2, VGPUPixelFormatType_UInt),
    VGPU_DEFINE_COLOR_FORMAT(RG8SInt, true, false, 2, VGPUPixelFormatType_SInt),

    // 4 bytes color formats
    VGPU_DEFINE_COLOR_FORMAT(R32UInt, true, true, 4, VGPUPixelFormatType_UInt),
    VGPU_DEFINE_COLOR_FORMAT(R32SInt, true, true, 4, VGPUPixelFormatType_SInt),
    VGPU_DEFINE_COLOR_FORMAT(R32Float, true, true, 4, VGPUPixelFormatType_Float),
    VGPU_DEFINE_COLOR_FORMAT(RG16UInt, true, false, 4, VGPUPixelFormatType_UInt),
    VGPU_DEFINE_COLOR_FORMAT(RG16SInt, true, false, 4, VGPUPixelFormatType_SInt),
    VGPU_DEFINE_COLOR_FORMAT(RG16Float, true, false, 4, VGPUPixelFormatType_Float),
    VGPU_DEFINE_COLOR_FORMAT_NEW(RGBA8, true, true, 4, VGPUPixelFormatType_UNorm),
    VGPU_DEFINE_COLOR_FORMAT_NEW(RGBA8_SRGB, true, false, 4, VGPUPixelFormatType_UNormSrgb),
    VGPU_DEFINE_COLOR_FORMAT(RGBA8SNorm, false, true, 4, VGPUPixelFormatType_SNorm),
    VGPU_DEFINE_COLOR_FORMAT(RGBA8UInt, true, true, 4, VGPUPixelFormatType_UInt),
    VGPU_DEFINE_COLOR_FORMAT(RGBA8SInt, true, true, 4, VGPUPixelFormatType_SInt),
    VGPU_DEFINE_COLOR_FORMAT_NEW(BGRA8, true, false, 4, VGPUPixelFormatType_UNorm),
    VGPU_DEFINE_COLOR_FORMAT_NEW(BGRA8_SRGB, true, false, 4, VGPUPixelFormatType_UNormSrgb),
    VGPU_DEFINE_COLOR_FORMAT_NEW(RGB10A2, true, false, 4, VGPUPixelFormatType_UNorm),
    VGPU_DEFINE_COLOR_FORMAT_NEW(RG11B10F, false, false, 4, VGPUPixelFormatType_Float),

    // 8 bytes color formats
    VGPU_DEFINE_COLOR_FORMAT(RG32UInt, true, true, 8, VGPUPixelFormatType_UInt),
    VGPU_DEFINE_COLOR_FORMAT(RG32SInt, true, true, 8, VGPUPixelFormatType_SInt),
    VGPU_DEFINE_COLOR_FORMAT(RG32Float, true, true, 8, VGPUPixelFormatType_Float),
    VGPU_DEFINE_COLOR_FORMAT(RGBA16UInt, true, true, 8, VGPUPixelFormatType_UInt),
    VGPU_DEFINE_COLOR_FORMAT(RGBA16SInt, true, true, 8, VGPUPixelFormatType_SInt),
    VGPU_DEFINE_COLOR_FORMAT(RGBA16Float, true, true, 8, VGPUPixelFormatType_Float),
    // 16 bytes color formats
    VGPU_DEFINE_COLOR_FORMAT(RGBA32UInt, true, true, 16, VGPUPixelFormatType_UInt),
    VGPU_DEFINE_COLOR_FORMAT(RGBA32SInt, true, true, 16, VGPUPixelFormatType_SInt),
    VGPU_DEFINE_COLOR_FORMAT(RGBA32Float, true, true, 16, VGPUPixelFormatType_Float),

    // Depth only formats
    VGPU_DEFINE_DEPTH_FORMAT(D32F, 4, VGPUPixelFormatType_Float),
    // Packed depth/depth-stencil formats
    VGPU_DEFINE_DEPTH_FORMAT(D24_PLUS, 4, VGPUPixelFormatType_Float),
    VGPU_DEFINE_DEPTH_FORMAT(D24S8, 4, VGPUPixelFormatType_Float),

    VGPU_DEFINE_COMPRESSED_FORMAT(BC1, VGPUPixelFormatType_UNorm, 8, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC1_SRGB, VGPUPixelFormatType_UNormSrgb, 8, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC2, VGPUPixelFormatType_UNorm, 16, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC2_SRGB, VGPUPixelFormatType_UNormSrgb, 16, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC3, VGPUPixelFormatType_UNorm, 16, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC3_SRGB, VGPUPixelFormatType_UNormSrgb, 16, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC4, VGPUPixelFormatType_UNorm, 8, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC4S, VGPUPixelFormatType_SNorm, 8, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC5, VGPUPixelFormatType_UNorm, 16, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC5S, VGPUPixelFormatType_SNorm, 16, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC6H_UFLOAT, VGPUPixelFormatType_Float, 16, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC6H_SFLOAT, VGPUPixelFormatType_Float, 16, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC7, VGPUPixelFormatType_UNorm, 16, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC7_SRGB, VGPUPixelFormatType_UNormSrgb, 16, 4, 4),
};

#undef VGPU_DEFINE_COLOR_FORMAT
#undef VGPU_DEFINE_DEPTH_FORMAT
#undef VGPU_DEFINE_DEPTH_STENCIL_FORMAT
#undef VGPU_DEFINE_COMPRESSED_FORMAT

bool vgpu_is_color_format(vgpu_texture_format format) {
    VGPU_ASSERT(FormatDesc[format].format == format);
    return FormatDesc[format].aspect == VGPUPixelFormatAspect_Color;
}

bool vgpu_is_depth_format(vgpu_texture_format format) {
    VGPU_ASSERT(FormatDesc[format].format == format);
    return FormatDesc[format].aspect == VGPUPixelFormatAspect_Depth || FormatDesc[format].aspect == VGPUPixelFormatAspect_DepthStencil;
}

bool vgpu_is_stencil_format(vgpu_texture_format format) {
    VGPU_ASSERT(FormatDesc[format].format == format);
    return FormatDesc[format].aspect == VGPUPixelFormatAspect_Stencil || FormatDesc[format].aspect == VGPUPixelFormatAspect_DepthStencil;
}

bool vgpu_is_depth_stencil_format(vgpu_texture_format format) {
    VGPU_ASSERT(FormatDesc[format].format == format);
    return FormatDesc[format].aspect != VGPUPixelFormatAspect_Color;
}

bool vgpu_is_compressed_format(vgpu_texture_format format) {
    VGPU_ASSERT(FormatDesc[format].format == format);
    return FormatDesc[format].compressed;
}
