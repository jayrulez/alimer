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

static const vgpu_driver* drivers[] = {
#if defined(VGPU_DRIVER_D3D11)
    &d3d11_driver,
#endif
#if defined(VGPU_DRIVER_D3D12) && TODO
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

static void vgpu_log_default_callback(void* user_data, VGPULogLevel level, const char* message) {

}

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

static vgpu_PFN_log vgpu_log_callback = vgpu_log_default_callback;
static void* vgpu_log_user_data = NULL;
const vgpu_allocation_callbacks* vgpu_alloc_cb = &vgpu_default_alloc_cb;
void* vgpu_allocation_user_data = NULL;

void vgpu_log_set_log_callback(vgpu_PFN_log callback, void* user_data) {
    vgpu_log_callback = callback;
    vgpu_log_user_data = user_data;
}

void vgpu_set_allocation_callbacks(const vgpu_allocation_callbacks* callbacks) {
    if (callbacks == NULL) {
        vgpu_alloc_cb = &vgpu_default_alloc_cb;
    }
    else {
        vgpu_alloc_cb = callbacks;
    }
}

static VGPUGraphicsContext* s_graphicsContext = nullptr;

/* Context */
static VGPUDeviceDescription VGPUDeviceDescriptionDef(const VGPUDeviceDescription* desc) {
    VGPUDeviceDescription def = *desc;
    def.swapchain.width = _vgpu_def(desc->swapchain.width, 1u);
    def.swapchain.height = _vgpu_def(desc->swapchain.height, 1u);
    def.swapchain.colorFormat = _vgpu_def(desc->swapchain.colorFormat, VGPUTextureFormat_BGRA8UNorm);
    def.swapchain.depthStencilFormat = _vgpu_def(desc->swapchain.depthStencilFormat, VGPUTextureFormat_Undefined);
    def.swapchain.sampleCount = _vgpu_def(desc->swapchain.sampleCount, VGPUTextureSampleCount1);
    return def;
}

bool vgpuInit(VGPUBackendType backendType, const VGPUDeviceDescription* desc)
{
    VGPU_ASSERT(desc);

    if (s_graphicsContext != nullptr) {
        return true;
    }
    
    if (backendType == VGPUBackendType_Count) {
        for (uint32_t i = 0; _vgpu_count_of(drivers); i++) {
            if (drivers[i]->isSupported()) {
                s_graphicsContext = drivers[i]->createContext();
                break;
            }
        }
    }
    else {
        for (uint32_t i = 0; _vgpu_count_of(drivers); i++) {
            if (drivers[i]->backendType == backendType && drivers[i]->isSupported()) {
                s_graphicsContext = drivers[i]->createContext();
                break;
            }
        }
    }

    if (s_graphicsContext == nullptr) {
        return false;
    }

    VGPUDeviceDescription descDef = VGPUDeviceDescriptionDef(desc);
    if (!s_graphicsContext->init(&descDef)) {
        s_graphicsContext = nullptr;
        return false;
    }

    return true;
}

void vgpuShutdown(void) {
    if (s_graphicsContext == nullptr) {
        return;
    }

    s_graphicsContext->shutdown();
    s_graphicsContext = nullptr;
}

void vgpuBeginFrame(void) {
    s_graphicsContext->beginFrame();
}

void vgpuEndFrame(void) {
    s_graphicsContext->endFrame();
}

void vgpuBeginRenderPass(void) {

}

void vgpuEndRenderPass(void) {

}

/* Texture */
static VGPUTextureDescription _VGPUTextureDescriptionDefaults(const VGPUTextureDescription* desc) {
    VGPUTextureDescription def = *desc;
    def.type = _vgpu_def(desc->type, VGPUTextureType_2D);
    def.format = _vgpu_def(desc->format, VGPUTextureFormat_RGBA8UNorm);
    def.width = _vgpu_def(desc->width, 1u);
    def.height = _vgpu_def(desc->height, 1u);
    def.depth = _vgpu_def(desc->depth, 1u);
    def.mipLevels = _vgpu_def(desc->mipLevels, 1u);
    def.arrayLayers = _vgpu_def(desc->arrayLayers, 1u);
    def.sampleCount = _vgpu_def(desc->sampleCount, VGPUTextureSampleCount1);
    return def;
}

VGPUTexture vgpuCreateTexture(const VGPUTextureDescription* desc) {
    VGPU_ASSERT(s_graphicsContext);
    VGPU_ASSERT(desc);

    VGPUTextureDescription desc_def = _VGPUTextureDescriptionDefaults(desc);

    /* Alloc and init texture. */
    VGPUTexture texture = s_graphicsContext->allocTexture();
    if (!s_graphicsContext->initTexture(texture, &desc_def)) {
        vgpuDestroyTexture(texture);
        return { VGPU_INVALID_ID };
    }

    return texture;
}

void vgpuDestroyTexture(VGPUTexture texture) {
    VGPU_ASSERT(s_graphicsContext);
    if (texture.id != VGPU_INVALID_ID) {
        s_graphicsContext->destroyTexture(texture);
    }
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
