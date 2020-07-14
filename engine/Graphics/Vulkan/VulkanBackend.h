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
#include "core/Log.h"
#include "graphics/Types.h"

#include <vk_mem_alloc.h>
#include <volk.h>

namespace alimer
{
    class VulkanGraphicsImpl;

    const char* ToString(VkResult result);

    struct QueueFamilyIndices {
        uint32_t graphicsQueueFamilyIndex;
        uint32_t computeQueueFamily;
        uint32_t copyQueueFamily;
    };

    struct InstanceExtensions {
        bool debugUtils = false;
        bool headless = false;
        bool getSurfaceCapabilities2 = false;
    };

    struct PhysicalDeviceExtensions {
        bool swapchain;
        bool depth_clip_enable;
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
        bool raytracing;
        bool buffer_device_address;
        bool deferred_host_operations;
        bool descriptor_indexing;
        bool pipeline_library;
        bool multiview;
    };
}

/// Helper macro to test the result of Vulkan calls which can return an error.
#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			LOGE("Detected Vulkan error: {}", alimer::ToString(err)); \
		}                                                           \
	} while (0)

#define VK_LOG_ERROR(result, message) LOGE("{} - Vulkan error: {}", message, alimer::ToString(result));
