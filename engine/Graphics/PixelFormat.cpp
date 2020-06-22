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
        { PixelFormat::R8UNorm,                 "R8UNorm",                  PixelFormatType::UNorm,         8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
        { PixelFormat::R8SNorm,                 "R8SNorm",                  PixelFormatType::SNorm,         8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
        { PixelFormat::R8UInt,                  "R8UInt",                   PixelFormatType::UInt,          8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},
        { PixelFormat::R8SInt,                  "R8SInt",                   PixelFormatType::SInt,          8,          {1, 1, 1, 1, 1},        {0, 0, 8, 0, 0, 0}},

        // 16-bit pixel formats
        { PixelFormat::R16UNorm,                "R16UNorm",                 PixelFormatType::UNorm,         16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
        { PixelFormat::R16SNorm,                "R16SNorm",                 PixelFormatType::SNorm,         16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
        { PixelFormat::R16UInt,                 "R16UInt",                  PixelFormatType::UInt,          16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
        { PixelFormat::R16SInt,                 "R16SInt",                  PixelFormatType::SInt,          16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
        { PixelFormat::R16Float,                "R16Float",                 PixelFormatType::Float,         16,         {1, 1, 2, 1, 1},        {0, 0, 16, 0, 0, 0}},
        { PixelFormat::RG8UNorm,                "RG8UNorm",                 PixelFormatType::UNorm,         16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
        { PixelFormat::RG8SNorm,                "RG8SNorm",                 PixelFormatType::SNorm,         16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
        { PixelFormat::RG8UInt,                 "RG8UInt",                  PixelFormatType::UInt,          16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},
        { PixelFormat::RG8SInt,                 "RG8SInt",                  PixelFormatType::SInt,          16,         {1, 1, 2, 1, 1},        {0, 0, 8, 8, 0, 0}},

        // 32-bit pixel formats
        { PixelFormat::R32UInt,                 "R32UInt",                  PixelFormatType::UInt,          32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
        { PixelFormat::R32SInt,                 "R32SInt",                  PixelFormatType::SInt,          32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
        { PixelFormat::R32Float,                "R32Float",                 PixelFormatType::Float,         32,         {1, 1, 4, 1, 1},        {0, 0, 32, 0, 0, 0}},
        { PixelFormat::RG16UNorm,               "RG16UNorm",                PixelFormatType::UNorm,         32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
        { PixelFormat::RG16SNorm,               "RG16SNorm",                PixelFormatType::SNorm,         32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
        { PixelFormat::RG16UInt,                "RG16UInt",                 PixelFormatType::UInt,          32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
        { PixelFormat::RG16SInt,                "RG16SInt",                 PixelFormatType::SInt,          32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
        { PixelFormat::RG16Float,               "RG16Float",                PixelFormatType::Float,         32,         {1, 1, 4, 1, 1},        {0, 0, 16, 16, 0, 0}},
        { PixelFormat::RGBA8UNorm,              "RGBA8UNorm",               PixelFormatType::UNorm,         32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
        { PixelFormat::RGBA8UNormSrgb,          "RGBA8UNormSrgb",           PixelFormatType::UnormSrgb,     32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
        { PixelFormat::RGBA8SNorm,              "RGBA8SNorm",               PixelFormatType::SNorm,         32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
        { PixelFormat::RGBA8UInt,               "RGBA8UInt",                PixelFormatType::UInt,          32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
        { PixelFormat::RGBA8SInt,               "RGBA8SInt",                PixelFormatType::SInt,          32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
        { PixelFormat::BGRA8Unorm,              "BGRA8Unorm",               PixelFormatType::UNorm,         32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},
        { PixelFormat::BGRA8UnormSrgb,          "BGRA8UnormSrgb",           PixelFormatType::UnormSrgb,     32,         {1, 1, 4, 1, 1},        {0, 0, 8, 8, 8, 8}},

        // Packed 32-Bit Pixel formats
        { PixelFormat::RGB10A2UNorm,            "RGB10A2UNorm",             PixelFormatType::UNorm,         32,         {1, 1, 4, 1, 1},        {0, 0, 10, 10, 10, 2}},
        { PixelFormat::RG11B10Float,            "RG11B10Float",             PixelFormatType::Float,         32,         {1, 1, 4, 1, 1},        {0, 0, 11, 11, 10, 0}},

        // 64-Bit Pixel Formats
        { PixelFormat::RG32UInt,                "RG32UInt",                 PixelFormatType::UInt,          64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
        { PixelFormat::RG32SInt,                "RG32SInt",                 PixelFormatType::SInt,          64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
        { PixelFormat::RG32Float,               "RG32Float",                PixelFormatType::Float,         64,         {1, 1, 8, 1, 1},        {0, 0, 32, 32, 0, 0}},
        { PixelFormat::RGBA16UNorm,             "RGBA16UNorm",              PixelFormatType::UNorm,         64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
        { PixelFormat::RGBA16SNorm,             "RGBA16SNorm",              PixelFormatType::SNorm,         64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
        { PixelFormat::RGBA16UInt,              "RGBA16UInt",               PixelFormatType::UInt,          64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
        { PixelFormat::RGBA16SInt,              "RGBA16SInt",               PixelFormatType::SInt,          64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},
        { PixelFormat::RGBA16Float,             "Rgba16Float",              PixelFormatType::Float,         64,         {1, 1, 8, 1, 1},        {0, 0, 16, 16, 16, 16}},

        // 128-Bit Pixel Formats
        { PixelFormat::RGBA32UInt,              "RGBA32UInt",               PixelFormatType::UInt,          128,        {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},
        { PixelFormat::RGBA32SInt,              "RGBA32SInt",               PixelFormatType::SInt,          128,        {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},
        { PixelFormat::RGBA32Float,             "RGBA32Float",              PixelFormatType::Float,         128,        {1, 1, 16, 1, 1},       {0, 0, 32, 32, 32, 32}},

        // Depth-stencil
        { PixelFormat::Depth32Float,            "Depth32Float",             PixelFormatType::Float,         32,         {1, 1, 4, 1, 1},        {32, 0, 0, 0, 0, 0}},
        { PixelFormat::Depth16UNorm,            "Depth16UNorm",             PixelFormatType::UNorm,         16,         {1, 1, 2, 1, 1},        {16, 0, 0, 0, 0, 0}},
        { PixelFormat::Depth24PlusStencil8,     "Depth24PlusStencil8",      PixelFormatType::UNorm,         32,         {1, 1, 4, 1, 1},        {24, 8, 0, 0, 0, 0}},

        // Compressed formats
        { PixelFormat::BC1RGBAUNorm,            "BC1RGBAUNorm",             PixelFormatType::UNorm,         4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
        { PixelFormat::BC1RGBAUNormSrgb,        "BC1RGBAUNormSrgb",         PixelFormatType::UnormSrgb,     4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
        { PixelFormat::BC2RGBAUNorm,            "BC2RGBAUNorm",             PixelFormatType::UNorm,         8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
        { PixelFormat::BC2RGBAUNormSrgb,        "BC2RGBAUNormSrgb",         PixelFormatType::UnormSrgb,     8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
        { PixelFormat::BC3RGBAUNorm,            "BC3RGBAUNorm",             PixelFormatType::UNorm,         8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
        { PixelFormat::BC3RGBAUNormSrgb,        "BC3RGBAUNormSrgb",         PixelFormatType::UnormSrgb,     8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
        { PixelFormat::BC4RUNorm,               "BC4RUNorm",                PixelFormatType::UNorm,         4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
        { PixelFormat::BC4RSNorm,               "BC4RSNorm",                PixelFormatType::SNorm,         4,          {4, 4, 8, 1, 1},        {0, 0, 0, 0, 0, 0}},
        { PixelFormat::BC5RGUNorm,              "BC5RGUNorm",               PixelFormatType::UNorm,         8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
        { PixelFormat::BC5RGSNorm,              "BC5RGSNorm",               PixelFormatType::SNorm,         8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
        { PixelFormat::BC6HRGBUFloat,           "BC6HRGBUFloat",            PixelFormatType::Float,         8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
        { PixelFormat::BC6HRGBSFloat,           "BC6HRGBSFloat",            PixelFormatType::Float,         8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
        { PixelFormat::BC7RGBAUNorm,            "BC7RGBAUNorm",             PixelFormatType::UNorm,         8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
        { PixelFormat::BC7RGBAUNormSrgb,        "BC7RGBAUNormSrgb",         PixelFormatType::UnormSrgb,     8,          {4, 4, 16, 1, 1},       {0, 0, 0, 0, 0, 0}},
    };
}
