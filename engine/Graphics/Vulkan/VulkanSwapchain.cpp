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

#include "VulkanSwapchain.h"
#include "VulkanBackend.h"
#include "VulkanResources.h"

namespace alimer
{
    namespace
    {
        inline uint32_t ChooseImageCount(uint32_t requestImageCount, uint32_t minImageCount, uint32_t maxImageCount)
        {
            if (maxImageCount != 0)
            {
                requestImageCount = Min(requestImageCount, maxImageCount);
            }

            requestImageCount = Max(requestImageCount, minImageCount);
            return requestImageCount;
        }

        inline VkSurfaceFormatKHR ChooseSurfaceFormat(
            const VkSurfaceFormatKHR requested_surface_format,
            const std::vector<VkSurfaceFormatKHR>& available_surface_formats,
            const std::vector<VkSurfaceFormatKHR>& surface_format_priority_list)
        {
            // Try to find the requested surface format in the supported surface formats
            auto surface_format_it = std::find_if(
                available_surface_formats.begin(),
                available_surface_formats.end(),
                [&requested_surface_format](const VkSurfaceFormatKHR& surface) {
                    if (surface.format == requested_surface_format.format &&
                        surface.colorSpace == requested_surface_format.colorSpace)
                    {
                        return true;
                    }

                    return false;
                });

            // If the requested surface format isn't found, then try to request a format from the priority list
            if (surface_format_it == available_surface_formats.end())
            {
                for (auto& surface_format : surface_format_priority_list)
                {
                    surface_format_it = std::find_if(
                        available_surface_formats.begin(),
                        available_surface_formats.end(),
                        [&surface_format](const VkSurfaceFormatKHR& surface) {
                            if (surface.format == surface_format.format &&
                                surface.colorSpace == surface_format.colorSpace)
                            {
                                return true;
                            }

                            return false;
                        });
                    if (surface_format_it != available_surface_formats.end())
                    {
                        //LOGW("(Swapchain) Surface format ({}) not supported. Selecting ({}).", to_string(requested_surface_format), to_string(*surface_format_it));
                        return *surface_format_it;
                    }
                }

                // If nothing found, default the first supporte surface format
                surface_format_it = available_surface_formats.begin();
                //LOGW("(Swapchain) Surface format ({}) not supported. Selecting ({}).", to_string(requested_surface_format), to_string(*surface_format_it));
            }
            else
            {
                //LOGI("(Swapchain) Surface format selected: {}", to_string(requested_surface_format));
            }

            return *surface_format_it;
        }

        inline VkExtent2D ChooseExtent(
            VkExtent2D request_extent,
            const VkExtent2D& min_image_extent,
            const VkExtent2D& max_image_extent,
            const VkExtent2D& current_extent)
        {
            if (request_extent.width < 1 || request_extent.height < 1)
            {
                LOGW("(Swapchain) Image extent ({}, {}) not supported. Selecting ({}, {}).", request_extent.width, request_extent.height, current_extent.width, current_extent.height);
                return current_extent;
            }

            request_extent.width = Max(request_extent.width, min_image_extent.width);
            request_extent.width = Min(request_extent.width, max_image_extent.width);

            request_extent.height = Max(request_extent.height, min_image_extent.height);
            request_extent.height = Min(request_extent.height, max_image_extent.height);

            return request_extent;
        }

        inline VkSurfaceTransformFlagBitsKHR ChooseTransform(
            VkSurfaceTransformFlagBitsKHR requestTransform,
            VkSurfaceTransformFlagsKHR supportedTransform,
            VkSurfaceTransformFlagBitsKHR currentTransform)
        {
            if (requestTransform & supportedTransform)
            {
                return requestTransform;
            }

            return currentTransform;
        }

        inline VkCompositeAlphaFlagBitsKHR ChooseCompositeAlpha(VkCompositeAlphaFlagBitsKHR request_composite_alpha, VkCompositeAlphaFlagsKHR supportedCompositeAlpha)
        {
            if (request_composite_alpha & supportedCompositeAlpha)
            {
                return request_composite_alpha;
            }

            static const std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
                VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
                VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR};

            for (VkCompositeAlphaFlagBitsKHR composite_alpha : compositeAlphaFlags)
            {
                if (composite_alpha & supportedCompositeAlpha)
                {
                    LOGW("(Swapchain) Composite alpha '{}' not supported. Selecting '{}.", to_string(request_composite_alpha), to_string(composite_alpha));
                    return composite_alpha;
                }
            }

            throw std::runtime_error("No compatible composite alpha found.");
        }

        inline VkPresentModeKHR ChoosePresentMode(VkPresentModeKHR request_present_mode,
                                                  const std::vector<VkPresentModeKHR>& available_present_modes,
                                                  const std::vector<VkPresentModeKHR>& present_mode_priority_list)
        {
            auto present_mode_it = std::find(available_present_modes.begin(), available_present_modes.end(), request_present_mode);

            if (present_mode_it == available_present_modes.end())
            {
                // If nothing found, always default to FIFO
                VkPresentModeKHR chosen_present_mode = VK_PRESENT_MODE_FIFO_KHR;

                for (auto& present_mode : present_mode_priority_list)
                {
                    if (std::find(available_present_modes.begin(), available_present_modes.end(), present_mode) != available_present_modes.end())
                    {
                        chosen_present_mode = present_mode;
                    }
                }

                //LOGW("(Swapchain) Present mode '{}' not supported. Selecting '{}'.", to_string(request_present_mode), to_string(chosen_present_mode));
                return chosen_present_mode;
            }
            else
            {
                //LOGI("(Swapchain) Present mode selected: {}", to_string(request_present_mode));
                return *present_mode_it;
            }
        }

    }

    VulkanSwapchain::VulkanSwapchain(VulkanGraphics& graphics, VkSurfaceKHR surface, bool verticalSync)
        : graphics{graphics}
        , surface{surface}
        , desiredPresentMode(verticalSync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR)
    {
        CreateOrResize();
    }

    VulkanSwapchain::~VulkanSwapchain()
    {
        if (handle != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(graphics.GetVkDevice(), handle, nullptr);
        }

        if (surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(graphics.GetVkInstance(), surface, nullptr);
        }
    }

    void VulkanSwapchain::CreateOrResize()
    {
        VkPhysicalDevice physicalDevice = graphics.GetVkPhysicalDevice();

        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities));

        uint32_t surfaceFormatCount{0U};
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr));
        std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, surfaceFormats.data()));

        uint32_t presentModeCount{0U};
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr));
        std::vector<VkPresentModeKHR> surfacePresentModes(presentModeCount);
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, surfacePresentModes.data()));

        VkSwapchainKHR oldSwapchain = handle;
        VkSurfaceFormatKHR surfaceFormat{};
        surfaceFormat = ChooseSurfaceFormat(surfaceFormat, surfaceFormats, surfaceFormatPriorityList);
        VkExtent2D extent{};

        VkSwapchainCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        createInfo.surface = surface;
        createInfo.minImageCount = ChooseImageCount(kImageCount, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount);
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = ChooseExtent(extent, surfaceCapabilities.minImageExtent, surfaceCapabilities.maxImageExtent, surfaceCapabilities.currentExtent);
        createInfo.imageArrayLayers = 1u;

        // Usage: Enable transfer source/dest on swap chain images if supported
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
        {
            createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        {
            createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }

        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0u;
        createInfo.pQueueFamilyIndices = nullptr;
        createInfo.preTransform = ChooseTransform(transform, surfaceCapabilities.supportedTransforms, surfaceCapabilities.currentTransform);
        createInfo.compositeAlpha = ChooseCompositeAlpha(VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR, surfaceCapabilities.supportedCompositeAlpha);
        createInfo.presentMode = ChoosePresentMode(desiredPresentMode, surfacePresentModes, presentModePriorityList);
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = oldSwapchain;

        VkResult result = vkCreateSwapchainKHR(graphics.GetVkDevice(), &createInfo, nullptr, &handle);

        if (result != VK_SUCCESS)
        {
            VK_THROW(result, "Cannot create Swapchain");
        }

        if (surfaceFormat.format == VK_FORMAT_R8G8B8A8_SRGB)
            colorFormat = PixelFormat::RGBA8UnormSrgb;
        else if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB)
            colorFormat = PixelFormat::BGRA8UnormSrgb;
        else if (surfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM)
            colorFormat = PixelFormat::BGRA8Unorm;
        else
            colorFormat = PixelFormat::BGRA8Unorm;

        VK_CHECK(vkGetSwapchainImagesKHR(graphics.GetVkDevice(), handle, &imageCount, nullptr));

        std::vector<VkImage> images(imageCount);
        VK_CHECK(vkGetSwapchainImagesKHR(graphics.GetVkDevice(), handle, &imageCount, images.data()));

        colorTextures.resize(imageCount);
        for (uint32_t i = 0; i < imageCount; i++)
        {

        }
    }

    VkResult VulkanSwapchain::AcquireNextImage(uint32_t& imageIndex, VkSemaphore imageAcquiredSemaphore, VkFence fence)
    {
        return vkAcquireNextImageKHR(graphics.GetVkDevice(), handle, std::numeric_limits<uint64_t>::max(), imageAcquiredSemaphore, fence, &imageIndex);
    }
}
