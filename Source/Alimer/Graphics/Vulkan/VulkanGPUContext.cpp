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

#if TODO
#include "VulkanGPUContext.h"
#include "VulkanGPUSwapChain.h"
#include "VulkanGPUDevice.h"

namespace alimer
{
    VulkanGPUContext::VulkanGPUContext(VulkanGPUDevice* device_, const GPUContextDescription& desc, VkSurfaceKHR surface_, bool isMain_)
        : GPUContext(desc.width, desc.height, isMain_)
        , device(device_)
        , surface(surface_)
    {

    }

    VulkanGPUContext::~VulkanGPUContext()
    {
        for (auto& per_frame : frames)
        {
            TeardownFrame(per_frame);
        }
        frames.clear();
    }

    void VulkanGPUContext::TeardownFrame(VulkanRenderFrame& frame)
    {
        Purge(frame);

        if (frame.fence != VK_NULL_HANDLE)
        {
            vkDestroyFence(device->GetHandle(), frame.fence, nullptr);
            frame.fence = VK_NULL_HANDLE;
        }

        if (frame.primaryCommandBuffer != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers(device->GetHandle(), frame.primaryCommandPool, 1, &frame.primaryCommandBuffer);
            frame.primaryCommandBuffer = VK_NULL_HANDLE;
        }

        if (frame.primaryCommandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(device->GetHandle(), frame.primaryCommandPool, nullptr);
            frame.primaryCommandPool = VK_NULL_HANDLE;
        }

        if (frame.swapchainAcquireSemaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(device->GetHandle(), frame.swapchainAcquireSemaphore, nullptr);
            frame.swapchainAcquireSemaphore = VK_NULL_HANDLE;
        }

        if (frame.swapchainReleaseSemaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(device->GetHandle(), frame.swapchainReleaseSemaphore, nullptr);
            frame.swapchainReleaseSemaphore = VK_NULL_HANDLE;
        }
    }

    void VulkanGPUContext::Purge(VulkanRenderFrame& frame)
    {
        while (frame.deferredReleases.size())
        {
            const VulkanResourceRelease* ref = &frame.deferredReleases.front();

            switch (ref->type)
            {
            case VK_OBJECT_TYPE_BUFFER:
                if (ref->memory)
                    vmaDestroyBuffer(device->GetAllocator(), (VkBuffer)ref->handle, ref->memory);
                else
                    vkDestroyBuffer(device->GetHandle(), (VkBuffer)ref->handle, nullptr);
                break;

            case VK_OBJECT_TYPE_IMAGE:
                if (ref->memory)
                    vmaDestroyImage(device->GetAllocator(), (VkImage)ref->handle, ref->memory);
                else
                    vkDestroyImage(device->GetHandle(), (VkImage)ref->handle, nullptr);
                break;

            case VK_OBJECT_TYPE_DEVICE_MEMORY:
                vkFreeMemory(device->GetHandle(), (VkDeviceMemory)ref->handle, nullptr);
                break;

            case VK_OBJECT_TYPE_IMAGE_VIEW:
                vkDestroyImageView(device->GetHandle(), (VkImageView)ref->handle, nullptr);
                break;

            case VK_OBJECT_TYPE_SAMPLER:
                vkDestroySampler(device->GetHandle(), (VkSampler)ref->handle, nullptr);
                break;

            case VK_OBJECT_TYPE_RENDER_PASS:
                vkDestroyRenderPass(device->GetHandle(), (VkRenderPass)ref->handle, nullptr);
                break;

            case VK_OBJECT_TYPE_FRAMEBUFFER:
                vkDestroyFramebuffer(device->GetHandle(), (VkFramebuffer)ref->handle, nullptr);
                break;

            case VK_OBJECT_TYPE_PIPELINE:
                vkDestroyPipeline(device->GetHandle(), (VkPipeline)ref->handle, nullptr);
                break;
            default:
                break;
            }

            frame.deferredReleases.pop();
        }
    }

    bool VulkanGPUContext::BeginFrameImpl()
    {
        //if (colorTextures.empty())
        if (swapChain == nullptr)
        {
            CreateObjects();
        }
        else
        {
            // Only handle surface changes if a swapchain exists
            /*if (swapchain)
            {
                handle_surface_changes();
            }*/
        }

        VkResult result = VK_SUCCESS;
        if (swapChain)
        {
            VkSemaphore acquireSemaphore = device->RequestSemaphore();

            result = swapChain->AcquireNextImage(acquireSemaphore, &activeFrameIndex);

            if (result != VK_SUCCESS)
            {
                device->ReturnSemaphore(acquireSemaphore);
                return false;
            }

            // Recycle the old semaphore back into the semaphore manager.
            VkSemaphore oldSemaphore = frames[activeFrameIndex].swapchainAcquireSemaphore;

            if (oldSemaphore != VK_NULL_HANDLE)
            {
                device->ReturnSemaphore(oldSemaphore);
            }

            frames[activeFrameIndex].swapchainAcquireSemaphore = acquireSemaphore;

            // Handle outdated error in acquire.
            if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
            {
                //resize(context.swapchain_dimensions.width, context.swapchain_dimensions.height);
                //result = acquire_next_image(context, &index);
            }

            if (result != VK_SUCCESS)
            {
                //vkQueueWaitIdle(context.queue);
                return false;
            }
        }

        // If we have outstanding fences for this swapchain image, wait for them to complete first.
        // After begin frame returns, it is safe to reuse or delete resources which
        // were used previously.
        //
        // We wait for fences which completes N frames earlier, so we do not stall,
        // waiting for all GPU work to complete before this returns.
        // Normally, this doesn't really block at all,
        // since we're waiting for old frames to have been completed, but just in case.
        if (frames[activeFrameIndex].fence != VK_NULL_HANDLE)
        {
            VK_CHECK(vkWaitForFences(device->GetHandle(), 1, &frames[activeFrameIndex].fence, true, UINT64_MAX));
            VK_CHECK(vkResetFences(device->GetHandle(), 1, &frames[activeFrameIndex].fence));
        }

        if (frames[activeFrameIndex].primaryCommandPool != VK_NULL_HANDLE)
        {
            vkResetCommandPool(device->GetHandle(), frames[activeFrameIndex].primaryCommandPool, 0);
        }

        // Begin primary frame command buffer. We will only submit this once before it's recycled
        VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_CHECK(vkBeginCommandBuffer(frames[activeFrameIndex].primaryCommandBuffer, &beginInfo));

        return true;
    }

    void VulkanGPUContext::EndFrameImpl()
    {
        VkResult result = VK_SUCCESS;
        VkCommandBuffer commandBuffer = frames[activeFrameIndex].primaryCommandBuffer;

        TextureBarrier(swapChain->GetImage(activeFrameIndex), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        //swapChainImageLayouts[backbufferIndex] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // Complete the command buffer.
        VK_CHECK(vkEndCommandBuffer(commandBuffer));

        // Submit it to the queue with a release semaphore.
        if (frames[activeFrameIndex].swapchainReleaseSemaphore == VK_NULL_HANDLE)
        {
            VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
            VK_CHECK(vkCreateSemaphore(device->GetHandle(), &semaphoreInfo, nullptr,
                &frames[activeFrameIndex].swapchainReleaseSemaphore));
        }

        VkPipelineStageFlags waitStage{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &frames[activeFrameIndex].swapchainAcquireSemaphore;
        submitInfo.pWaitDstStageMask = &waitStage;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &frames[activeFrameIndex].swapchainReleaseSemaphore;

        // Submit command buffer to graphics queue.
        VK_CHECK(vkQueueSubmit(device->GetGraphicsQueue(), 1, &submitInfo, frames[activeFrameIndex].fence));

        // Present swapchain image
        if (swapChain)
        {
            result = swapChain->Present(frames[activeFrameIndex].swapchainReleaseSemaphore, activeFrameIndex);

            // Handle Outdated error in present.
            if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
            {
                //resize(context.swapchain_dimensions.width, context.swapchain_dimensions.height);
            }
            else if (result != VK_SUCCESS)
            {
                LOGE("Failed to present swapchain image.");
            }
        }
    }

    void VulkanGPUContext::CreateObjects()
    {
        if (surface)
        {
            swapChain = new VulkanGPUSwapChain(device, surface, verticalSync);
            frames.resize(swapChain->GetImageCount());
        }
        else
        {
            frames.resize(1);
        }

        // Create frame data.
        for (size_t i = 0, count = frames.size(); i < count; i++)
        {
            VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            VK_CHECK(vkCreateFence(device->GetHandle(), &fenceInfo, nullptr, &frames[i].fence));

            VkCommandPoolCreateInfo commandPoolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
            commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            commandPoolInfo.queueFamilyIndex = device->GetGraphicsQueueFamilyIndex();
            VK_CHECK(vkCreateCommandPool(device->GetHandle(), &commandPoolInfo, nullptr, &frames[i].primaryCommandPool));

            VkCommandBufferAllocateInfo cmdAllocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
            cmdAllocateInfo.commandPool = frames[i].primaryCommandPool;
            cmdAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdAllocateInfo.commandBufferCount = 1;
            VK_CHECK(vkAllocateCommandBuffers(device->GetHandle(), &cmdAllocateInfo, &frames[i].primaryCommandBuffer));
        }
    }

    void VulkanGPUContext::PushDebugGroup(const String& name)
    {
    }

    void VulkanGPUContext::PopDebugGroup()
    {
    }

    void VulkanGPUContext::InsertDebugMarker(const String& name)
    {
    }

    void VulkanGPUContext::BeginRenderPass(uint32_t numColorAttachments, const RenderPassColorAttachment* colorAttachments, const RenderPassDepthStencilAttachment* depthStencil)
    {
        /*VkCommandBuffer commandBuffer = frames[activeFrameIndex].primaryCommandBuffer;

        uint32_t clearValueCount = 0;
        VkClearValue clearValues[kMaxColorAttachments + 1];

        for (uint32_t i = 0; i < numColorAttachments; i++)
        {
            //TextureBarrier(commandBuffer, swapchainImages[backbufferIndex], swapChainImageLayouts[backbufferIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            //swapChainImageLayouts[backbufferIndex] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            clearValues[clearValueCount].color.float32[0] = colorAttachments[i].clearColor.r;
            clearValues[clearValueCount].color.float32[1] = colorAttachments[i].clearColor.g;
            clearValues[clearValueCount].color.float32[2] = colorAttachments[i].clearColor.b;
            clearValues[clearValueCount].color.float32[3] = colorAttachments[i].clearColor.a;
            clearValueCount++;
        }

        VkRenderPassBeginInfo beginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        beginInfo.renderPass = device->GetRenderPass(numColorAttachments, colorAttachments, depthStencil);
        beginInfo.framebuffer = device->GetFramebuffer(beginInfo.renderPass, numColorAttachments, colorAttachments, depthStencil);
        beginInfo.renderArea.offset.x = 0;
        beginInfo.renderArea.offset.y = 0;
        beginInfo.renderArea.extent.width = extent.width;
        beginInfo.renderArea.extent.height = extent.height;
        beginInfo.clearValueCount = clearValueCount;
        beginInfo.pClearValues = clearValues;
        vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);*/
    }

    void VulkanGPUContext::EndRenderPass()
    {
        /* TODO: Resolve */

        //VkCommandBuffer commandBuffer = frames[activeFrameIndex].primaryCommandBuffer;
        //vkCmdEndRenderPass(commandBuffer);
    }

    void VulkanGPUContext::SetScissorRect(const URect& scissorRect)
    {

    }

    void VulkanGPUContext::SetScissorRects(const URect* scissorRects, uint32_t count)
    {

    }

    void VulkanGPUContext::SetViewport(const Viewport& viewport)
    {

    }

    void VulkanGPUContext::SetViewports(const Viewport* viewports, uint32_t count)
    {

    }

    void VulkanGPUContext::SetBlendColor(const Color& color)
    {

    }

    void VulkanGPUContext::BindBuffer(uint32_t slot, GPUBuffer* buffer)
    {

    }

    void VulkanGPUContext::BindBufferData(uint32_t slot, const void* data, uint32_t size)
    {

    }

    void VulkanGPUContext::TextureBarrier(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        VkImageMemoryBarrier imageMemoryBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        imageMemoryBarrier.oldLayout = oldLayout;
        imageMemoryBarrier.newLayout = newLayout;
        imageMemoryBarrier.image = image;
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
        imageMemoryBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
        imageMemoryBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        // Source layouts (old)
        // Source access mask controls actions that have to be finished on the old layout
        // before it will be transitioned to the new layout
        switch (oldLayout)
        {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            // Image layout is undefined (or does not matter)
            // Only valid as initial layout
            // No flags required, listed only for completeness
            imageMemoryBarrier.srcAccessMask = 0;
            break;

        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            // Image is preinitialized
            // Only valid as initial layout for linear images, preserves memory contents
            // Make sure host writes have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image is a color attachment
            // Make sure any writes to the color buffer have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image is a depth/stencil attachment
            // Make sure any writes to the depth/stencil buffer have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Image is a transfer source
            // Make sure any reads from the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Image is a transfer destination
            // Make sure any writes to the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Image is read by a shader
            // Make sure any shader reads from the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            // Other source layouts aren't handled (yet)
            break;
        }

        // Target layouts (new)
        // Destination access mask controls the dependency for the new image layout
        switch (newLayout)
        {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Image will be used as a transfer destination
            // Make sure any writes to the image have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Image will be used as a transfer source
            // Make sure any reads from the image have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image will be used as a color attachment
            // Make sure any writes to the color buffer have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image layout will be used as a depth/stencil attachment
            // Make sure any writes to depth/stencil buffer have been finished
            imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Image will be read in a shader (sampler, input attachment)
            // Make sure any writes to the image have been finished
            if (imageMemoryBarrier.srcAccessMask == 0)
            {
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
            }
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            // Other source layouts aren't handled (yet)
            break;
        }

        VkCommandBuffer commandBuffer = frames[activeFrameIndex].primaryCommandBuffer;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
    }
}

#endif // TODO
