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
#include "Core/Log.h"
#include "VulkanGPUSwapChain.h"
#include "VulkanGPUTexture.h"
#include "VulkanGPUDevice.h"

namespace alimer
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

    VulkanGPUSwapChain::VulkanGPUSwapChain(VulkanGPUDevice* device_, VkSurfaceKHR surface_, bool verticalSync_)
        : device(device_)
        , presentQueue(device->GetGraphicsQueue())
        , surface(surface_)
        , verticalSync(verticalSync_)
    {
        UpdateSwapchain();
    }


    VulkanGPUSwapChain::~VulkanGPUSwapChain()
    {
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

    bool VulkanGPUSwapChain::UpdateSwapchain()
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

            const bool srgb = false;
            bool found = false;
            for (uint32_t i = 0; i < surfaceCaps.formats.size(); i++)
            {
                if (srgb)
                {
                    if (surfaceCaps.formats[i].format == VK_FORMAT_R8G8B8A8_SRGB ||
                        surfaceCaps.formats[i].format == VK_FORMAT_B8G8R8A8_SRGB ||
                        surfaceCaps.formats[i].format == VK_FORMAT_A8B8G8R8_SRGB_PACK32)
                    {
                        format = surfaceCaps.formats[i];
                        found = true;
                        break;
                    }
                }
                else
                {
                    if (surfaceCaps.formats[i].format == VK_FORMAT_R8G8B8A8_UNORM ||
                        surfaceCaps.formats[i].format == VK_FORMAT_B8G8R8A8_UNORM ||
                        surfaceCaps.formats[i].format == VK_FORMAT_A8B8G8R8_UNORM_PACK32)
                    {
                        format = surfaceCaps.formats[i];
                        found = true;
                        break;
                    }
                }
            }

            if (!found)
            {
                format = surfaceCaps.formats[0];
            }
        }

        /* Extent */
        VkExtent2D swapchainSize = {}; // { backbufferSize.x, backbufferSize.y };
        if (swapchainSize.width < 1 || swapchainSize.height < 1)
        {
            swapchainSize = surfaceCaps.capabilities.currentExtent;
        }
        else
        {
            swapchainSize.width = Max(swapchainSize.width, surfaceCaps.capabilities.minImageExtent.width);
            swapchainSize.width = Min(swapchainSize.width, surfaceCaps.capabilities.maxImageExtent.width);
            swapchainSize.height = Max(swapchainSize.height, surfaceCaps.capabilities.minImageExtent.height);
            swapchainSize.height = Min(swapchainSize.height, surfaceCaps.capabilities.maxImageExtent.height);
        }

        VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        // Enable transfer source on swap chain images if supported
        if (surfaceCaps.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
            imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            //textureUsage |= VGPUTextureUsage_CopySrc;
        }

        // Enable transfer destination on swap chain images if supported
        if (surfaceCaps.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
            imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            //textureUsage |= VGPUTextureUsage_CopyDst;
        }

        VkSurfaceTransformFlagBitsKHR pre_transform;
        if ((surfaceCaps.capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) != 0)
            pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        else
            pre_transform = surfaceCaps.capabilities.currentTransform;

        VkCompositeAlphaFlagBitsKHR composite_mode = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        if (surfaceCaps.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
            composite_mode = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
        if (surfaceCaps.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
            composite_mode = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        if (surfaceCaps.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
            composite_mode = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
        if (surfaceCaps.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
            composite_mode = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;

        VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
        if (!verticalSync)
        {
            // The immediate present mode is not necessarily supported:
            for (auto& presentMode : surfaceCaps.presentModes)
            {
                if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                    break;
                }
                if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) && (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR))
                {
                    swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                }
            }
        }

        VkSwapchainKHR oldSwapchain = handle;

        /* We use same family for graphics and present so no sharing is necessary. */
        VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = format.format;
        createInfo.imageColorSpace = format.colorSpace;
        createInfo.imageExtent = swapchainSize;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = imageUsage;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = NULL;
        createInfo.preTransform = pre_transform;
        createInfo.compositeAlpha = composite_mode;
        createInfo.presentMode = swapchainPresentMode;
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
        images.resize(imageCount);
        //swapChainImageLayouts.resize(imageCount);
        vkGetSwapchainImagesKHR(device->GetHandle(), handle, &imageCount, images.data());

#if TODO


        frame.clear();
        frame.resize(imageCount);

        for (uint32_t i = 0; i < imageCount; i++)
        {
            swapChainImageLayouts[i] = VK_IMAGE_LAYOUT_UNDEFINED;

            VkImageViewCreateInfo view_info{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
            view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view_info.format = format.format;
            view_info.image = swapchainImages[i];
            view_info.subresourceRange.levelCount = 1;
            view_info.subresourceRange.layerCount = 1;
            view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            view_info.components.r = VK_COMPONENT_SWIZZLE_R;
            view_info.components.g = VK_COMPONENT_SWIZZLE_G;
            view_info.components.b = VK_COMPONENT_SWIZZLE_B;
            view_info.components.a = VK_COMPONENT_SWIZZLE_A;

            VkImageView imageView;
            VK_CHECK(vkCreateImageView(device, &view_info, nullptr, &imageView));
            swapchainImageViews.push_back(imageView);

            //backbufferTextures[backbufferIndex] = new VulkanTexture(this, swapChainImages[i]);
            //backbufferTextures[backbufferIndex]->SetName(fmt::format("Back Buffer {}", i));
            SetObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)swapchainImages[i], fmt::format("Back Buffer {}", i));
    }
#endif // TODO

        return true;
}

    VkResult VulkanGPUSwapChain::AcquireNextImage(VkSemaphore acquireSemaphore, uint32_t* imageIndex)
    {
        return vkAcquireNextImageKHR(device->GetHandle(), handle, UINT64_MAX, acquireSemaphore, VK_NULL_HANDLE, imageIndex);
    }

    VkResult VulkanGPUSwapChain::Present(VkSemaphore semaphore, uint32 imageIndex)
    {
        VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &handle;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &semaphore;
        return vkQueuePresentKHR(presentQueue, &presentInfo);
    }
}

#endif // TODO
