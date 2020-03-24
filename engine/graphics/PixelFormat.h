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
#include <agpu.h>
#include <string>

namespace alimer
{
    /// Defines pixel format.
    enum class PixelFormat : uint32_t
    {
        Undefined = 0,
        // 8-bit pixel formats
        R8UNorm,
        R8SNorm,
        R8UInt,
        R8SInt,

        // 16-bit pixel formats
        R16UNorm,
        R16SNorm,
        R16UInt,
        R16SInt,
        R16Float,
        RG8UNorm,
        RG8SNorm,
        RG8UInt,
        RG8SInt,

        // 32-bit pixel formats
        R32UInt,
        R32SInt,
        R32Float,
        RG16UNorm,
        RG16SNorm,
        RG16UInt,
        RG16SInt,
        RG16Float,
        RGBA8UNorm,
        RGBA8UNormSrgb,
        RGBA8SNorm,
        RGBA8UInt,
        RGBA8SInt,
        BGRA8UNorm,
        BGRA8UNormSrgb,

        // Packed 32-Bit Pixel formats
        RGB10A2UNorm,
        RG11B10Float,

        // 64-Bit Pixel Formats
        RG32UInt,
        RG32SInt,
        RG32Float,
        RGBA16UNorm,
        RGBA16SNorm,
        RGBA16UInt,
        RGBA16SInt,
        RGBA16Float,

        // 128-Bit Pixel Formats
        RGBA32UInt,
        RGBA32SInt,
        RGBA32Float,

        // Depth-stencil formats
        Depth16UNorm,
        Depth32Float,
        Depth24UNormStencil8,
        Depth32FloatStencil8,

        // Compressed BC formats
        BC1RGBAUNorm,
        BC1RGBAUNormSrgb,
        BC2RGBAUNorm,
        BC2RGBAUNormSrgb,
        BC3RGBAUNorm,
        BC3RGBAUNormSrgb,
        BC4RUNorm,
        BC4RSNorm,
        BC5RGUNorm,
        BC5RGSNorm,
        BC6HRGBUFloat,
        BC6HRGBSFloat,
        BC7RGBAUNorm,
        BC7RGBAUNormSrgb,

        // Compressed PVRTC Pixel Formats
        PVRTC_RGB2,
        PVRTC_RGBA2,
        PVRTC_RGB4,
        PVRTC_RGBA4,

        // Compressed ETC Pixel Formats
        ETC2_RGB8,
        ETC2_RGB8Srgb,
        ETC2_RGB8A1,
        ETC2_RGB8A1Srgb,

        // Compressed ASTC Pixel Formats
        ASTC4x4,
        ASTC5x5,
        ASTC6x6,
        ASTC8x5,
        ASTC8x6,
        ASTC8x8,
        ASTC10x10,
        ASTC12x12,

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
        UNormSrgb,
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

    ALIMER_API extern const PixelFormatDesc FormatDesc[];

    /// Get the number of bytes per format.
    inline uint32_t GetFormatBitsPerPixel(PixelFormat format)
    {
        ALIMER_ASSERT(FormatDesc[(uint32_t)format].format == format);
        return FormatDesc[(uint32_t)format].bitsPerPixel;
    }

    inline uint32_t GetFormatBlockSize(PixelFormat format)
    {
        ALIMER_ASSERT(FormatDesc[(uint32_t)format].format == format);
        return FormatDesc[(uint32_t)format].compression.blockSize;
    }

    /// Check if the format has a depth component
    inline bool IsDepthFormat(PixelFormat format)
    {
        ALIMER_ASSERT(FormatDesc[(uint32_t)format].format == format);
        return FormatDesc[(uint32_t)format].bits.depth > 0;
    }

    /// Check if the format has a stencil component
    inline bool IsStencilFormat(PixelFormat format)
    {
        ALIMER_ASSERT(FormatDesc[(uint32_t)format].format == format);
        return FormatDesc[(uint32_t)format].bits.stencil > 0;
    }

    /// Check if the format has depth or stencil components
    inline bool IsDepthStencilFormat(PixelFormat format)
    {
        return IsDepthFormat(format) || IsStencilFormat(format);
    }

    /// Check if the format is a compressed format
    inline bool IsCompressedFormat(PixelFormat format)
    {
        ALIMER_ASSERT(FormatDesc[(uint32_t)format].format == format);
        return format >= PixelFormat::BC1RGBAUNorm && format <= PixelFormat::PVRTC_RGBA4;
    }

    /// Get the format compression ration along the x-axis
    inline uint32_t GetFormatBlockWidth(PixelFormat format)
    {
        ALIMER_ASSERT(FormatDesc[(uint32_t)format].format == format);
        return FormatDesc[(uint32_t)format].compression.blockWidth;
    }

    /// Get the format compression ration along the y-axis
    inline uint32_t GetFormatBlockHeight(PixelFormat format)
    {
        ALIMER_ASSERT(FormatDesc[(uint32_t)format].format == format);
        return FormatDesc[(uint32_t)format].compression.blockHeight;
    }

    /// Get the format Type
    inline PixelFormatType GetFormatType(PixelFormat format)
    {
        ALIMER_ASSERT(FormatDesc[(uint32_t)format].format == format);
        return FormatDesc[(uint32_t)format].type;
    }

    inline const std::string& to_string(PixelFormat format)
    {
        ALIMER_ASSERT(FormatDesc[(uint32_t)format].format == format);
        return FormatDesc[(uint32_t)format].name;
    }
} 
