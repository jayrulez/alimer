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

#include "graphics/PixelFormat.h"

namespace alimer
{
    const PixelFormatDesc kFormatDesc[] =
    {
        // format                               name,                       type                            bpp         compression             bits
        { PixelFormat::Unknown,                 "Unknown",                  PixelFormatType::Unknown,       0,          {0, 0, 0, 0, 0},        {0, 0, 0, 0, 0, 0}},

        // 8-bit pixel formats
        { PixelFormat::R8Unorm,                 "R8Unorm",                  PixelFormatType::UNorm,         8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
        { PixelFormat::R8Snorm,                 "R8Snorm",                  PixelFormatType::SNorm,         8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
        { PixelFormat::R8Uint,                  "R8Uint",                   PixelFormatType::UInt,          8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
        { PixelFormat::R8Sint,                  "R8Sint",                   PixelFormatType::SInt,          8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},

        // 16-bit pixel formats
        { PixelFormat::R16Unorm,                "R16Unorm",                 PixelFormatType::UNorm,         16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
        { PixelFormat::R16Snorm,                "R16Snorm",                 PixelFormatType::SNorm,         16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
        { PixelFormat::R16Uint,                 "R16Uint",                  PixelFormatType::UInt,          16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
        { PixelFormat::R16Sint,                 "R16Sint",                  PixelFormatType::SInt,          16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
        { PixelFormat::R16Float,                "R16Float",                 PixelFormatType::Float,         16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
        { PixelFormat::Rg8Unorm,                "Rg8Unorm",                 PixelFormatType::UNorm,         16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
        { PixelFormat::Rg8Snorm,                "Rg8Snorm",                 PixelFormatType::SNorm,         16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
        { PixelFormat::Rg8Uint,                 "Rg8Uint",                  PixelFormatType::UInt,          16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
        { PixelFormat::Rg8Sint,                 "Rg8Sint",                  PixelFormatType::SInt,          16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},

        // 32-bit pixel formats
        { PixelFormat::R32Uint,                 "R32Uint",                  PixelFormatType::UInt,          32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
        { PixelFormat::R32Sint,                 "R32Sint",                  PixelFormatType::SInt,          32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
        { PixelFormat::R32Float,                "R32Float",                 PixelFormatType::Float,         32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
        { PixelFormat::Rg16Unorm,               "Rg16Unorm",                PixelFormatType::UNorm,         32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
        { PixelFormat::Rg16Snorm,               "Rg16Snorm",                PixelFormatType::SNorm,         32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
        { PixelFormat::Rg16Uint,                "Rg16Uint",                 PixelFormatType::UInt,          32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
        { PixelFormat::Rg16Sint,                "Rg16Sint",                 PixelFormatType::SInt,          32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
        { PixelFormat::Rg16Float,               "Rg16Float",                PixelFormatType::Float,         32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
        { PixelFormat::Rgba8Unorm,              "Rgba8Unorm",               PixelFormatType::UNorm,         32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
        { PixelFormat::Rgba8UnormSrgb,          "Rgba8UnormSrgb",           PixelFormatType::UnormSrgb,     32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
        { PixelFormat::Rgba8Snorm,              "Rgba8Snorm",               PixelFormatType::SNorm,         32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
        { PixelFormat::Rgba8Uint,               "Rgba8Uint",                PixelFormatType::UInt,          32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
        { PixelFormat::Rgba8Sint,               "Rgba8Sint",                PixelFormatType::SInt,          32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
        { PixelFormat::Bgra8Unorm,              "Bgra8Unorm",               PixelFormatType::UNorm,         32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
        { PixelFormat::Bgra8UnormSrgb,          "Bgra8UnormSrgb",           PixelFormatType::UnormSrgb,     32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},

        // Packed 32-Bit Pixel formats
        { PixelFormat::Rgb10a2Unorm,            "Rgb10a2Unorm",             PixelFormatType::UNorm,         32,         {1, 1, 4, 1, 1},        {0, 0, 10, 10, 10, 2}},
        { PixelFormat::Rg11b10Float,            "Rg11b10Float",             PixelFormatType::Float,         32,         {1, 1, 4, 1, 1},        {0, 0, 11, 11, 10, 0}},

        // 64-Bit Pixel Formats
        { PixelFormat::RG32Uint,                "RG32Uint",                 PixelFormatType::UInt,          64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
        { PixelFormat::RG32Sint,                "RG32Sint",                 PixelFormatType::SInt,          64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
        { PixelFormat::RG32Float,               "RG32Float",                PixelFormatType::Float,         64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
        { PixelFormat::Rgba16Unorm,             "Rgba16Unorm",              PixelFormatType::UNorm,         64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
        { PixelFormat::Rgba16Snorm,             "Rgba16Snorm",              PixelFormatType::SNorm,         64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
        { PixelFormat::Rgba16Uint,              "Rgba16Uint",               PixelFormatType::UInt,          64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
        { PixelFormat::Rgba16Sint,              "Rgba16Sint",               PixelFormatType::SInt,          64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
        { PixelFormat::Rgba16Float,             "Rgba16Float",              PixelFormatType::Float,         64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},

        // 128-Bit Pixel Formats
        { PixelFormat::Rgba32Uint,              "RGBA32Uint",               PixelFormatType::UInt,          128,        {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},
        { PixelFormat::Rgba32Sint,              "RGBA32Sint",               PixelFormatType::SInt,          128,        {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},
        { PixelFormat::Rgba32Float,             "RGBA32Float",              PixelFormatType::Float,         128,        {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},

        // Depth-stencil
        { PixelFormat::Depth32Float,            "Depth32Float",             PixelFormatType::UNorm,         32,         {1, 1, 4, 1, 1},        {32, 0, 0, 0, 0, 0}},
        { PixelFormat::Depth16Unorm,            "Depth16Unorm",             PixelFormatType::UNorm,         16,         {1, 1, 2, 1, 1},        {16, 0, 0, 0, 0, 0}},
        { PixelFormat::Depth24Plus,             "Depth24Plus",              PixelFormatType::Float,         32,         {1, 1, 4, 1, 1},        {32, 8, 0, 0, 0, 0}},
        { PixelFormat::Depth24PlusStencil8,     "Depth24PlusStencil8",      PixelFormatType::UNorm,         32,         {1, 1, 4, 1, 1},        {24, 8, 0, 0, 0, 0}},

        // Compressed formats
        { PixelFormat::Bc1RgbaUnorm,            "Bc1RgbaUnorm",             PixelFormatType::UNorm,         4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
        { PixelFormat::Bc1RgbaUnormSrgb,        "Bc1RgbaUnormSrgb",         PixelFormatType::UnormSrgb,     4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
        { PixelFormat::Bc2RgbaUnorm,            "Bc2RgbaUnorm",             PixelFormatType::UNorm,         8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
        { PixelFormat::Bc2RgbaUnormSrgb,        "Bc2RgbaUnormSrgb",         PixelFormatType::UnormSrgb,     8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
        { PixelFormat::Bc3RgbaUnorm,            "Bc3RgbaUnorm",             PixelFormatType::UNorm,         8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
        { PixelFormat::Bc3RgbaUnormSrgb,        "Bc3RgbaUnormSrgb",         PixelFormatType::UnormSrgb,     8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
        { PixelFormat::Bc4RUnorm,               "Bc4RUnorm",                PixelFormatType::UNorm,         4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
        { PixelFormat::Bc4RSnorm,               "Bc4RSnorm",                PixelFormatType::SNorm,         4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
        { PixelFormat::Bc5RgUnorm,              "Bc5RGUNorm",               PixelFormatType::UNorm,         8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
        { PixelFormat::Bc5RgSnorm,              "Bc5RGSNorm",               PixelFormatType::SNorm,         8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
        { PixelFormat::Bc6HRgbUfloat,           "Bc6HRgbUfloat",            PixelFormatType::Float,         8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
        { PixelFormat::Bc6HRgbSfloat,           "Bc6HRgbSfloat",            PixelFormatType::Float,         8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
        { PixelFormat::Bc7RgbaUnorm,            "Bc7RGBAUnorm",             PixelFormatType::UNorm,         8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
        { PixelFormat::Bc7RgbaUnormSrgb,        "Bc7RGBAUnormSrgb",         PixelFormatType::UnormSrgb,     8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    };
}
