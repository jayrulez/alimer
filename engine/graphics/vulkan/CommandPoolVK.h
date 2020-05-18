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

#pragma once

#include "graphics/ICommandBuffer.h"
#include "VulkanBackend.h"
#include <vector>
#include <memory>

namespace alimer
{
    class CommandQueueVK;
    class CommandBufferVK;

    class CommandPoolVK final
    {
    public:
        CommandPoolVK(GraphicsDeviceVK* device, CommandQueueVK* queue, uint32_t queueFamilyIndex);
        ~CommandPoolVK();

        void Reset();
        ICommandBuffer& RequestCommandBuffer(VkCommandBufferLevel level);
        GraphicsDeviceVK* GetDevice() const { return device; }
        VkCommandPool GetHandle() const { return handle; }

    private:
        GraphicsDeviceVK* device;
        CommandQueueVK* queue;
        VkCommandPool handle;

        uint32_t activePrimaryCommandBufferCount{ 0 };
        std::vector<std::unique_ptr<CommandBufferVK>> primaryCommandBuffers;
    };
}
