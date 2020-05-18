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

#include "graphics/ICommandQueue.h"
#include "VulkanBackend.h"
#include <vector>
#include <memory>

namespace alimer
{
    class ISwapChain;
    class CommandPoolVK;

    class CommandQueueVK final : public ICommandQueue
    {
    public:
        CommandQueueVK(GraphicsDeviceVK* device_, CommandQueueType type_);
        ~CommandQueueVK() override;

        bool Init(const char* name, uint32_t queueFamilyIndex_, uint32_t index);
        void Destroy() override;
        bool SupportPresent(VkSurfaceKHR surface);
        void AddWaitSemaphore(VkSemaphore semaphore, VkPipelineStageFlags waitStage);

        ICommandBuffer& RequestCommandBuffer() override;
        void Submit(const ICommandBuffer& commandBuffer) override;
        void Present(VkSwapchainKHR swapChain, uint32_t imageIndex);

        IGraphicsDevice* GetDevice() const override;
        ALIMER_FORCEINLINE VkQueue GetHandle() const { return handle; }
        ALIMER_FORCEINLINE CommandQueueType GetType() const override { return type; }

    private:
        GraphicsDeviceVK* device;
        CommandQueueType type;
        VkQueue handle = VK_NULL_HANDLE;
        uint32_t queueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        std::vector<VkSemaphore> waitSemaphores;
        std::vector<VkPipelineStageFlags> waitStages;
        std::vector<VkSemaphore> signalSemaphores;
    };
}
