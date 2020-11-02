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

#include "Graphics/PixelFormat.h"

namespace alimer
{
    const PixelFormatDesc kFormatDesc[] = {
        // format                               name,                       type                            bpp         compression bits
        {PixelFormat::Invalid, "Invalid", PixelFormatType::Unknown, 0, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}},

        // 8-bit pixel formats
        {PixelFormat::R8Unorm, "R8Unorm", PixelFormatType::UNorm, 8, {1, 1, 1, 1, 1}, {0, 0, 8, 0, 0, 0}},
        {PixelFormat::R8Snorm, "R8Snorm", PixelFormatType::SNorm, 8, {1, 1, 1, 1, 1}, {0, 0, 8, 0, 0, 0}},
        {PixelFormat::R8Uint, "R8Uint", PixelFormatType::UInt, 8, {1, 1, 1, 1, 1}, {0, 0, 8, 0, 0, 0}},
        {PixelFormat::R8Sint, "R8Sint", PixelFormatType::SInt, 8, {1, 1, 1, 1, 1}, {0, 0, 8, 0, 0, 0}},

        // 16-bit pixel formats
        {PixelFormat::R16Unorm, "R16Unorm", PixelFormatType::UNorm, 16, {1, 1, 2, 1, 1}, {0, 0, 16, 0, 0, 0}},
        {PixelFormat::R16Snorm, "R16Snorm", PixelFormatType::SNorm, 16, {1, 1, 2, 1, 1}, {0, 0, 16, 0, 0, 0}},
        {PixelFormat::R16Uint, "R16Uint", PixelFormatType::UInt, 16, {1, 1, 2, 1, 1}, {0, 0, 16, 0, 0, 0}},
        {PixelFormat::R16Sint, "R16Sint", PixelFormatType::SInt, 16, {1, 1, 2, 1, 1}, {0, 0, 16, 0, 0, 0}},
        {PixelFormat::R16Float, "R16Float", PixelFormatType::Float, 16, {1, 1, 2, 1, 1}, {0, 0, 16, 0, 0, 0}},
        {PixelFormat::RG8Unorm, "RG8Unorm", PixelFormatType::UNorm, 16, {1, 1, 2, 1, 1}, {0, 0, 8, 8, 0, 0}},
        {PixelFormat::RG8Snorm, "RG8Snorm", PixelFormatType::SNorm, 16, {1, 1, 2, 1, 1}, {0, 0, 8, 8, 0, 0}},
        {PixelFormat::RG8Uint, "RG8Uint", PixelFormatType::UInt, 16, {1, 1, 2, 1, 1}, {0, 0, 8, 8, 0, 0}},
        {PixelFormat::RG8Sint, "RG8Sint", PixelFormatType::SInt, 16, {1, 1, 2, 1, 1}, {0, 0, 8, 8, 0, 0}},

        // 32-bit pixel formats
        {PixelFormat::R32Uint, "R32Uint", PixelFormatType::UInt, 32, {1, 1, 4, 1, 1}, {0, 0, 32, 0, 0, 0}},
        {PixelFormat::R32Sint, "R32Sint", PixelFormatType::SInt, 32, {1, 1, 4, 1, 1}, {0, 0, 32, 0, 0, 0}},
        {PixelFormat::R32Float, "R32Float", PixelFormatType::Float, 32, {1, 1, 4, 1, 1}, {0, 0, 32, 0, 0, 0}},
        {PixelFormat::RG16Unorm, "RG16Unorm", PixelFormatType::UNorm, 32, {1, 1, 4, 1, 1}, {0, 0, 16, 16, 0, 0}},
        {PixelFormat::RG16Snorm, "RG16Snorm", PixelFormatType::SNorm, 32, {1, 1, 4, 1, 1}, {0, 0, 16, 16, 0, 0}},
        {PixelFormat::RG16Uint, "RG16Uint", PixelFormatType::UInt, 32, {1, 1, 4, 1, 1}, {0, 0, 16, 16, 0, 0}},
        {PixelFormat::RG16Sint, "RG16Sint", PixelFormatType::SInt, 32, {1, 1, 4, 1, 1}, {0, 0, 16, 16, 0, 0}},
        {PixelFormat::RG16Float, "RG16Float", PixelFormatType::Float, 32, {1, 1, 4, 1, 1}, {0, 0, 16, 16, 0, 0}},
        {PixelFormat::RGBA8Unorm, "RGBA8Unorm", PixelFormatType::UNorm, 32, {1, 1, 4, 1, 1}, {0, 0, 8, 8, 8, 8}},
        {PixelFormat::RGBA8UnormSrgb, "RGBA8UnormSrgb", PixelFormatType::UnormSrgb, 32, {1, 1, 4, 1, 1}, {0, 0, 8, 8, 8, 8}},
        {PixelFormat::RGBA8Snorm, "RGBA8Snorm", PixelFormatType::SNorm, 32, {1, 1, 4, 1, 1}, {0, 0, 8, 8, 8, 8}},
        {PixelFormat::RGBA8Uint, "RGBA8Uint", PixelFormatType::UInt, 32, {1, 1, 4, 1, 1}, {0, 0, 8, 8, 8, 8}},
        {PixelFormat::RGBA8Sint, "RGBA8Sint", PixelFormatType::SInt, 32, {1, 1, 4, 1, 1}, {0, 0, 8, 8, 8, 8}},
        {PixelFormat::BGRA8Unorm, "BGRA8Unorm", PixelFormatType::UNorm, 32, {1, 1, 4, 1, 1}, {0, 0, 8, 8, 8, 8}},
        {PixelFormat::BGRA8UnormSrgb, "BGRA8UnormSrgb", PixelFormatType::UnormSrgb, 32, {1, 1, 4, 1, 1}, {0, 0, 8, 8, 8, 8}},

        // Packed 32-Bit Pixel formats
        {PixelFormat::RGB10A2Unorm, "RGB10A2Unorm", PixelFormatType::UNorm, 32, {1, 1, 4, 1, 1}, {0, 0, 10, 10, 10, 2}},
        {PixelFormat::RG11B10Float, "RG11B10Float", PixelFormatType::Float, 32, {1, 1, 4, 1, 1}, {0, 0, 11, 11, 10, 0}},
        {PixelFormat::RGB9E5Float, "RGB9E5Float", PixelFormatType::Float, 32, {1, 1, 4, 1, 1}, {0, 0, 9, 9, 9, 5}},

        // 64-Bit Pixel Formats
        {PixelFormat::RG32Uint, "RG32Uint", PixelFormatType::UInt, 64, {1, 1, 8, 1, 1}, {0, 0, 32, 32, 0, 0}},
        {PixelFormat::RG32Sint, "RG32Sint", PixelFormatType::SInt, 64, {1, 1, 8, 1, 1}, {0, 0, 32, 32, 0, 0}},
        {PixelFormat::RG32Float, "RG32Float", PixelFormatType::Float, 64, {1, 1, 8, 1, 1}, {0, 0, 32, 32, 0, 0}},
        {PixelFormat::RGBA16Unorm, "RGBA16Unorm", PixelFormatType::UNorm, 64, {1, 1, 8, 1, 1}, {0, 0, 16, 16, 16, 16}},
        {PixelFormat::RGBA16Snorm, "RGBA16Snorm", PixelFormatType::SNorm, 64, {1, 1, 8, 1, 1}, {0, 0, 16, 16, 16, 16}},
        {PixelFormat::RGBA16Uint, "RGBA16Uint", PixelFormatType::UInt, 64, {1, 1, 8, 1, 1}, {0, 0, 16, 16, 16, 16}},
        {PixelFormat::RGBA16Sint, "RGBA16Sint", PixelFormatType::SInt, 64, {1, 1, 8, 1, 1}, {0, 0, 16, 16, 16, 16}},
        {PixelFormat::RGBA16Float, "Rgba16Float", PixelFormatType::Float, 64, {1, 1, 8, 1, 1}, {0, 0, 16, 16, 16, 16}},

        // 128-Bit Pixel Formats
        {PixelFormat::RGBA32Uint, "RGBA32Uint", PixelFormatType::UInt, 128, {1, 1, 16, 1, 1}, {0, 0, 32, 32, 32, 32}},
        {PixelFormat::RGBA32Sint, "RGBA32Sint", PixelFormatType::SInt, 128, {1, 1, 16, 1, 1}, {0, 0, 32, 32, 32, 32}},
        {PixelFormat::RGBA32Float, "RGBA32Float", PixelFormatType::Float, 128, {1, 1, 16, 1, 1}, {0, 0, 32, 32, 32, 32}},

        // Depth-stencil
        {PixelFormat::Depth16Unorm, "Depth16Unorm", PixelFormatType::UNorm, 16, {1, 1, 2, 1, 1}, {16, 0, 0, 0, 0, 0}},
        {PixelFormat::Depth32Float, "Depth32Float", PixelFormatType::Float, 32, {1, 1, 4, 1, 1}, {32, 0, 0, 0, 0, 0}},
        {PixelFormat::Depth24UnormStencil8, "Depth24UnormStencil8", PixelFormatType::UNorm, 32, {1, 1, 4, 1, 1}, {24, 8, 0, 0, 0, 0}},

        // Compressed formats
        {PixelFormat::BC1RGBAUnorm, "BC1RGBAUnorm", PixelFormatType::UNorm, 4, {4, 4, 8, 1, 1}, {0, 0, 0, 0, 0, 0}},
        {PixelFormat::BC1RGBAUnormSrgb, "BC1RGBAUnormSrgb", PixelFormatType::UnormSrgb, 4, {4, 4, 8, 1, 1}, {0, 0, 0, 0, 0, 0}},
        {PixelFormat::BC2RGBAUnorm, "BC2RGBAUnorm", PixelFormatType::UNorm, 8, {4, 4, 16, 1, 1}, {0, 0, 0, 0, 0, 0}},
        {PixelFormat::BC2RGBAUnormSrgb, "BC2RGBAUnormSrgb", PixelFormatType::UnormSrgb, 8, {4, 4, 16, 1, 1}, {0, 0, 0, 0, 0, 0}},
        {PixelFormat::BC3RGBAUnorm, "BC3RGBAUnorm", PixelFormatType::UNorm, 8, {4, 4, 16, 1, 1}, {0, 0, 0, 0, 0, 0}},
        {PixelFormat::BC3RGBAUnormSrgb, "BC3RGBAUnormSrgb", PixelFormatType::UnormSrgb, 8, {4, 4, 16, 1, 1}, {0, 0, 0, 0, 0, 0}},
        {PixelFormat::BC4RUnorm, "BC4RUnorm", PixelFormatType::UNorm, 4, {4, 4, 8, 1, 1}, {0, 0, 0, 0, 0, 0}},
        {PixelFormat::BC4RSnorm, "BC4RSnorm", PixelFormatType::SNorm, 4, {4, 4, 8, 1, 1}, {0, 0, 0, 0, 0, 0}},
        {PixelFormat::BC5RGUnorm, "BC5RGUnorm", PixelFormatType::UNorm, 8, {4, 4, 16, 1, 1}, {0, 0, 0, 0, 0, 0}},
        {PixelFormat::BC5RGSnorm, "BC5RGSnorm", PixelFormatType::SNorm, 8, {4, 4, 16, 1, 1}, {0, 0, 0, 0, 0, 0}},
        {PixelFormat::BC6HRGBUfloat, "BC6HRGBUfloat", PixelFormatType::Float, 8, {4, 4, 16, 1, 1}, {0, 0, 0, 0, 0, 0}},
        {PixelFormat::BC6HRGBFloat, "BC6HRGBFloat", PixelFormatType::Float, 8, {4, 4, 16, 1, 1}, {0, 0, 0, 0, 0, 0}},
        {PixelFormat::BC7RGBAUnorm, "BC7RGBAUnorm", PixelFormatType::UNorm, 8, {4, 4, 16, 1, 1}, {0, 0, 0, 0, 0, 0}},
        {PixelFormat::BC7RGBAUnormSrgb, "BC7RGBAUnormSrgb", PixelFormatType::UnormSrgb, 8, {4, 4, 16, 1, 1}, {0, 0, 0, 0, 0, 0}},
    };
}
