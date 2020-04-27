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

#include "core/Assert.h"
#include "gpu/gpu.h"
#include <string>

namespace alimer
{
    /// Defines pixel format.
    enum class PixelFormat : uint32_t
    {
        Unknown = 0,
        // 8-bit pixel formats
        R8Unorm,
        R8Snorm,
        R8Uint,
        R8Sint,
        // 16-bit pixel formats
        R16Unorm,
        R16Snorm,
        R16Uint,
        R16Sint,
        R16Float,
        Rg8Unorm,
        Rg8Snorm,
        Rg8Uint,
        Rg8Sint,
        // 32-bit pixel formats
        R32Uint,
        R32Sint,
        R32Float,
        Rg16Unorm,
        Rg16Snorm,
        Rg16Uint,
        Rg16Sint,
        Rg16Float,
        Rgba8Unorm,
        Rgba8UnormSrgb,
        Rgba8Snorm,
        Rgba8Uint,
        Rgba8Sint,
        Bgra8Unorm,
        Bgra8UnormSrgb,
        // Packed 32-Bit Pixel formats
        Rgb10a2Unorm,
        Rg11b10Float,
        // 64-Bit Pixel Formats
        RG32Uint,
        RG32Sint,
        RG32Float,
        Rgba16Unorm,
        Rgba16Snorm,
        Rgba16Uint,
        Rgba16Sint,
        Rgba16Float,
        // 128-Bit Pixel Formats
        Rgba32Uint,
        Rgba32Sint,
        Rgba32Float,
        // Depth-stencil formats
        Depth32Float,
        Depth16Unorm,
        Depth24Plus,
        Depth24PlusStencil8,

        // Compressed BC formats
        Bc1RgbaUnorm,
        Bc1RgbaUnormSrgb,
        Bc2RgbaUnorm,
        Bc2RgbaUnormSrgb,
        Bc3RgbaUnorm,
        Bc3RgbaUnormSrgb,
        Bc4RUnorm,
        Bc4RSnorm,
        Bc5RgUnorm,
        Bc5RgSnorm,
        Bc6HRgbUfloat,
        Bc6HRgbSfloat,
        Bc7RgbaUnorm,
        Bc7RgbaUnormSrgb,

        Count
    };

    /**
    * Pixel format Type
    */
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
        uint8_t                 bitsPerPixel;
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
    };

    ALIMER_API extern const PixelFormatDesc kFormatDesc[];

    /// Get the number of bytes per format.
    inline uint32_t GetFormatBitsPerPixel(PixelFormat format)
    {
        ALIMER_ASSERT(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].bitsPerPixel;
    }

    inline uint32_t GetFormatBlockSize(PixelFormat format)
    {
        ALIMER_ASSERT(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].compression.blockSize;
    }

    /// Check if the format has a depth component
    inline bool IsDepthFormat(PixelFormat format)
    {
        ALIMER_ASSERT(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].bits.depth > 0;
    }

    /// Check if the format has a stencil component
    inline bool IsStencilFormat(PixelFormat format)
    {
        ALIMER_ASSERT(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].bits.stencil > 0;
    }

    /// Check if the format has depth or stencil components
    inline bool IsDepthStencilFormat(PixelFormat format)
    {
        return IsDepthFormat(format) || IsStencilFormat(format);
    }

    /// Check if the format is a compressed format
    inline bool IsCompressedFormat(PixelFormat format)
    {
        ALIMER_ASSERT(kFormatDesc[(uint32_t)format].format == format);
        return format >= PixelFormat::Bc1RgbaUnorm && format <= PixelFormat::Bc7RgbaUnormSrgb;
    }

    /// Get the format compression ration along the x-axis
    inline uint32_t GetFormatBlockWidth(PixelFormat format)
    {
        ALIMER_ASSERT(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].compression.blockWidth;
    }

    /// Get the format compression ration along the y-axis
    inline uint32_t GetFormatBlockHeight(PixelFormat format)
    {
        ALIMER_ASSERT(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].compression.blockHeight;
    }

    /// Get the format Type
    inline PixelFormatType GetFormatType(PixelFormat format)
    {
        ALIMER_ASSERT(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].type;
    }

    inline const std::string& to_string(PixelFormat format)
    {
        ALIMER_ASSERT(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].name;
    }

    /**
    * Check if a format represents sRGB color space.
    */
    inline bool isSrgbFormat(PixelFormat format)
    {
        return (GetFormatType(format) == PixelFormatType::UnormSrgb);
    }

    /**
    * Convert a SRGB format to linear. If the format is already linear no conversion will be made.
    */
    inline PixelFormat srgbToLinearFormat(PixelFormat format)
    {
        switch (format)
        {
        case PixelFormat::Bc1RgbaUnormSrgb:
            return PixelFormat::Bc1RgbaUnorm;
        case PixelFormat::Bc2RgbaUnormSrgb:
            return PixelFormat::Bc2RgbaUnorm;
        case PixelFormat::Bc3RgbaUnormSrgb:
            return PixelFormat::Bc3RgbaUnorm;
        case PixelFormat::Bgra8UnormSrgb:
            return PixelFormat::Bgra8Unorm;
        case PixelFormat::Rgba8UnormSrgb:
            return PixelFormat::Rgba8Unorm;
        case PixelFormat::Bc7RgbaUnormSrgb:
            return PixelFormat::Bc7RgbaUnorm;
        default:
            ALIMER_ASSERT(isSrgbFormat(format) == false);
            return format;
        }
    }

    /**
    * Convert an linear format to sRGB. If the format doesn't have a matching sRGB format no conversion will be made.
    */
    inline PixelFormat linearToSrgbFormat(PixelFormat format)
    {
        switch (format)
        {
        case PixelFormat::Bc1RgbaUnorm:
            return PixelFormat::Bc1RgbaUnormSrgb;
        case PixelFormat::Bc2RgbaUnorm:
            return PixelFormat::Bc2RgbaUnormSrgb;
        case PixelFormat::Bc3RgbaUnorm:
            return PixelFormat::Bc3RgbaUnormSrgb;
        case PixelFormat::Bgra8Unorm:
            return PixelFormat::Bgra8UnormSrgb;
        case PixelFormat::Rgba8Unorm:
            return PixelFormat::Rgba8UnormSrgb;
        case PixelFormat::Bc7RgbaUnorm:
            return PixelFormat::Bc7RgbaUnormSrgb;
        default:
            return format;
        }
    }
} 
