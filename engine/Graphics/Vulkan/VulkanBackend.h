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

#include "Graphics/Graphics.h"
#include <array>
#include <vk_mem_alloc.h>
#include <volk.h>

#define VK_DEFAULT_FENCE_TIMEOUT 100000000000

namespace alimer
{
    /*struct QueueFamilyIndices
    {
        uint32_t graphicsQueueFamilyIndex;
        uint32_t computeQueueFamily;
        uint32_t copyQueueFamily;
    };*/

    struct PhysicalDeviceExtensions
    {
        bool swapchain;
        bool depthClipEnable;
        bool maintenance_1;
        bool maintenance_2;
        bool maintenance_3;
        bool get_memory_requirements2;
        bool dedicated_allocation;
        bool bind_memory2;
        bool memory_budget;
        bool image_format_list;
        bool sampler_mirror_clamp_to_edge;
        bool win32_full_screen_exclusive;
        bool performance_query;
        bool host_query_reset;
    };

    struct VkFormatDesc
    {
        PixelFormat format;
        VkFormat vkFormat;
    };

    extern const VkFormatDesc kVkFormatDesc[];

    constexpr VkFormat VulkanImageFormat(PixelFormat format)
    {
        switch (format)
        {
            // 8-bit pixel formats
            case PixelFormat::R8Unorm:
                return VK_FORMAT_R8_UNORM;
            case PixelFormat::R8Snorm:
                return VK_FORMAT_R8_SNORM;
            case PixelFormat::R8Uint:
                return VK_FORMAT_R8_UINT;
            case PixelFormat::R8Sint:
                return VK_FORMAT_R8_SINT;
                // 16-bit pixel formats
            case PixelFormat::R16Unorm:
                return VK_FORMAT_R16_UNORM;
            case PixelFormat::R16Snorm:
                return VK_FORMAT_R16_SNORM;
            case PixelFormat::R16Uint:
                return VK_FORMAT_R16_UINT;
            case PixelFormat::R16Sint:
                return VK_FORMAT_R16_SINT;
            case PixelFormat::R16Float:
                return VK_FORMAT_R16_SFLOAT;
            case PixelFormat::RG8Unorm:
                return VK_FORMAT_R8G8_UNORM;
            case PixelFormat::RG8Snorm:
                return VK_FORMAT_R8G8_SNORM;
            case PixelFormat::RG8Uint:
                return VK_FORMAT_R8G8_UINT;
            case PixelFormat::RG8Sint:
                return VK_FORMAT_R8G8_SINT;
                // 32-bit pixel formats
            case PixelFormat::R32Uint:
                return VK_FORMAT_R32_UINT;
            case PixelFormat::R32Sint:
                return VK_FORMAT_R32_SINT;
            case PixelFormat::R32Float:
                return VK_FORMAT_R32_SFLOAT;
            case PixelFormat::RG16Unorm:
                return VK_FORMAT_R16G16_UNORM;
            case PixelFormat::RG16Snorm:
                return VK_FORMAT_R16G16_SNORM;
            case PixelFormat::RG16Uint:
                return VK_FORMAT_R16G16_UINT;
            case PixelFormat::RG16Sint:
                return VK_FORMAT_R16G16_SINT;
            case PixelFormat::RG16Float:
                return VK_FORMAT_R16G16_SFLOAT;
            case PixelFormat::RGBA8Unorm:
                return VK_FORMAT_R8G8B8A8_UNORM;
            case PixelFormat::RGBA8UnormSrgb:
                return VK_FORMAT_R8G8B8A8_SRGB;
            case PixelFormat::RGBA8Snorm:
                return VK_FORMAT_R8G8B8A8_SNORM;
            case PixelFormat::RGBA8Uint:
                return VK_FORMAT_R8G8B8A8_UINT;
            case PixelFormat::RGBA8Sint:
                return VK_FORMAT_R8G8B8A8_SINT;
            case PixelFormat::BGRA8Unorm:
                return VK_FORMAT_B8G8R8A8_UNORM;
            case PixelFormat::BGRA8UnormSrgb:
                return VK_FORMAT_B8G8R8A8_SRGB;
                // Packed 32-Bit formats
            case PixelFormat::RGB10A2Unorm:
                return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
            case PixelFormat::RG11B10Float:
                return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
            case PixelFormat::RGB9E5Float:
                return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
                // 64-Bit Pixel Formats
            case PixelFormat::RG32Uint:
                return VK_FORMAT_R32G32_UINT;
            case PixelFormat::RG32Sint:
                return VK_FORMAT_R32G32_SINT;
            case PixelFormat::RG32Float:
                return VK_FORMAT_R32G32_SFLOAT;
            case PixelFormat::RGBA16Unorm:
                return VK_FORMAT_R16G16B16A16_UNORM;
            case PixelFormat::RGBA16Snorm:
                return VK_FORMAT_R16G16B16A16_SNORM;
            case PixelFormat::RGBA16Uint:
                return VK_FORMAT_R16G16B16A16_UINT;
            case PixelFormat::RGBA16Sint:
                return VK_FORMAT_R16G16B16A16_SINT;
            case PixelFormat::RGBA16Float:
                return VK_FORMAT_R16G16B16A16_SFLOAT;
                // 128-Bit Pixel Formats
            case PixelFormat::RGBA32Uint:
                return VK_FORMAT_R32G32B32A32_UINT;
            case PixelFormat::RGBA32Sint:
                return VK_FORMAT_R32G32B32A32_SINT;
            case PixelFormat::RGBA32Float:
                return VK_FORMAT_R32G32B32A32_SFLOAT;
                // Depth-stencil formats
            case PixelFormat::Depth16Unorm:
                return VK_FORMAT_D16_UNORM;
            case PixelFormat::Depth32Float:
                return VK_FORMAT_D32_SFLOAT;
            case PixelFormat::Depth24UnormStencil8:
                return VK_FORMAT_D24_UNORM_S8_UINT; // Or VK_FORMAT_D32_SFLOAT_S8_UINT?
                // Compressed BC formats
            case PixelFormat::BC1RGBAUnorm:
                return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
            case PixelFormat::BC1RGBAUnormSrgb:
                return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
            case PixelFormat::BC2RGBAUnorm:
                return VK_FORMAT_BC2_UNORM_BLOCK;
            case PixelFormat::BC2RGBAUnormSrgb:
                return VK_FORMAT_BC2_SRGB_BLOCK;
            case PixelFormat::BC3RGBAUnorm:
                return VK_FORMAT_BC3_UNORM_BLOCK;
            case PixelFormat::BC3RGBAUnormSrgb:
                return VK_FORMAT_BC3_SRGB_BLOCK;
            case PixelFormat::BC4RSnorm:
                return VK_FORMAT_BC4_SNORM_BLOCK;
            case PixelFormat::BC4RUnorm:
                return VK_FORMAT_BC4_UNORM_BLOCK;
            case PixelFormat::BC5RGSnorm:
                return VK_FORMAT_BC5_SNORM_BLOCK;
            case PixelFormat::BC5RGUnorm:
                return VK_FORMAT_BC5_UNORM_BLOCK;
            case PixelFormat::BC6HRGBFloat:
                return VK_FORMAT_BC6H_SFLOAT_BLOCK;
            case PixelFormat::BC6HRGBUfloat:
                return VK_FORMAT_BC6H_UFLOAT_BLOCK;
            case PixelFormat::BC7RGBAUnorm:
                return VK_FORMAT_BC7_UNORM_BLOCK;
            case PixelFormat::BC7RGBAUnormSrgb:
                return VK_FORMAT_BC7_SRGB_BLOCK;

            default:
            case PixelFormat::Undefined:
                ALIMER_UNREACHABLE();
                return VK_FORMAT_UNDEFINED;
        }
    }

    const char* ToString(VkResult result);
}

/// Helper macro to test the result of Vulkan calls which can return an error.
#define VK_CHECK(x)                                                   \
    do                                                                \
    {                                                                 \
        VkResult err = x;                                             \
        if (err)                                                      \
        {                                                             \
            LOGE("Detected Vulkan error: {}", alimer::ToString(err)); \
        }                                                             \
    } while (0)

#define VK_LOG_ERROR(result, message) LOGE("{} - Vulkan error: {}", message, alimer::ToString(result));