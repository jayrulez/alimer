//
// Copyright (c) 2020 Amer Koleci and contributors.
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

#include "VulkanCommandPool.h"
#include "VulkanGraphicsDevice.h"

namespace Alimer
{
    VulkanCommandPool::VulkanCommandPool(VulkanGraphicsDevice& device, uint32_t queueFamilyIndex)
        : device{ device }
        , queueFamilyIndex{ queueFamilyIndex }
    {
        VkCommandPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        createInfo.queueFamilyIndex = queueFamilyIndex;
        createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        VkResult result = vkCreateCommandPool(device.GetHandle(), &createInfo, nullptr, &handle);

        if (result != VK_SUCCESS)
        {
            VK_LOG_ERROR(result, "Failed to create command pool");
        }
    }

    VulkanCommandPool::~VulkanCommandPool()
    {
        // Destroy command pool
        if (handle != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(device.GetHandle(), handle, nullptr);
        }
    }

    void VulkanCommandPool::Reset()
    {
        VK_CHECK(vkResetCommandPool(device.GetHandle(), handle, 0));

        primaryCommandBufferCount = 0;
    }

    VulkanCommandBuffer* VulkanCommandPool::RequestCommandBuffer(VkCommandBufferLevel level)
    {
        if (level == VK_COMMAND_BUFFER_LEVEL_PRIMARY)
        {
            if (primaryCommandBufferCount < primaryCommandBuffers.size())
            {
                return primaryCommandBuffers.at(primaryCommandBufferCount++).get();
            }

            primaryCommandBuffers.emplace_back(eastl::make_unique<VulkanCommandBuffer>(*this, level));
            primaryCommandBufferCount++;
            return primaryCommandBuffers.back().get();
        }

        // TODO:
        VulkanCommandBuffer* secondary = nullptr;
        return secondary;
    }
}
