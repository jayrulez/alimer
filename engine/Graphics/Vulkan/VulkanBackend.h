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
#include "Core/Log.h"
#include "Graphics/Types.h"

#include <volk.h>
#include "vk_mem_alloc.h"
#include <EASTL/queue.h>

namespace Alimer
{
    class VulkanGraphicsDevice;

    const char* ToString(VkResult result);

    struct VulkanInstanceExtensions
    {
        bool debugUtils;
        bool headless;
        bool get_physical_device_properties2;
        bool get_surface_capabilities2;
    };

    struct QueueFamilyIndices {
        uint32_t graphicsQueueFamilyIndex;
        uint32_t computeQueueFamily;
        uint32_t copyQueueFamily;
    };

    struct PhysicalDeviceExtensions {
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

    struct VulkanResourceRelease
    {
        VkObjectType type;
        void* handle;
        VmaAllocation memory;
    };

    struct VulkanRenderFrame
    {
        VkCommandPool primaryCommandPool = VK_NULL_HANDLE;
        VkCommandBuffer primaryCommandBuffer = VK_NULL_HANDLE;

        eastl::queue<VulkanResourceRelease> deferredReleases;
    };

    struct VkFormatDesc
    {
        PixelFormat format;
        VkFormat vkFormat;
    };

    extern const VkFormatDesc kVkFormatDesc[];

    static inline VkFormat ToVkFormat(PixelFormat format)
    {
        ALIMER_ASSERT(kVkFormatDesc[(uint32_t)format].format == format);
        return kVkFormatDesc[(uint32_t)format].vkFormat;
    }

    using Hash = uint64_t;

    class Hasher
    {
    public:
        explicit Hasher(Hash h_)
            : h(h_)
        {
        }

        Hasher() = default;

        template <typename T>
        inline void data(const T* data_, size_t size)
        {
            size /= sizeof(*data_);
            for (size_t i = 0; i < size; i++)
                h = (h * 0x100000001b3ull) ^ data_[i];
        }

        inline void u32(uint32_t value)
        {
            h = (h * 0x100000001b3ull) ^ value;
        }

        inline void s32(int32_t value)
        {
            u32(uint32_t(value));
        }

        inline void f32(float value)
        {
            union
            {
                float f32;
                uint32_t u32;
            } u;
            u.f32 = value;
            u32(u.u32);
        }

        inline void u64(uint64_t value)
        {
            u32(value & 0xffffffffu);
            u32(value >> 32);
        }

        template <typename T>
        inline void pointer(T* ptr)
        {
            u64(reinterpret_cast<uintptr_t>(ptr));
        }

        inline void string(const char* str)
        {
            char c;
            u32(0xff);
            while ((c = *str++) != '\0')
                u32(uint8_t(c));
        }

        inline void string(const eastl::string& str)
        {
            u32(0xff);
            for (auto& c : str)
                u32(uint8_t(c));
        }

        inline Hash get() const
        {
            return h;
        }

    private:
        Hash h = 0xcbf29ce484222325ull;
    };
}

/// Helper macro to test the result of Vulkan calls which can return an error.
#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			LOGE("Detected Vulkan error: %s", Alimer::ToString(err)); \
		}                                                           \
	} while (0)

#define VK_LOG_ERROR(result, message) LOGE("%s - Vulkan error: %s", message, Alimer::ToString(result));
