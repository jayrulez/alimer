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

#include <vk_mem_alloc.h>
#include <volk.h>
#include <EASTL/string.h>

namespace alimer
{
    class VulkanGPUDevice;

    struct VulkanDeviceFeatures
    {
        uint32_t apiVersion;
        bool headless = false;

        /// VK_KHR_get_surface_capabilities2
        bool surface_capabilities2 = false;
        /// VK_KHR_get_physical_device_properties2
        bool physical_device_properties2 = false;
        /// VK_KHR_external_memory_capabilities + VK_KHR_external_semaphore_capabilities
        bool external = false;
        /// VK_EXT_debug_utils
        bool debug_utils = false;

        /// Device - VK_KHR_get_memory_requirements2
        bool get_memory_requirements2 = false;

        /// Device - VK_KHR_dedicated_allocation
        bool dedicated = false;

        /// Device - VK_KHR_bind_memory2
        bool bind_memory2 = false;

        /// Device - VK_EXT_memory_budget
        bool memory_budget = false;
    };

    eastl::string to_string(VkResult result);
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
