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

#include "Core/Log.h"
#include "Graphics/GraphicsResource.h"
#include "volk.h"
#include <string>
#include <vector>

namespace alimer
{
    const std::string to_string(VkResult result);
    const std::string to_string(VkCompositeAlphaFlagBitsKHR composite_alpha);

    struct VkFormatDesc
    {
        PixelFormat format;
        VkFormat vkFormat;
    };

    extern const VkFormatDesc kVkFormatDesc[];

    static inline VkFormat ToVkFormat(PixelFormat format)
    {
        ALIMER_ASSERT(kVkFormatDesc[(uint32_t) format].format == format);
        return kVkFormatDesc[(uint32_t) format].vkFormat;
    }

    class VulkanSemaphorePool
    {
    public:
        VulkanSemaphorePool(VkDevice device);
        ~VulkanSemaphorePool();
        VulkanSemaphorePool(const VulkanSemaphorePool&) = delete;
        VulkanSemaphorePool(VulkanSemaphorePool&& other) = delete;
        VulkanSemaphorePool& operator=(const VulkanSemaphorePool&) = delete;
        VulkanSemaphorePool& operator=(VulkanSemaphorePool&&) = delete;

        void Reset();
        VkSemaphore RequestSemaphore();

    private:
        VkDevice device;

        std::vector<VkSemaphore> semaphores;
        uint32_t active_semaphore_count{0};
    };

    class VulkanFencePool
    {
    public:
        VulkanFencePool(VkDevice device);
        ~VulkanFencePool();

        VulkanFencePool(const VulkanFencePool&) = delete;
        VulkanFencePool(VulkanFencePool&& other) = delete;
        VulkanFencePool& operator=(const VulkanFencePool&) = delete;
        VulkanFencePool& operator=(VulkanFencePool&&) = delete;

        VkResult Reset();
        VkFence RequestFence();
        VkResult Wait(uint32_t timeout = std::numeric_limits<uint32_t>::max()) const;

    private:
        VkDevice device;
        uint32_t activeFenceCount;
        std::vector<VkFence> fences;
    };

    class VulkanCommandPool
    {
    public:
        VulkanCommandPool(VkDevice device, uint32_t queueFamilyIndex);
        ~VulkanCommandPool();

        VulkanCommandPool(const VulkanCommandPool&) = delete;
        VulkanCommandPool(VulkanCommandPool&& other) = delete;
        VulkanCommandPool& operator=(const VulkanCommandPool&) = delete;
        VulkanCommandPool& operator=(VulkanCommandPool&&) = delete;

        void Reset();

    private:
        VkDevice device;
        uint32_t queueFamilyIndex;
        VkCommandPool handle;
    };

    class VulkanGraphics;
    class VulkanRenderFrame
    {
    public:
        VulkanRenderFrame(VulkanGraphics& device);
        ~VulkanRenderFrame();

        void Reset();
        VkFence RequestFence();
        VkSemaphore RequestSemaphore();

    private:
        VulkanGraphics& device;
        VulkanFencePool fencePool;
        VulkanSemaphorePool semaphorePool;
        VulkanCommandPool commandPool;
    };
}

/// @brief Helper macro to test the result of Vulkan calls which can return an error.
#define VK_CHECK(x)                                                    \
    do                                                                 \
    {                                                                  \
        VkResult err = x;                                              \
        if (err)                                                       \
        {                                                              \
            LOGE("Detected Vulkan error: {}", alimer::to_string(err)); \
            abort();                                                   \
        }                                                              \
    } while (0)

#define VK_THROW(result, msg)                                         \
    do                                                                \
    {                                                                 \
        LOGE("Vulkan: {}, error {}", msg, alimer::to_string(result)); \
        abort();                                                      \
    } while (0)