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

#include "CommandPoolVK.h"
#include "CommandBufferVK.h"
#include "GraphicsDeviceVK.h"
#include "core/Assert.h"
#include "core/Log.h"

namespace alimer
{
    CommandPoolVK::CommandPoolVK(GraphicsDeviceVK* device, CommandQueueVK* queue, uint32_t queueFamilyIndex)
        : device{ device }
        , queue{ queue }
    {
        VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        createInfo.queueFamilyIndex = queueFamilyIndex;
        vkCreateCommandPool(device->GetHandle(), &createInfo, nullptr, &handle);
    }

    CommandPoolVK::~CommandPoolVK()
    {
        primaryCommandBuffers.Clear();

        if (handle != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(device->GetHandle(), handle, nullptr);
        }

        handle = VK_NULL_HANDLE;
    }

    void CommandPoolVK::Reset()
    {
        VkResult result = vkResetCommandPool(device->GetHandle(), handle, 0);

        if (result != VK_SUCCESS)
        {
            VK_THROW(result, "Failed to reset command buffer");
        }

        activePrimaryCommandBufferCount = 0;
    }

    ICommandBuffer& CommandPoolVK::RequestCommandBuffer(VkCommandBufferLevel level)
    {
        if (level == VK_COMMAND_BUFFER_LEVEL_PRIMARY)
        {
            if (activePrimaryCommandBufferCount < primaryCommandBuffers.size())
            {
                auto &result = primaryCommandBuffers.At(activePrimaryCommandBufferCount++);
                result->Begin();
                return *result;
            }

            primaryCommandBuffers.EmplaceBack(std::make_unique<CommandBufferVK>(queue, this, level));
            primaryCommandBuffers.Back()->Begin();
            activePrimaryCommandBufferCount++;
            return *primaryCommandBuffers.Back();
        }
        else
        {
            ICommandBuffer* cmd = nullptr;
            return *cmd;
        }
    }
}
