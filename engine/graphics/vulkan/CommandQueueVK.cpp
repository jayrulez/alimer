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

#if TODO_VK
#include "CommandQueueVK.h"
#include "CommandPoolVK.h"
#include "CommandBufferVK.h"
#include "GraphicsDeviceVK.h"
#include "core/Assert.h"
#include "core/Log.h"

namespace alimer
{

    CommandQueueVK::CommandQueueVK(GraphicsDeviceVK* device_, CommandQueueType type_)
        : device(device_)
        , type(type_)
    {

    }

    CommandQueueVK::~CommandQueueVK()
    {
        Destroy();
    }

    bool CommandQueueVK::Init(const char* name, uint32_t queueFamilyIndex_, uint32_t index)
    {
        queueFamilyIndex = queueFamilyIndex_;
        vkGetDeviceQueue(device->GetHandle(), queueFamilyIndex, index, &handle);
        if (name)
        {
            device->SetObjectName(VK_OBJECT_TYPE_QUEUE, (uint64_t)handle, name);
        }

        return true;
    }

    void CommandQueueVK::Destroy()
    {
    }

    bool CommandQueueVK::SupportPresent(VkSurfaceKHR surface)
    {
        VkBool32 presentSupport;
        vkGetPhysicalDeviceSurfaceSupportKHR(device->GetPhysicalDevice(), queueFamilyIndex, surface, &presentSupport);
        return presentSupport == VK_TRUE;
    }

    ICommandBuffer& CommandQueueVK::RequestCommandBuffer()
    {
        return device->RequestCommandBuffer(type);
    }

    void CommandQueueVK::Submit(const ICommandBuffer& commandBuffer)
    {
        const CommandBufferVK* vkbb = static_cast<const CommandBufferVK*>(&commandBuffer);
        vkbb->End();
        VkCommandBuffer vkCommandBuffer = static_cast<const CommandBufferVK*>(&commandBuffer)->GetHandle();

        VkSemaphore signalSemaphore = device->RequestSemaphore();

        VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = (uint32_t)waitSemaphores.size();
        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStages.data();
        submitInfo.commandBufferCount = 1u;
        submitInfo.pCommandBuffers = &vkCommandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &signalSemaphore;

        VkFence fence = device->RequestFence();
        VkResult result = vkQueueSubmit(handle, 1, &submitInfo, fence);
        if (result != VK_SUCCESS)
        {
            VK_THROW(result, "Submit failed");
            return;
        }

        signalSemaphores.push_back(signalSemaphore);

        waitSemaphores.clear();
        waitStages.clear();
    }

    void CommandQueueVK::Present(VkSwapchainKHR swapChain, uint32_t imageIndex)
    {
        VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapChain;
        presentInfo.waitSemaphoreCount = (uint32_t)signalSemaphores.size();
        presentInfo.pWaitSemaphores = signalSemaphores.data();
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;

        VkResult result = vkQueuePresentKHR(handle, &presentInfo);
        if (result == VK_SUBOPTIMAL_KHR
            || result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            //handle_surface_changes();
        }

        signalSemaphores.clear();
    }

    void CommandQueueVK::AddWaitSemaphore(VkSemaphore semaphore, VkPipelineStageFlags waitStage)
    {
        waitSemaphores.push_back(semaphore);
        waitStages.push_back(waitStage);
    }
}

#endif // TODO_VK
