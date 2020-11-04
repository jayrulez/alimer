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

#pragma once

#include "Core/Assert.h"
#include <string>

namespace alimer
{
    /// Defines pixel format.
    enum class PixelFormat : uint32_t
    {
        Undefined = 0,
        // 8-bit formats
        R8Unorm,
        R8Snorm,
        R8Uint,
        R8Sint,
        // 16-bit formats
        R16Unorm,
        R16Snorm,
        R16Uint,
        R16Sint,
        R16Float,
        RG8Unorm,
        RG8Snorm,
        RG8Uint,
        RG8Sint,
        // 32-bit formats
        R32Uint,
        R32Sint,
        R32Float,
        RG16Unorm,
        RG16Snorm,
        RG16Uint,
        RG16Sint,
        RG16Float,
        RGBA8Unorm,
        RGBA8UnormSrgb,
        RGBA8Snorm,
        RGBA8Uint,
        RGBA8Sint,
        BGRA8Unorm,
        BGRA8UnormSrgb,
        // Packed 32-Bit formats
        RGB10A2Unorm,
        RG11B10Float,
        RGB9E5Float,
        // 64-Bit formats
        RG32Uint,
        RG32Sint,
        RG32Float,
        RGBA16Unorm,
        RGBA16Snorm,
        RGBA16Uint,
        RGBA16Sint,
        RGBA16Float,
        // 128-Bit formats
        RGBA32Uint,
        RGBA32Sint,
        RGBA32Float,
        // Depth-stencil formats
        Depth16Unorm,
        Depth32Float,
        Depth24UnormStencil8,
        Depth32FloatStencil8,
        // Compressed BC formats
        BC1RGBAUnorm,
        BC1RGBAUnormSrgb,
        BC2RGBAUnorm,
        BC2RGBAUnormSrgb,
        BC3RGBAUnorm,
        BC3RGBAUnormSrgb,
        BC4RUnorm,
        BC4RSnorm,
        BC5RGUnorm,
        BC5RGSnorm,
        BC6HRGBUfloat,
        BC6HRGBFloat,
        BC7RGBAUnorm,
        BC7RGBAUnormSrgb,
        Count
    };

    /// Pixel format Type
    enum class PixelFormatType
    {
        /// Unknown format Type.
        Unknown = 0,
        /// Floating-point formats.
        Float,
        /// Unsigned normalized formats.
        UNorm,
        /// Unsigned normalized SRGB formats
        UnormSrgb,
        /// Signed normalized formats.
        SNorm,
        /// Unsigned integer formats.
        UInt,
        /// Signed integer formats.
        SInt
    };

    struct PixelFormatDesc
    {
        PixelFormat format;
        const std::string name;
        PixelFormatType type;
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
    };

    ALIMER_API extern const PixelFormatDesc kFormatDesc[];

    /// Get the number of bits per format.
    constexpr uint32_t GetFormatBitsPerPixel(PixelFormat format)
    {
        ALIMER_ASSERT(kFormatDesc[(uint32_t) format].format == format);
        return kFormatDesc[(uint32_t) format].bitsPerPixel;
    }

    constexpr uint32_t GetFormatBlockSize(PixelFormat format)
    {
        ALIMER_ASSERT(kFormatDesc[(uint32_t) format].format == format);
        return kFormatDesc[(uint32_t) format].compression.blockSize;
    }

    /// Check if the format has a depth component
    constexpr bool IsDepthFormat(PixelFormat format)
    {
        ALIMER_ASSERT(kFormatDesc[(uint32_t) format].format == format);
        return kFormatDesc[(uint32_t) format].bits.depth > 0;
    }

    /// Check if the format has a stencil component
    constexpr bool IsStencilFormat(PixelFormat format)
    {
        ALIMER_ASSERT(kFormatDesc[(uint32_t) format].format == format);
        return kFormatDesc[(uint32_t) format].bits.stencil > 0;
    }

    /// Check if the format has depth or stencil components
    constexpr bool IsDepthStencilFormat(PixelFormat format)
    {
        return IsDepthFormat(format) || IsStencilFormat(format);
    }

    /// Check if the format is a compressed format
    constexpr bool IsBlockCompressedFormat(PixelFormat format)
    {
        ALIMER_ASSERT(kFormatDesc[(uint32_t) format].format == format);
        switch (format)
        {
            case PixelFormat::BC1RGBAUnorm:
            case PixelFormat::BC1RGBAUnormSrgb:
            case PixelFormat::BC2RGBAUnorm:
            case PixelFormat::BC2RGBAUnormSrgb:
            case PixelFormat::BC3RGBAUnorm:
            case PixelFormat::BC3RGBAUnormSrgb:
            case PixelFormat::BC4RUnorm:
            case PixelFormat::BC4RSnorm:
            case PixelFormat::BC5RGUnorm:
            case PixelFormat::BC5RGSnorm:
            case PixelFormat::BC6HRGBUfloat:
            case PixelFormat::BC6HRGBFloat:
            case PixelFormat::BC7RGBAUnorm:
            case PixelFormat::BC7RGBAUnormSrgb:
                return true;
        }

        return false;
    }

    /// Get the format compression ration along the x-axis
    constexpr uint32_t GetFormatBlockWidth(PixelFormat format)
    {
        ALIMER_ASSERT(kFormatDesc[(uint32_t) format].format == format);
        return kFormatDesc[(uint32_t) format].compression.blockWidth;
    }

    /// Get the format compression ration along the y-axis
    constexpr uint32_t GetFormatBlockHeight(PixelFormat format)
    {
        ALIMER_ASSERT(kFormatDesc[(uint32_t) format].format == format);
        return kFormatDesc[(uint32_t) format].compression.blockHeight;
    }

    /// Get the format Type
    constexpr PixelFormatType GetFormatType(PixelFormat format)
    {
        ALIMER_ASSERT(kFormatDesc[(uint32_t) format].format == format);
        return kFormatDesc[(uint32_t) format].type;
    }

    constexpr const std::string& ToString(PixelFormat format)
    {
        ALIMER_ASSERT(kFormatDesc[(uint32_t) format].format == format);
        return kFormatDesc[(uint32_t) format].name;
    }

    /// Check if a format represents sRGB color space.
    constexpr bool IsSrgbFormat(PixelFormat format)
    {
        return (GetFormatType(format) == PixelFormatType::UnormSrgb);
    }

    /// Convert a SRGB format to linear. If the format is already linear no conversion will be made.
    constexpr PixelFormat SRGBToLinearFormat(PixelFormat format)
    {
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
                ALIMER_ASSERT(IsSrgbFormat(format) == false);
                return format;
        }
    }

    /// Convert an linear format to sRGB. If the format doesn't have a matching sRGB format no conversion will be made.
    constexpr PixelFormat LinearToSRGBFormat(PixelFormat format)
    {
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
