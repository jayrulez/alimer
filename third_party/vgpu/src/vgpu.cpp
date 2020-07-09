//
// Copyright (c) 2020 Amer Koleci and contributors.
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
#include <string.h> /* memset */

namespace vgpu
{
    /* Log */
#define VGPU_MAX_LOG_MESSAGE (4096)
    static LogCallback s_log_callback = nullptr;
    static void* s_log_userData = nullptr;

    void setLogCallback(LogCallback callback, void* userData) {
        s_log_callback = callback;
        s_log_userData = userData;
    }

    void log(vgpu_log_level level, const char* format, ...) {
        if (s_log_callback) {
            va_list args;
            va_start(args, format);
            char message[VGPU_MAX_LOG_MESSAGE];
            vsnprintf(message, VGPU_MAX_LOG_MESSAGE, format, args);
            s_log_callback(s_log_userData, level, message);
            va_end(args);
        }
    }

    void logError(const char* format, ...) {
        if (s_log_callback) {
            va_list args;
            va_start(args, format);
            char message[VGPU_MAX_LOG_MESSAGE];
            vsnprintf(message, VGPU_MAX_LOG_MESSAGE, format, args);
            s_log_callback(s_log_userData, VGPU_LOG_LEVEL_ERROR, message);
            va_end(args);
        }
    }

    void logInfo(const char* format, ...) {
        if (s_log_callback) {
            va_list args;
            va_start(args, format);
            char message[VGPU_MAX_LOG_MESSAGE];
            vsnprintf(message, VGPU_MAX_LOG_MESSAGE, format, args);
            s_log_callback(s_log_userData, VGPU_LOG_LEVEL_INFO, message);
            va_end(args);
        }
    }


    /* Drivers */
    static const Driver* drivers[] = {
    #if defined(VGPU_DRIVER_D3D11)
        &d3d11_driver,
    #endif
    #if defined(VGPU_DRIVER_D3D12) 
        &d3d12_driver,
    #endif
    #if defined(VGPU_DRIVER_VULKAN)
        &vulkan_driver,
    #endif
        nullptr
    };

    static BackendType s_backendType = BackendType::Count;
    static Renderer* s_renderer = nullptr;

    bool init(InitFlags flags, const PresentationParameters& presentationParameters)
    {
        if (s_renderer) {
            return true;
        }

        if (s_backendType == BackendType::Count) {
            for (uint32_t i = 0; _vgpu_count_of(drivers); i++) {
                if (drivers[i]->isSupported()) {
                    s_renderer = drivers[i]->initRenderer();
                    break;
                }
            }
        }
        else {
            for (uint32_t i = 0; _vgpu_count_of(drivers); i++) {
                if (drivers[i]->backendType == s_backendType && drivers[i]->isSupported()) {
                    s_renderer = drivers[i]->initRenderer();
                    break;
                }
            }
        }

        if (!s_renderer->init(flags, presentationParameters)) {
            s_renderer = nullptr;
            return false;
        }

        return true;
    }

    void shutdown(void)
    {
        if (s_renderer == nullptr) {
            return;
        }

        s_renderer->shutdown();
        s_renderer = nullptr;
    }

    void beginFrame(void)
    {
        s_renderer->beginFrame();
    }

    void endFrame(void)
    {
        s_renderer->endFrame();
    }

    const Caps* queryCaps()
    {
        return s_renderer->queryCaps();
    }

    /* Commands */
    void cmdSetViewport(CommandList commandList, float x, float y, float width, float height, float min_depth, float max_depth)
    {
        s_renderer->cmdSetViewport(commandList, x, y, width, height, min_depth, max_depth);
    }

    void cmdSetScissor(CommandList commandList, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        s_renderer->cmdSetScissor(commandList, x, y, width, height);
    }

    /* Pixel format helpers */
    const PixelFormatDesc kFormatDesc[] =
    {
        // Format, Name, BytesPerBlock, ChannelCount, Type, {depth, stencil, compressed}, {CompressionRatio.Width,CompressionRatio.Height}, {numChannelBits.x, numChannelBits.y, numChannelBits.z, numChannelBits.w}
        {PixelFormat::Invalid,                  "Invalid",      0,      0,  PixelFormatType::Unknown,    {false, false, false},       {1, 1},     {0, 0, 0, 0}},
        {PixelFormat::R8Unorm,                  "R8Unorm",      1,      1,  PixelFormatType::Unorm,      {false, false, false},       {1, 1},     {8, 0, 0, 0}},
        {PixelFormat::R8Snorm,                  "R8Snorm",      1,      1,  PixelFormatType::Snorm,      {false, false, false},       {1, 1},     {8, 0, 0, 0}},
        {PixelFormat::R8Uint,                   "R8Int",        1,      1,  PixelFormatType::Sint,       {false, false, false},       {1, 1},     {8, 0, 0, 0}},
        {PixelFormat::R8Sint,                   "R8uint",       1,      1,  PixelFormatType::Uint,       {false, false, false},       {1, 1},     {8, 0, 0, 0}},

        {PixelFormat::R16Uint,                  "R16Uint",      2,      1,  PixelFormatType::Uint,       {false, false, false},      {1, 1},     {16, 0, 0, 0}},
        {PixelFormat::R16Sint,                  "R16Int",       2,      1,  PixelFormatType::Sint,       {false, false, false},      {1, 1},     {16, 0, 0, 0}},
        {PixelFormat::R16Float,                 "R16Float",     2,      1,  PixelFormatType::Float,      {false, false, false},      {1, 1},     {16, 0, 0, 0}},
        {PixelFormat::RG8Unorm,                 "RG8Unorm",     2,      2,  PixelFormatType::Unorm,      {false, false, false},      {1, 1},     {8, 8, 0, 0}},
        {PixelFormat::RG8Snorm,                 "RG8Snorm",     2,      2,  PixelFormatType::Snorm,      {false, false, false},      {1, 1},     {8, 8, 0, 0}},
        {PixelFormat::RG8Uint,                  "RG8Uint",      2,      2,  PixelFormatType::Uint,       {false, false, false},      {1, 1},     {8, 8, 0, 0}},
        {PixelFormat::RG8Sint,                  "RG8Int",       2,      2,  PixelFormatType::Sint,       {false, false, false},      {1, 1},     {8, 8, 0, 0}},

        {PixelFormat::R32Uint,                  "R32Uint",      4,      1,  PixelFormatType::Uint,       {false, false, false},      {1, 1},     {32, 0, 0, 0}},
        {PixelFormat::R32Sint,                  "R32Sint",      4,      1,  PixelFormatType::Sint,       {false, false, false},      {1, 1},     {32, 0, 0, 0}},
        {PixelFormat::R32Float,                 "R32Float",     4,      1,  PixelFormatType::Float,      {false, false, false},      {1, 1},     {32, 0, 0, 0}},

        {PixelFormat::RG16Uint,                 "RG16Uint",         4,      2,  PixelFormatType::Uint,       {false,  false, false},      {1, 1},     {16, 16, 0, 0}},
        {PixelFormat::RG16Sint,                 "RG16Int",          4,      2,  PixelFormatType::Sint,       {false,  false, false},      {1, 1},     {16, 16, 0, 0}},
        {PixelFormat::RG16Float,                "RG16Float",        4,      2,  PixelFormatType::Float,      {false,  false, false},      {1, 1},     {16, 16, 0, 0}},
        {PixelFormat::RGBA8Unorm,               "RGBA8Unorm",       4,      4,  PixelFormatType::Unorm,      {false,  false, false},      {1, 1},     {8, 8, 8, 8}},
        {PixelFormat::RGBA8UnormSrgb,           "RGBA8UnormSrgb",   4,      4,  PixelFormatType::UnormSrgb,  {false, false, false},      {1, 1},     {8, 8, 8, 8}},
        {PixelFormat::RGBA8Snorm,               "RGBA8Snorm",       4,      4,  PixelFormatType::Snorm,      {false, false, false},       {1, 1},     {8, 8, 8, 8}},
        {PixelFormat::RGBA8Uint,                "RGBA8Uint",        4,      4,  PixelFormatType::Uint,       {false, false, false},       {1, 1},     {8, 8, 8, 8}},
        {PixelFormat::RGBA8Sint,                "RGBA8Sint",        4,      4,  PixelFormatType::Sint,       {false, false, false},       {1, 1},     {8, 8, 8, 8}},
        {PixelFormat::BGRA8Unorm,               "BGRA8Unorm",       4,      4,  PixelFormatType::Unorm,      {false, false, false},       {1, 1},     {8, 8, 8, 8}},
        {PixelFormat::BGRA8UnormSrgb,           "BGRA8UnormSrgb",   4,      4,  PixelFormatType::UnormSrgb,  {false, false, false},      {1, 1},     {8, 8, 8, 8}},

        {PixelFormat::RGB10A2Unorm,             "RGB10A2Unorm",     4,      4,  PixelFormatType::Unorm,       {false,  false, false},     {1, 1},     {10, 10, 10, 2}},
        {PixelFormat::RG11B10Float,             "RG11B10Float",     4,      3,  PixelFormatType::Float,       {false,  false, false},     {1, 1},     {11, 11, 10, 0}},

        {PixelFormat::RG32Uint,                 "RG32Uint",         8,      2,  PixelFormatType::Uint,       {false,  false, false},      {1, 1},     {32, 32, 0, 0}},
        {PixelFormat::RG32Sint,                 "RG32Sin",          8,      2,  PixelFormatType::Sint,       {false,  false, false},      {1, 1},     {32, 32, 0, 0}},
        {PixelFormat::RG32Float,                "RG32Float",        8,      2,  PixelFormatType::Float,      {false,  false, false},      {1, 1},     {32, 32, 0, 0}},
        {PixelFormat::RGBA16Uint,               "RGBA16Uint",       8,      4,  PixelFormatType::Uint,       {false,  false, false},      {1, 1},     {16, 16, 16, 16}},
        {PixelFormat::RGBA16Sint,               "RGBA16Sint",       8,      4,  PixelFormatType::Sint,       {false,  false, false},      {1, 1},     {16, 16, 16, 16}},
        {PixelFormat::RGBA16Float,              "RGBA16Float",      8,      4,  PixelFormatType::Float,      {false,  false, false},      {1, 1},     {16, 16, 16, 16}},

        {PixelFormat::RGBA32Uint,               "RGBA32Uint",       16,     4,  PixelFormatType::Uint,       {false,  false, false},      {1, 1},     {32, 32, 32, 32}},
        {PixelFormat::RGBA32Sint,               "RGBA32Sint",        16,     4,  PixelFormatType::Sint,       {false,  false, false},      {1, 1},     {32, 32, 32, 32}},
        {PixelFormat::RGBA32Float,              "RGBA32Float",      16,     4,  PixelFormatType::Float,      {false,  false, false},      {1, 1},     {32, 32, 32, 32}},

        {PixelFormat::Depth16Unorm,             "Depth16Unorm",             2,  1,  PixelFormatType::Unorm,     {true, false, false},   {1, 1},     {16, 0, 0, 0}},
        {PixelFormat::Depth32Float,             "Depth32Float",             4,  1,  PixelFormatType::Float,     {true, false, false},   {1, 1},     {32, 0, 0, 0}},
        {PixelFormat::Depth24UnormStencil8,     "Depth24UnormStencil8",     4,  2,  PixelFormatType::Unorm,     {true, true, false},    {1, 1},     {24, 8, 0, 0}},

        {PixelFormat::BC1RGBAUnorm,             "BC1RGBAUnorm",         8,      3,  PixelFormatType::Unorm,       {false,  false, true},      {4, 4},     {64, 0, 0, 0}},
        {PixelFormat::BC1RGBAUnormSrgb,         "BC1RGBAUnormSrgb",     8,      3,  PixelFormatType::UnormSrgb,  {false,  false, true},      {4, 4},     {64, 0, 0, 0}},
        {PixelFormat::BC2RGBAUnorm,             "BC2RGBAUnorm",         16,     4,  PixelFormatType::Unorm,       {false,  false, true},      {4, 4},     {128, 0, 0, 0}},
        {PixelFormat::BC2RGBAUnormSrgb,         "BC2RGBAUnormSrgb",     16,     4,  PixelFormatType::UnormSrgb,  {false,  false, true},      {4, 4},     {128, 0, 0, 0}},
        {PixelFormat::BC3RGBAUnorm,             "BC3RGBAUnorm",         16,     4,  PixelFormatType::Unorm,       {false,  false, true},      {4, 4},     {128, 0, 0, 0}},
        {PixelFormat::BC3RGBAUnormSrgb,         "BC3RGBAUnormSrgb",     16,     4,  PixelFormatType::UnormSrgb,  {false,  false, true},      {4, 4},     {128, 0, 0, 0}},
        {PixelFormat::BC4RUnorm,                "BC4RUnorm",            8,      1,  PixelFormatType::Unorm,       {false,  false, true},      {4, 4},     {64, 0, 0, 0}},
        {PixelFormat::BC4RSnorm,                "BC4RSnorm",            8,      1,  PixelFormatType::Snorm,       {false,  false, true},      {4, 4},     {64, 0, 0, 0}},
        {PixelFormat::BC5RGUnorm,               "BC5RGUnorm",           16,     2,  PixelFormatType::Unorm,       {false,  false, true},      {4, 4},     {128, 0, 0, 0}},
        {PixelFormat::BC5RGSnorm,               "BC5RGSnorm",           16,     2,  PixelFormatType::Snorm,       {false,  false, true},      {4, 4},     {128, 0, 0, 0}},
        {PixelFormat::BC6HRGBUfloat,            "BC6HRGBUfloat",              16,     3,  PixelFormatType::Float,       {false,  false, true},      {4, 4},     {128, 0, 0, 0}},
        {PixelFormat::BC6HRGBSfloat,            "BC6HRGBSfloat",              16,     3,  PixelFormatType::Float,       {false,  false, true},      {4, 4},     {128, 0, 0, 0}},
        {PixelFormat::BC7RGBAUnorm,             "BC7RGBAUnorm",         16,     4,  PixelFormatType::Unorm,       {false,  false, true},      {4, 4},     {128, 0, 0, 0}},
        {PixelFormat::BC7RGBAUnormSrgb,         "BC7RGBAUnormSrgb",     16,     4,  PixelFormatType::UnormSrgb,  {false,  false, true},      {4, 4},     {128, 0, 0, 0}},
    };

    static_assert(_vgpu_count_of(kFormatDesc) == (unsigned)PixelFormat::Count, "Format desc table has a wrong size");
}
