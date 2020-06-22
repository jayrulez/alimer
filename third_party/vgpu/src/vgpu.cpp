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
#include <stdio.h>
#include <stdarg.h>

/* Log */
#define VGPU_MAX_LOG_MESSAGE (4096)

static void vgpu_log_default_callback(void* user_data, vgpu_log_level level, const char* message) {

}

static vgpu_PFN_log vgpu_log_callback = vgpu_log_default_callback;
static void* vgpu_log_user_data = NULL;

void vgpu_log_set_log_callback(vgpu_PFN_log callback, void* user_data) {
    vgpu_log_callback = callback;
    vgpu_log_user_data = user_data;
}

void vgpu_log(vgpu_log_level level, const char* format, ...) {
    if (vgpu_log_callback) {
        char msg[VGPU_MAX_LOG_MESSAGE];
        va_list args;
        va_start(args, format);
        vsnprintf(msg, sizeof(msg), format, args);
        vgpu_log_callback(vgpu_log_user_data, level, msg);
        va_end(args);
    }
}

void vgpu_log_error(const char* format, ...) {
    if (vgpu_log_callback) {
        char msg[VGPU_MAX_LOG_MESSAGE];
        va_list args;
        va_start(args, format);
        vsnprintf(msg, sizeof(msg), format, args);
        vgpu_log_callback(vgpu_log_user_data, VGPU_LOG_LEVEL_ERROR, msg);
        va_end(args);
    }
}

static const vgpu_driver* drivers[] = {
#if defined(VGPU_DRIVER_D3D11) 
    &d3d11_driver,
#endif
#if defined(VGPU_DRIVER_D3D12) && defined(TODO_D3D12)
    &d3d12_driver,
#endif
#if defined(VGPU_DRIVER_VULKAN)&& defined(TODO)
    &vulkan_driver,
#endif
#if defined(VGPU_DRIVER_OPENGL)&& defined(TODO)
    &gl_driver,
#endif
    NULL
};

static vgpu_context* s_gpu_context = NULL;

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
            //if (info->color_attachments[i].texture.id == VGPU_INVALID_ID)
            //    continue;

            //uint32_t mip_level = info->color_attachments[i].level;
            //width = _vgpu_min(width, vgpu::textureGetWidth(info->color_attachments[i].texture, mip_level));
            //height = _vgpu_min(height, vgpu_texture_get_height(info->color_attachments[i].texture, mip_level));
        }

        /*if (info->depth_stencil_attachment.texture.id != VGPU_INVALID_ID)
        {
            uint32_t mip_level = info->depth_stencil_attachment.level;
            //width = _vgpu_min(width, vgpu_texture_get_width(info->depth_stencil_attachment.texture, mip_level));
            //height = _vgpu_min(height, vgpu_texture_get_height(info->depth_stencil_attachment.texture, mip_level));
        }*/
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

namespace vgpu
{
    static BackendType s_BackendType = BackendType::Default;
    void setPreferredBackend(BackendType backendType) {
        if (s_gpu_context) {
            return;
        }

        s_BackendType = backendType;
    }

    bool init(const PresentationParameters& presentationParameters, InitFlags flags) {
        if (s_gpu_context) {
            return true;
        }

        if (s_BackendType == BackendType::Default) {
            for (uint32_t i = 0; _vgpu_count_of(drivers); i++) {
                if (drivers[i]->isSupported()) {
                    s_gpu_context = drivers[i]->create_context();
                    break;
                }
            }
        }
        else {
            for (uint32_t i = 0; _vgpu_count_of(drivers); i++) {
                if (drivers[i]->backendType == s_BackendType && drivers[i]->isSupported()) {
                    s_gpu_context = drivers[i]->create_context();
                    break;
                }
            }
        }

        if (s_gpu_context == NULL) {
            return false;
        }

        if (!s_gpu_context->init(presentationParameters, flags)) {
            s_gpu_context = NULL;
            return false;
        }

        return true;
    }

    void shutdown(void) {
        if (!s_gpu_context) {
            return;
        }

        s_gpu_context->shutdown();
        s_gpu_context = NULL;
    }

    const Caps* getCaps() {
        return s_gpu_context->getCaps();
    }

    bool BeginFrame(void) {
        return s_gpu_context->frame_begin();
    }

    void EndFrame(void) {
        s_gpu_context->frame_end();
    }

    /* Texture methods */
    TextureHandle CreateTexture(const TextureDesc& desc) {
        VGPU_ASSERT(s_gpu_context);

        return s_gpu_context->createTexture(desc);
    }

    void DestroyTexture(TextureHandle texture) {
        VGPU_ASSERT(s_gpu_context);
        if (texture.isValid()) {
            s_gpu_context->destroyTexture(texture);
        }
    }

    uint32_t textureGetWidth(TextureHandle texture, uint32_t mipLevel) {
        return s_gpu_context->texture_get_width(texture, mipLevel);
    }

    uint32_t textureGetHeight(TextureHandle texture, uint32_t mipLevel) {
        return s_gpu_context->texture_get_height(texture, mipLevel);
    }

    /* Buffer methods */
    BufferHandle CreateBuffer(const BufferDesc& desc, const void* pInitData) {
        VGPU_ASSERT(s_gpu_context);

        return s_gpu_context->createBuffer(desc, pInitData);
    }

    void DestroyBuffer(BufferHandle buffer) {
        VGPU_ASSERT(s_gpu_context);
        if (buffer.isValid()) {
            s_gpu_context->destroyBuffer(buffer);
        }
    }

    /* CommandBuffer */
    void InsertDebugMarker(const char* name, CommandList commandList) {
        VGPU_ASSERT(name);
        s_gpu_context->insertDebugMarker(name, commandList);
    }

    void PushDebugGroup(const char* name, CommandList commandList) {
        VGPU_ASSERT(name);
        s_gpu_context->pushDebugGroup(name, commandList);
    }

    void PopDebugGroup(CommandList commandList) {
        s_gpu_context->popDebugGroup(commandList);
    }

    void BeginRenderPass(const RenderPassDesc* desc, CommandList commandList) {
        s_gpu_context->beginRenderPass(desc, commandList);
    }

    void EndRenderPass(CommandList commandList) {
        s_gpu_context->endRenderPass(commandList);
    }

    /* Helper methods */
    struct FormatDesc {
        PixelFormat format;
        const char* name;
        uint32_t bytesPerBlock;
        uint32_t channelCount;
        PixelFormatType type;
        struct
        {
            bool isDepth;
            bool isStencil;
            bool isCompressed;
        };
        struct
        {
            uint32_t width;
            uint32_t height;
        } compressionRatio;
        int numChannelBits[4];
    };

    const FormatDesc kFormatDesc[] =
    {
        // Format                           Name,           BytesPerBlock   ChannelCount  Type                          {bDepth,   bStencil, bCompressed},  {CompressionRatio.Width,     CompressionRatio.Height}    {numChannelBits.x, numChannelBits.y, numChannelBits.z, numChannelBits.w}
        {PixelFormat::Undefined,            "Undefined",    0,              0,              PixelFormatType::Unknown,   {false, false, false},              {1, 1},                                                  {0, 0, 0, 0}},
        // 8-bit pixel formats
        {PixelFormat::R8Unorm,              "R8UNorm",      1,              1,              PixelFormatType::Unorm,     {false, false, false},              {1, 1},                                                  {8, 0, 0, 0}},
        {PixelFormat::R8Snorm,              "R8Snorm",      1,              1,              PixelFormatType::Snorm,     {false, false, false},              {1, 1},                                                  {8, 0, 0, 0}},
        {PixelFormat::R8Uint,               "R8Uint",       1,              1,              PixelFormatType::Uint,      {false, false, false},              {1, 1},                                                  {8, 0, 0, 0}},
        {PixelFormat::R8Sint,               "R8Sint",       1,              1,              PixelFormatType::Sint,      {false, false, false},              {1, 1},                                                  {8, 0, 0, 0}},
        // 16-bit pixel formats
        {PixelFormat::R16Uint,              "R16Uint",      2,              1,              PixelFormatType::Uint,       {false, false, false},             {1, 1},                                                  {16, 0, 0, 0}},
        {PixelFormat::R16Sint,              "R16Sint",      2,              1,              PixelFormatType::Sint,       {false, false, false},             {1, 1},                                                  {16, 0, 0, 0}},
        {PixelFormat::R16Float,             "R16Float",     2,              1,              PixelFormatType::Float,      {false, false, false},             {1, 1},                                                  {16, 0, 0, 0}},
        {PixelFormat::RG8Unorm,             "RG8Unorm",     2,              2,              PixelFormatType::Unorm,      {false, false, false},             {1, 1},                                                  {8, 8, 0, 0}},
        {PixelFormat::RG8Snorm,             "RG8Snorm",     2,              2,              PixelFormatType::Snorm,      {false, false, false},             {1, 1},                                                  {8, 8, 0, 0}},
        {PixelFormat::RG8Uint,              "RG8Uint",      2,              2,              PixelFormatType::Uint,       {false, false, false},             {1, 1},                                                  {8, 8, 0, 0}},
        {PixelFormat::RG8Sint,              "RG8Sint",      2,              2,              PixelFormatType::Sint,       {false, false, false},             {1, 1},                                                  {8, 8, 0, 0}},
        // 32-bit pixel formats
        {PixelFormat::R32Uint,              "R32Uint",      4,              1,              PixelFormatType::Uint,       {false, false, false},             {1, 1},                                                  {32, 0, 0, 0}},
        {PixelFormat::R32Sint,              "R32Sint",      4,              1,              PixelFormatType::Sint,       {false, false, false},             {1, 1},                                                  {32, 0, 0, 0}},
        {PixelFormat::R32Float,             "R32Float",     4,              1,              PixelFormatType::Float,      {false, false, false},             {1, 1},                                                  {32, 0, 0, 0}},
        {PixelFormat::RG16Uint,             "RG16Uint",     4,              2,              PixelFormatType::Uint,       {false, false, false},             {1, 1},                                                  {16, 16, 0, 0}},
        {PixelFormat::RG16Sint,             "RG16Sint",     4,              2,              PixelFormatType::Sint,       {false, false, false},             {1, 1},                                                  {16, 16, 0, 0}},
        {PixelFormat::RG16Float,            "RG16Float",        4,          2,              PixelFormatType::Float,      {false, false, false},             {1, 1},                                                  {16, 16, 0, 0}},
        {PixelFormat::RGBA8Unorm,           "RGBA8Unorm",       4,          4,              PixelFormatType::Unorm,      {false, false, false},             {1, 1},                                                  {8, 8, 8, 8}},
        {PixelFormat::RGBA8UnormSrgb,       "RGBA8UnormSrgb",   4,          4,              PixelFormatType::UnormSrgb,  {false, false, false},             {1, 1},                                                  {8, 8, 8, 8}},
        {PixelFormat::RGBA8Uint,            "RGBA8Uint",        4,          4,              PixelFormatType::Uint,       {false, false, false},             {1, 1},                                                  {8, 8, 8, 8}},
        {PixelFormat::RGBA8Sint,            "RGBA8Sint",        4,          4,              PixelFormatType::Sint,       {false, false, false},             {1, 1},                                                  {8, 8, 8, 8}},
        {PixelFormat::BGRA8Unorm,           "BGRA8Unorm",       4,          4,              PixelFormatType::Unorm,      {false, false, false},             {1, 1},                                                  {8, 8, 8, 8}},
        {PixelFormat::BGRA8UnormSrgb,       "BGRA8UnormSrgb",   4,          4,              PixelFormatType::UnormSrgb,  {false, false, false},             {1, 1},                                                  {8, 8, 8, 8}},
        // Packed 32-Bit Pixel formats
        {PixelFormat::RGB10A2Unorm,         "RGB10A2Unorm",     4,          4,              PixelFormatType::Unorm,      {false, false, false},             {1, 1},                                                  {10, 10, 10, 2}},
        {PixelFormat::RG11B10Float,         "RG11B10Float",     4,          3,              PixelFormatType::Float,      {false, false, false},             {1, 1},                                                  {11, 11, 10, 0}},
        // 64-bit pixel formats
        {PixelFormat::RG32Uint,             "RG32Uint",         8,          2,              PixelFormatType::Uint,       {false, false, false},             {1, 1},                                                  {32, 32, 0, 0}},
        {PixelFormat::RG32Sint,             "RG32Sint",         8,          2,              PixelFormatType::Sint,       {false, false, false},             {1, 1},                                                  {32, 32, 0, 0}},
        {PixelFormat::RG32Float,            "RG32Float",        8,          2,              PixelFormatType::Float,      {false, false, false},             {1, 1},                                                  {32, 32, 0, 0}},
        {PixelFormat::RGBA16Uint,           "RGBA16Uint",       8,          4,              PixelFormatType::Uint,       {false, false, false},             {1, 1},                                                  {16, 16, 16, 16}},
        {PixelFormat::RGBA16Sint,           "RGBA16Sint",       8,          4,              PixelFormatType::Sint,       {false, false, false},             {1, 1},                                                  {16, 16, 16, 16}},
        {PixelFormat::RGBA16Float,          "RGBA16Float",      8,          4,              PixelFormatType::Float,      {false, false, false},             {1, 1},                                                  {16, 16, 16, 16}},
        // 128-bit pixel formats
        {PixelFormat::RGBA32Uint,           "RGBA32Uint",       16,         4,              PixelFormatType::Uint,       {false, false, false},             {1, 1},                                                  {32, 32, 32, 32}},
        {PixelFormat::RGBA32Sint,           "RGBA32Sint",       16,         4,              PixelFormatType::Sint,       {false, false, false},             {1, 1},                                                  {32, 32, 32, 32}},
        {PixelFormat::RGBA32Float,          "RGBA32Float",      16,         4,              PixelFormatType::Float,      {false, false, false},             {1, 1},                                                  {32, 32, 32, 32}},
        // Depth and Stencil Pixel Formats
        {PixelFormat::Depth32Float,         "Depth32Float",     4,          1,              PixelFormatType::Float,      {true, false, false},              {1, 1},                                                  {32, 0, 0, 0}},
        {PixelFormat::Depth24UnormStencil8, "Depth24UnormStencil8",  4,     2,              PixelFormatType::Unorm,      {true, true, false},               {1, 1},                                                  {24, 8, 0, 0}},
        // Compressed BC Pixel Formats
        {PixelFormat::BC1RGBAUnorm,         "BC1RGBAUnorm",        8,       3,              PixelFormatType::Unorm,      {false, false, true},              {4, 4},                                                  {64, 0, 0, 0}},
        {PixelFormat::BC1RGBAUnormSrgb,     "BC1RGBAUnormSrgb",    8,       3,              PixelFormatType::UnormSrgb,  {false, false, true},              {4, 4},                                                  {64, 0, 0, 0}},
        {PixelFormat::BC2RGBAUnorm,         "BC2RGBAUnorm",        16,      4,              PixelFormatType::Unorm,      {false, false, true},              {4, 4},                                                  {128, 0, 0, 0}},
        {PixelFormat::BC2RGBAUnormSrgb,     "BC2RGBAUnormSrgb",    16,      4,              PixelFormatType::UnormSrgb,  {false, false, true},              {4, 4},                                                  {128, 0, 0, 0}},
        {PixelFormat::BC3RGBAUnorm,         "BC3RGBAUnorm",        16,      4,              PixelFormatType::Unorm,      {false, false, true},              {4, 4},                                                  {128, 0, 0, 0}},
        {PixelFormat::BC3RGBAUnormSrgb,     "BC3RGBAUnormSrgb",    16,      4,              PixelFormatType::UnormSrgb,  {false, false, true},              {4, 4},                                                  {128, 0, 0, 0}},
        {PixelFormat::BC4RUnorm,            "BC4RUnorm",            8,      1,              PixelFormatType::Unorm,      {false, false, true},              {4, 4},                                                  {64, 0, 0, 0}},
        {PixelFormat::BC4RSnorm,            "BC4RSnorm",            8,      1,              PixelFormatType::Snorm,      {false, false, true},              {4, 4},                                                  {64, 0, 0, 0}},
        {PixelFormat::BC5RGUnorm,           "BC5RGUnorm",          16,      2,              PixelFormatType::Unorm,      {false, false, true},              {4, 4},                                                  {128, 0, 0, 0}},
        {PixelFormat::BC5RGSnorm,           "BC5RGSnorm",          16,      2,              PixelFormatType::Snorm,      {false, false, true},              {4, 4},                                                  {128, 0, 0, 0}},
        {PixelFormat::BC6HRGBUfloat,        "BC6HRGBUfloat",       16,      3,              PixelFormatType::Float,      {false, false, true},              {4, 4},                                                  {128, 0, 0, 0}},
        {PixelFormat::BC6HRGBSfloat,        "BC6HRGBSfloat",       16,      3,              PixelFormatType::Float,      {false, false, true},              {4, 4},                                                  {128, 0, 0, 0}},
        {PixelFormat::BC7RGBAUnorm,         "BC7RGBAUnorm",        16,      4,              PixelFormatType::Unorm,      {false, false, true},              {4, 4},                                                  {128, 0, 0, 0}},
        {PixelFormat::BC7RGBAUnormSrgb,     "BC7RGBAUnormrgb",     16,      4,              PixelFormatType::UnormSrgb,  {false, false, true},              {4, 4},                                                  {128, 0, 0, 0}},
    };

    uint32_t getFormatBytesPerBlock(PixelFormat format) {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].bytesPerBlock;
    }

    uint32_t getFormatPixelsPerBlock(PixelFormat format) {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].compressionRatio.width * kFormatDesc[(uint32_t)format].compressionRatio.height;
    }

    bool isDepthFormat(PixelFormat format) {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].isDepth;
    }

    bool isStencilFormat(PixelFormat format) {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].isStencil;
    }

    bool isDepthStencilFormat(PixelFormat format) {
        return isDepthFormat(format) || isStencilFormat(format);
    }

    bool isCompressedFormat(PixelFormat format) {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].isCompressed;
    }

    uint32_t getFormatWidthCompressionRatio(PixelFormat format) {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].compressionRatio.width;
    }

    uint32_t getFormatHeightCompressionRatio(PixelFormat format) {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].compressionRatio.height;
    }

    uint32_t getFormatChannelCount(PixelFormat format) {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].channelCount;
    }

    PixelFormatType getFormatType(PixelFormat format) {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].type;
    }

    PixelFormatAspect getFormatAspect(PixelFormat format) {
        switch (format)
        {
        case PixelFormat::Depth32Float:
            return PixelFormatAspect::Depth;

        case PixelFormat::Depth24UnormStencil8:
            return PixelFormatAspect::DepthStencil;

        default:
            return PixelFormatAspect::Color;
        }
    }

    bool isSrgbFormat(PixelFormat format) {
        return (getFormatType(format) == PixelFormatType::UnormSrgb);
    }

    PixelFormat srgbToLinearFormat(PixelFormat format) {

        switch (format)
        {
        case PixelFormat::BC1RGBAUnormSrgb:
            return PixelFormat::BC1RGBAUnorm;
        case PixelFormat::BC2RGBAUnormSrgb:
            return PixelFormat::BC2RGBAUnorm;
        case PixelFormat::BC3RGBAUnormSrgb:
            return PixelFormat::BC3RGBAUnorm;
        case PixelFormat::BGRA8UnormSrgb:
            return PixelFormat::BGRA8Unorm;
        case PixelFormat::RGBA8UnormSrgb:
            return PixelFormat::RGBA8Unorm;
        case PixelFormat::BC7RGBAUnormSrgb:
            return PixelFormat::BC7RGBAUnorm;
        default:
            assert(isSrgbFormat(format) == false);
            return format;
        }
    }

    PixelFormat linearToSrgbFormat(PixelFormat format) {
        switch (format)
        {
        case PixelFormat::BC1RGBAUnorm:
            return PixelFormat::BC1RGBAUnormSrgb;
        case PixelFormat::BC2RGBAUnorm:
            return PixelFormat::BC2RGBAUnormSrgb;
        case PixelFormat::BC3RGBAUnorm:
            return PixelFormat::BC3RGBAUnormSrgb;
        case PixelFormat::BGRA8Unorm:
            return PixelFormat::BGRA8UnormSrgb;
        case PixelFormat::RGBA8Unorm:
            return PixelFormat::RGBA8UnormSrgb;
        case PixelFormat::BC7RGBAUnorm:
            return PixelFormat::BC7RGBAUnormSrgb;
        default:
            return format;
        }
    }
}
