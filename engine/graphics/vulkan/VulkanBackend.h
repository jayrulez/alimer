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

#include "volk.h"
#include <string>

VK_DEFINE_HANDLE(VmaAllocator);
VK_DEFINE_HANDLE(VmaAllocation);

namespace alimer
{
    class GraphicsDeviceVK;

    struct PhysicalDeviceExtensions {
        bool swapchain;
        bool depth_clip_enable;
        bool maintenance_1;
        bool maintenance_2;
        bool maintenance_3;
        bool KHR_get_memory_requirements2;
        bool KHR_dedicated_allocation;
        bool KHR_bind_memory2;
        bool EXT_memory_budget;
        bool image_format_list;
        bool debug_marker;
        bool win32_full_screen_exclusive;
        bool raytracing;
        bool buffer_device_address;
        bool deferred_host_operations;
        bool descriptor_indexing;
        bool pipeline_library;
    };

    struct VulkanDeviceFeatures
    {
        /// VK_KHR_get_physical_device_properties2
        bool physicalDeviceProperties2 = false;
        /// VK_KHR_external_memory_capabilities
        bool externalMemoryCapabilities = false;
        /// VK_KHR_external_semaphore_capabilities
        bool externalSemaphoreCapabilities = false;
        /// VK_EXT_debug_utils
        bool debugUtils = false;
        /// VK_EXT_headless_surface
        bool headless = false;
        /// VK_KHR_surface
        bool surface = false;
        /// VK_KHR_get_surface_capabilities2
        bool surfaceCapabilities2 = false;

        /// VK_KHR_external_memory_capabilities + VK_KHR_external_semaphore_capabilities
        bool external = false;
    };


    struct QueueFamilyIndices
    {
        uint32_t graphicsFamily = VK_QUEUE_FAMILY_IGNORED;
        uint32_t computeFamily = VK_QUEUE_FAMILY_IGNORED;
        uint32_t transferFamily = VK_QUEUE_FAMILY_IGNORED;
        uint32_t timestampValidBits;

        bool IsComplete()
        {
            return (graphicsFamily != VK_QUEUE_FAMILY_IGNORED);
        }
    };

    enum class TextureState : uint32_t
    {
        Undefined = 0,
        General,
        RenderTarget,
        ShaderRead,
        ShaderWrite,
        Present,
    };

    static inline VkImageAspectFlags GetVkAspectMask(VkFormat format)
    {
        switch (format)
        {
        case VK_FORMAT_UNDEFINED:
            return 0;

        case VK_FORMAT_S8_UINT:
            return VK_IMAGE_ASPECT_STENCIL_BIT;

        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT;

        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_X8_D24_UNORM_PACK32:
            return VK_IMAGE_ASPECT_DEPTH_BIT;

        default:
            return VK_IMAGE_ASPECT_COLOR_BIT;
        }
    }

    std::string to_string(VkResult result);
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
}

/// Helper macro to test the result of Vulkan calls which can return an error.
#define VK_CHECK(x)                                                                 \
	do                                                                              \
	{                                                                               \
		VkResult result = x;                                                        \
		if (result != VK_SUCCESS)                                                   \
		{                                                                           \
			ALIMER_LOGE("Detected Vulkan error: %s", alimer::to_string(result));    \
			abort();                                                                \
		}                                                                           \
	} while (0)

#define VK_THROW(result, str) ALIMER_LOGE("%s : %s", str, alimer::to_string(result)); 
