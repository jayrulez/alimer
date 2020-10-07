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

#include "Core/Log.h"
#include "VulkanSwapChain.h"
#include "VulkanTexture.h"
#include "VulkanGraphicsDevice.h"
#include "VulkanCommandBuffer.h"

namespace Alimer
{
    namespace
    {
        struct SwapChainSupportDetails {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        };

        SwapChainSupportDetails QuerySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, bool getSurfaceCapabilities2, bool win32_full_screen_exclusive)
        {
            SwapChainSupportDetails details;

            VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR };
            surfaceInfo.surface = surface;

            if (getSurfaceCapabilities2)
            {
                VkSurfaceCapabilities2KHR surfaceCaps2 = { VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR };

                if (vkGetPhysicalDeviceSurfaceCapabilities2KHR(physicalDevice, &surfaceInfo, &surfaceCaps2) != VK_SUCCESS)
                {
                    return details;
                }
                details.capabilities = surfaceCaps2.surfaceCapabilities;

                uint32_t formatCount;
                if (vkGetPhysicalDeviceSurfaceFormats2KHR(physicalDevice, &surfaceInfo, &formatCount, nullptr) != VK_SUCCESS)
                {
                    return details;
                }

                std::vector<VkSurfaceFormat2KHR> formats2(formatCount);

                for (auto& format2 : formats2)
                {
                    format2 = {};
                    format2.sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
                }

                if (vkGetPhysicalDeviceSurfaceFormats2KHR(physicalDevice, &surfaceInfo, &formatCount, formats2.data()) != VK_SUCCESS)
                {
                    return details;
                }

                details.formats.reserve(formatCount);
                for (auto& format2 : formats2)
                {
                    details.formats.push_back(format2.surfaceFormat);
                }
            }
            else
            {
                if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities) != VK_SUCCESS)
                {
                    return details;
                }

                uint32_t formatCount;
                if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr) != VK_SUCCESS)
                {
                    return details;
                }

                details.formats.resize(formatCount);
                if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data()) != VK_SUCCESS)
                {
                    return details;
                }
            }

            uint32_t presentModeCount = 0;
#ifdef _WIN32
            if (getSurfaceCapabilities2 && win32_full_screen_exclusive)
            {
                if (vkGetPhysicalDeviceSurfacePresentModes2EXT(physicalDevice, &surfaceInfo, &presentModeCount, nullptr) != VK_SUCCESS)
                {
                    return details;
                }

                details.presentModes.resize(presentModeCount);
                if (vkGetPhysicalDeviceSurfacePresentModes2EXT(physicalDevice, &surfaceInfo, &presentModeCount, details.presentModes.data()) != VK_SUCCESS)
                {
                    return details;
                }
            }
            else
#endif
            {
                if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr) != VK_SUCCESS)
                {
                    return details;
                }

                details.presentModes.resize(presentModeCount);
                if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data()) != VK_SUCCESS)
                {
                    return details;
                }
            }

            return details;
        }
    }

    VulkanSwapChain::VulkanSwapChain(VulkanGraphicsDevice* device_, VkSurfaceKHR surface, const SwapChainDescription& desc)
        : SwapChain(desc)
        , device(device_)
        , presentQueue(device->GetGraphicsQueue())
        , surface{ surface }
    {
        RecreateSwapchain();
    }


    VulkanSwapChain::~VulkanSwapChain()
    {
        Destroy();
    }

    void VulkanSwapChain::Destroy()
    {
        colorTextures.clear();

        if (handle != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(device->GetHandle(), handle, nullptr);
            handle = VK_NULL_HANDLE;
        }

        if (surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(device->GetVkInstance(), surface, nullptr);
            surface = VK_NULL_HANDLE;
        }
    }

    bool VulkanSwapChain::RecreateSwapchain()
    {
        if (handle != VK_NULL_HANDLE)
        {
            device->WaitForGPU();
        }

        SwapChainSupportDetails surfaceCaps = QuerySwapchainSupport(
            device->GetVkPhysicalDevice(),
            surface,
            device->GetInstanceExtensions().get_surface_capabilities2,
            device->GetPhysicalDeviceExtensions().win32_full_screen_exclusive
        );

        /* Detect image count. */
        imageCount = surfaceCaps.capabilities.minImageCount + 1;
        if ((surfaceCaps.capabilities.maxImageCount > 0) && (imageCount > surfaceCaps.capabilities.maxImageCount))
        {
            imageCount = surfaceCaps.capabilities.maxImageCount;
        }

        /* Surface format. */
        VkSurfaceFormatKHR format;
        if (surfaceCaps.formats.size() == 1 &&
            surfaceCaps.formats[0].format == VK_FORMAT_UNDEFINED)
        {
            format = surfaceCaps.formats[0];
            format.format = VK_FORMAT_B8G8R8A8_UNORM;
        }
        else
        {
            if (surfaceCaps.formats.size() == 0)
            {
                LOGE("Vulkan: Surface has no formats.");
                return false;
            }

            bool found = false;
            VkFormat vkColorFormat = ToVkFormat(colorFormat);
            for (uint32_t i = 0; i < surfaceCaps.formats.size(); i++)
            {
                if (surfaceCaps.formats[i].format == vkColorFormat)
                {
                    format = surfaceCaps.formats[i];
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                format = surfaceCaps.formats[0];
            }
        }

        /* Extent */
        width = Max(width, surfaceCaps.capabilities.minImageExtent.width);
        width = Min(width, surfaceCaps.capabilities.maxImageExtent.width);
        height = Max(height, surfaceCaps.capabilities.minImageExtent.height);
        height = Min(height, surfaceCaps.capabilities.maxImageExtent.height);

        VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        usage |= surfaceCaps.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        usage |= surfaceCaps.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        VkSurfaceTransformFlagBitsKHR preTransform;
        if ((surfaceCaps.capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) != 0)
            preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        else
            preTransform = surfaceCaps.capabilities.currentTransform;

        VkCompositeAlphaFlagBitsKHR compositeMode = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        if (surfaceCaps.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
            compositeMode = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
        if (surfaceCaps.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
            compositeMode = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        if (surfaceCaps.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
            compositeMode = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
        if (surfaceCaps.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
            compositeMode = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;

        // The immediate present mode is not necessarily supported.
        VkPresentModeKHR vkPresentMode = VK_PRESENT_MODE_FIFO_KHR;
        /*if (presentMode == PresentMode::Mailbox)
        {
            for (auto& presentMode : surfaceCaps.presentModes)
            {
                if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    vkPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                    break;
                }
            }

            if (vkPresentMode != VK_PRESENT_MODE_MAILBOX_KHR)
            {
                for (auto& presentMode : surfaceCaps.presentModes)
                {
                    if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                    {
                        vkPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                        break;
                    }
                }
            }
        }

        for (auto& presentMode : surfaceCaps.presentModes)
        {
            if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                immediatePresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
            else if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            {
                immediatePresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }*/

        VkSwapchainKHR oldSwapchain = handle;

        /* We use same family for graphics and present so no sharing is necessary. */
        VkSwapchainCreateInfoKHR createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.pNext = NULL;
        createInfo.flags = 0;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = format.format;
        createInfo.imageColorSpace = format.colorSpace;
        createInfo.imageExtent.width = width;
        createInfo.imageExtent.height = height;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = usage;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
        createInfo.preTransform = preTransform;
        createInfo.compositeAlpha = compositeMode;
        createInfo.presentMode = vkPresentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = oldSwapchain;

        VkResult result = vkCreateSwapchainKHR(device->GetHandle(), &createInfo, nullptr, &handle);
        if (result != VK_SUCCESS) {
            return false;
        }

        if (oldSwapchain != VK_NULL_HANDLE)
        {
            /*for (VkImageView view : swapchainImageViews)
            {
                vkDestroyImageView(device, view, nullptr);
            }

            VK_CHECK(vkGetSwapchainImagesKHR(device, oldSwapchain, &imageCount, nullptr));

            for (size_t i = 0; i < imageCount; i++)
            {
                TeardownPerFrame(frame[i]);
            }

            swapchainImageViews.clear();*/
            vkDestroySwapchainKHR(device->GetHandle(), oldSwapchain, nullptr);
        }

        vkGetSwapchainImagesKHR(device->GetHandle(), handle, &imageCount, nullptr);
        std::vector<VkImage> images(imageCount);
        colorTextures.resize(imageCount);
        vkGetSwapchainImagesKHR(device->GetHandle(), handle, &imageCount, images.data());

        TextureDescription textureDesc = TextureDescription::New2D(colorFormat, width, height, false, TextureUsage::RenderTarget);
        for (uint32_t i = 0; i < imageCount; i++)
        {
            colorTextures[i] = new VulkanTexture(device, textureDesc, images[i], VK_IMAGE_LAYOUT_UNDEFINED);
            colorTextures[i]->SetName(fmt::format("Back Buffer {}", i));
        }

        return true;
        //return AcquireNextImage();
    }

    bool VulkanSwapChain::AcquireNextImage()
    {
        imageIndex = kInvalidImageIndex;

        VkSemaphore aquiredSemaphore = device->RequestSemaphore();

        VkResult result = AcquireNextImage(imageIndex, aquiredSemaphore, VK_NULL_HANDLE);
        if (result != VK_SUCCESS)
        {
            VK_LOG_ERROR(result, "Vulkan: Failed to acquire next Vulkan image");
            return false;
        }

        if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            // Recreate Swapchain
            RecreateSwapchain();
        }

        return true;
    }

    VkResult VulkanSwapChain::AcquireNextImage(uint32_t& imageIndex, VkSemaphore imageAcquiredSemaphore, VkFence fence)
    {
        return vkAcquireNextImageKHR(device->GetHandle(), handle, Limits<uint64_t>::Max, imageAcquiredSemaphore, fence, &imageIndex);
    }

    bool VulkanSwapChain::Present()
    {
        if (!AcquireNextImage())
            return false;

        //VulkanCommandBuffer* commandBuffer = static_cast<VulkanCommandBuffer*>(device->GetCommandBuffer());
        //commandBuffer->Commit();

        VkPresentInfoKHR presentInfo;
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext = nullptr;
        presentInfo.waitSemaphoreCount = 0u;
        presentInfo.pWaitSemaphores = nullptr;
        presentInfo.swapchainCount = 1u;
        presentInfo.pSwapchains = &handle;
        presentInfo.pImageIndices = &currentBackBufferIndex;
        presentInfo.pResults = nullptr;
        VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);

        // Handle Outdated error in present.
        if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            return RecreateSwapchain();
        }
        else if (result != VK_SUCCESS)
        {
            LOGE("Failed to present swapchain image.");
        }

        return AcquireNextImage();
    }

    CommandBuffer* VulkanSwapChain::CurrentFrameCommandBuffer()
    {
        return nullptr;
    }
}
