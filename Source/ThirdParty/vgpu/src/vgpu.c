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
#if defined(VGPU_DRIVER_D3D11) && defined(TODO_D3D11)
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

static vgpu_swapchain_info vgpu_swapchain_info_def(const vgpu_swapchain_info* info) {
    vgpu_swapchain_info def = *info;
    def.width = _vgpu_def(info->width, 1u);
    def.height = _vgpu_def(info->height, 1u);
    def.colorFormat = _vgpu_def(info->colorFormat, VGPUTextureFormat_BGRA8UNorm);
    def.depthStencilFormat = _vgpu_def(info->depthStencilFormat, VGPUTextureFormat_Undefined);
    //def.sample_count = _vgpu_def(info->sample_count, 1u);
    return def;
}

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

/* Texture */
static vgpu_texture_info vgpu_texture_info_def(const vgpu_texture_info* info) {
    vgpu_texture_info def = *info;
    def.type = _vgpu_def(info->type, VGPU_TEXTURE_TYPE_2D);
    def.format = _vgpu_def(info->format, VGPUTextureFormat_RGBA8UNorm);
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
    if (texture.id != VGPU_INVALID_ID) {
        s_gpu_context->texture_destroy(texture);
    }
}

uint32_t vgpu_texture_get_width(vgpu_texture texture, uint32_t mipLevel) {
    return s_gpu_context->texture_get_width(texture, mipLevel);
}

uint32_t vgpu_texture_get_height(vgpu_texture texture, uint32_t mipLevel) {
    return s_gpu_context->texture_get_height(texture, mipLevel);
}

/* Buffer */
vgpu_buffer vgpu_buffer_create(const vgpu_buffer_info* info) {
    VGPU_ASSERT(s_gpu_context);
    VGPU_ASSERT(info);

    return s_gpu_context->buffer_create(info);
}

void vgpu_buffer_destroy(vgpu_buffer handle) {
    VGPU_ASSERT(s_gpu_context);
    if (handle.id != VGPU_INVALID_ID) {
        s_gpu_context->buffer_destroy(handle);
    }
}

/* Framebuffer */
static VGPUFramebufferDescription _VGPUFramebufferDescriptionDefaults(const VGPUFramebufferDescription* desc) {
    VGPUFramebufferDescription def = *desc;
    uint32_t width = desc->width;
    uint32_t height = desc->height;

    if (width == 0 || height == 0) {
        width = UINT32_MAX;
        height = UINT32_MAX;

        for (uint32_t i = 0; i < VGPU_MAX_COLOR_ATTACHMENTS; i++)
        {
            if (desc->colorAttachments[i].texture.id == VGPU_INVALID_ID)
                continue;

            uint32_t mipLevel = desc->colorAttachments[i].mipLevel;
            width = _vgpu_min(width, vgpu_texture_get_width(desc->colorAttachments[i].texture, mipLevel));
            height = _vgpu_min(height, vgpu_texture_get_height(desc->colorAttachments[i].texture, mipLevel));
        }

        /*if (info.depth_stencil)
        {
            unsigned lod = info.depth_stencil->get_create_info().base_level;
            width = min(width, info.depth_stencil->get_image().get_width(lod));
            height = min(height, info.depth_stencil->get_image().get_height(lod));
        }*/
    }

    def.width = width;
    def.height = height;
    def.layers = _vgpu_def(desc->layers, 1u);
    return def;
}

vgpu_framebuffer vgpu_framebuffer_create(const VGPUFramebufferDescription* desc) {
    VGPU_ASSERT(s_gpu_context);
    VGPU_ASSERT(desc);

    VGPUFramebufferDescription desc_def = _VGPUFramebufferDescriptionDefaults(desc);
    return s_gpu_context->framebuffer_create(&desc_def);
}

vgpu_framebuffer vgpu_framebuffer_create_from_window(const vgpu_swapchain_info* info) {
    VGPU_ASSERT(s_gpu_context);
    VGPU_ASSERT(info);

    vgpu_swapchain_info info_def = vgpu_swapchain_info_def(info);
    return s_gpu_context->framebuffer_create_from_window(&info_def);
}

void vgpu_framebuffer_destroy(vgpu_framebuffer framebuffer) {
    VGPU_ASSERT(s_gpu_context);
    if (framebuffer.id != VGPU_INVALID_ID) {
        s_gpu_context->framebuffer_destroy(framebuffer);
    }
}

vgpu_framebuffer vgpu_framebuffer_get_default(void) {
    return s_gpu_context->getDefaultFramebuffer();
}

/* CommandBuffer */
VGPUCommandBuffer vgpuBeginCommandBuffer(const char* name, bool profile) {
    VGPU_ASSERT(name);
    return s_gpu_context->beginCommandBuffer(name, profile);
}

void vgpuInsertDebugMarker(VGPUCommandBuffer commandBuffer, const char* name) {
    VGPU_ASSERT(name);
    s_gpu_context->insertDebugMarker(commandBuffer, name);
}

void vgpuPushDebugGroup(VGPUCommandBuffer commandBuffer, const char* name) {
    VGPU_ASSERT(name);
    s_gpu_context->pushDebugGroup(commandBuffer, name);
}

void vgpuPopDebugGroup(VGPUCommandBuffer commandBuffer) {
    s_gpu_context->popDebugGroup(commandBuffer);
}

void vgpuBeginRenderPass(VGPUCommandBuffer commandBuffer, const VGPURenderPassBeginDescription* beginDesc) {
    s_gpu_context->beginRenderPass(commandBuffer, beginDesc);
}

void vgpuEndRenderPass(VGPUCommandBuffer commandBuffer) {
    s_gpu_context->endRenderPass(commandBuffer);
}

/* PixelFormat */
typedef struct VGPUPixelFormatDescription {
    VGPUPixelFormat format;
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

#define VGPU_DEFINE_DEPTH_FORMAT(format, byteSize, type) \
     { VGPUTextureFormat_##format##, #format, true, false, true, false, VGPUPixelFormatAspect_Depth, type, byteSize, 1, 1 }

#define VGPU_DEFINE_DEPTH_STENCIL_FORMAT(format, aspect, byteSize) \
     { VGPUTextureFormat_##format##, #format, true, false, true, false, aspect, VGPUPixelFormatType_Unknown, byteSize, 1, 1 }

#define VGPU_DEFINE_COMPRESSED_FORMAT(format, type, byteSize, width, height) \
     { VGPUTextureFormat_##format##, #format, false, true, true, false, VGPUPixelFormatAspect_Color, type, byteSize, width, height }

const VGPUPixelFormatDescription FormatDesc[] =
{
    { VGPUTextureFormat_Undefined,  "Undefined", false, false, false, false, VGPUPixelFormatAspect_Color, VGPUPixelFormatType_Unknown, 0, 0, 0},
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
    VGPU_DEFINE_COLOR_FORMAT(RGBA8UNorm, true, true, 4, VGPUPixelFormatType_UNorm),
    VGPU_DEFINE_COLOR_FORMAT(RGBA8UNormSrgb, true, false, 4, VGPUPixelFormatType_UNormSrgb),
    VGPU_DEFINE_COLOR_FORMAT(RGBA8SNorm, false, true, 4, VGPUPixelFormatType_SNorm),
    VGPU_DEFINE_COLOR_FORMAT(RGBA8UInt, true, true, 4, VGPUPixelFormatType_UInt),
    VGPU_DEFINE_COLOR_FORMAT(RGBA8SInt, true, true, 4, VGPUPixelFormatType_SInt),
    VGPU_DEFINE_COLOR_FORMAT(BGRA8UNorm, true, false, 4, VGPUPixelFormatType_UNorm),
    VGPU_DEFINE_COLOR_FORMAT(BGRA8UNormSrgb, true, false, 4, VGPUPixelFormatType_UNormSrgb),
    VGPU_DEFINE_COLOR_FORMAT(RGB10A2UNorm, true, false, 4, VGPUPixelFormatType_UNorm),
    VGPU_DEFINE_COLOR_FORMAT(RG11B10Float, false, false, 4, VGPUPixelFormatType_Float),

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
    VGPU_DEFINE_DEPTH_FORMAT(Depth32Float, 4, VGPUPixelFormatType_Float),
    // Packed depth/depth-stencil formats
    VGPU_DEFINE_DEPTH_FORMAT(Depth24Plus, 4, VGPUPixelFormatType_Float),
    VGPU_DEFINE_DEPTH_FORMAT(Depth24PlusStencil8, 4, VGPUPixelFormatType_Float),

    VGPU_DEFINE_COMPRESSED_FORMAT(BC1RGBAUNorm, VGPUPixelFormatType_UNorm, 8, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC1RGBAUNormSrgb, VGPUPixelFormatType_UNormSrgb, 8, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC2RGBAUNorm, VGPUPixelFormatType_UNorm, 16, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC2RGBAUNormSrgb, VGPUPixelFormatType_UNormSrgb, 16, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC3RGBAUNorm, VGPUPixelFormatType_UNorm, 16, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC3RGBAUNormSrgb, VGPUPixelFormatType_UNormSrgb, 16, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC4RUNorm, VGPUPixelFormatType_UNorm, 8, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC4RSNorm, VGPUPixelFormatType_SNorm, 8, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC5RGUNorm, VGPUPixelFormatType_UNorm, 16, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC5RGSNorm, VGPUPixelFormatType_SNorm, 16, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC6HRGBSFloat, VGPUPixelFormatType_Float, 16, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC6HRGBUFloat, VGPUPixelFormatType_Float, 16, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC7RGBAUNorm, VGPUPixelFormatType_UNorm, 16, 4, 4),
    VGPU_DEFINE_COMPRESSED_FORMAT(BC7RGBAUNormSrgb, VGPUPixelFormatType_UNormSrgb, 16, 4, 4),
};

#undef VGPU_DEFINE_COLOR_FORMAT
#undef VGPU_DEFINE_DEPTH_FORMAT
#undef VGPU_DEFINE_DEPTH_STENCIL_FORMAT
#undef VGPU_DEFINE_COMPRESSED_FORMAT

bool vgpuIsColorFormat(VGPUPixelFormat format) {
    VGPU_ASSERT(FormatDesc[format].format == format);
    return FormatDesc[format].aspect == VGPUPixelFormatAspect_Color;
}

bool vgpuIsDepthFormat(VGPUPixelFormat format) {
    VGPU_ASSERT(FormatDesc[format].format == format);
    return FormatDesc[format].aspect == VGPUPixelFormatAspect_Depth || FormatDesc[format].aspect == VGPUPixelFormatAspect_DepthStencil;
}

bool vgpuIsStencilFormat(VGPUPixelFormat format) {
    VGPU_ASSERT(FormatDesc[format].format == format);
    return FormatDesc[format].aspect == VGPUPixelFormatAspect_Stencil || FormatDesc[format].aspect == VGPUPixelFormatAspect_DepthStencil;
}

bool vgpuIsDepthOrStencilFormat(VGPUPixelFormat format) {
    VGPU_ASSERT(FormatDesc[format].format == format);
    return FormatDesc[format].aspect != VGPUPixelFormatAspect_Color;
}

bool vgpuIsCompressedFormat(VGPUPixelFormat format) {
    VGPU_ASSERT(FormatDesc[format].format == format);
    return FormatDesc[format].compressed;
}
