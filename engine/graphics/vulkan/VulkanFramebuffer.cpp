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
#include "VulkanFramebuffer.h"
#include "VulkanGPUDevice.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "math/math.h"

using namespace std;

namespace alimer
{
    namespace {
        static VkPresentModeKHR ChooseSwapPresentMode(
            const vector<VkPresentModeKHR>& availablePresentModes, bool vsyncEnabled)
        {
            // Try to match the correct present mode to the vsync state.
            vector<VkPresentModeKHR> desiredModes;
            if (vsyncEnabled) desiredModes = { VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_FIFO_RELAXED_KHR };
            else desiredModes = { VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_MAILBOX_KHR };

            // Iterate over all available present mdoes and match to one of the desired ones.
            for (const auto& availablePresentMode : availablePresentModes)
            {
                for (auto mode : desiredModes)
                {
                    if (availablePresentMode == mode)
                        return availablePresentMode;
                }
            }

            // If no match was found, return the first present mode or default to FIFO.
            if (availablePresentModes.size() > 0) return availablePresentModes[0];
            else return VK_PRESENT_MODE_FIFO_KHR;
        }
    }

    VulkanFramebuffer::VulkanFramebuffer(VulkanGPUDevice* device, VkSurfaceKHR surface, uint32_t width, uint32_t height, const SwapChainDescriptor* descriptor)
        : Framebuffer(device)
        , surface{ surface }
    {
        BackendResize();
    }

    VulkanFramebuffer::~VulkanFramebuffer()
    {
    }

    FramebufferResizeResult VulkanFramebuffer::BackendResize()
    {
      

        auto vkGPUDevice = StaticCast<VulkanGPUDevice>(device);
        VkPhysicalDevice gpu = vkGPUDevice->GetPhysicalDevice();
        


        const bool tripleBuffer = false;
        uint32_t imageCount = (tripleBuffer) ? 3 : surfaceCapabilities.minImageCount + 1;
        if (surfaceCapabilities.maxImageCount > 0 &&
            imageCount > surfaceCapabilities.maxImageCount)
        {
            imageCount = surfaceCapabilities.maxImageCount;
        }

        // Clamp the target width, height to boundaries.
        extent.width =
            max(min(extent.width, surfaceCapabilities.maxImageExtent.width), surfaceCapabilities.minImageExtent.width);
        extent.height =
            max(min(extent.height, surfaceCapabilities.maxImageExtent.height), surfaceCapabilities.minImageExtent.height);

        // Enable transfer source and destination on swap chain images if supported
        VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
            imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
            imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }

        VkSurfaceTransformFlagBitsKHR preTransform;
        if ((surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) != 0) {
            // We prefer a non-rotated transform
            preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        }
        else {
            preTransform = surfaceCapabilities.currentTransform;
        }

        VkCompositeAlphaFlagBitsKHR compositeMode = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
            compositeMode = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
        if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
            compositeMode = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
            compositeMode = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
        if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
            compositeMode = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;

        /* Choose present mode. */
        uint32_t num_present_modes;
        vector<VkPresentModeKHR> presentModes;
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &num_present_modes, nullptr) != VK_SUCCESS)
            return FramebufferResizeResult::Error;
        presentModes.resize(num_present_modes);
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &num_present_modes, presentModes.data()) != VK_SUCCESS)
            return FramebufferResizeResult::Error;

        VkPresentModeKHR presentMode = ChooseSwapPresentMode(presentModes, true);
        VkSwapchainKHR oldSwapchain = swapchain;

        VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = format.format;
        createInfo.imageColorSpace = format.colorSpace;
        createInfo.imageExtent.width = extent.width;
        createInfo.imageExtent.height = extent.height;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = imageUsage;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.preTransform = preTransform;
        createInfo.compositeAlpha = compositeMode;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = oldSwapchain;

        VkResult  result = vkCreateSwapchainKHR(vkGPUDevice->GetDevice(), &createInfo, nullptr, &swapchain);
        if (result != VK_SUCCESS)
        {
            VK_THROW(result, "Cannot create Swapchain");
            return FramebufferResizeResult::Error;
        }

        if (oldSwapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(vkGPUDevice->GetDevice(), oldSwapchain, nullptr);
        }

        PixelFormat colorFormat;
        switch (createInfo.imageFormat)
        {
        case VK_FORMAT_R8G8B8A8_SRGB:
            colorFormat = PixelFormat::RGBA8UNormSrgb;
            break;

        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
            colorFormat = PixelFormat::BGRA8UNormSrgb;
            break;

        case VK_FORMAT_R8G8B8A8_UNORM:
            colorFormat = PixelFormat::RGBA8UNorm;
            break;

        default:
            colorFormat = PixelFormat::BGRA8UNorm;
            break;
        }

        if (vkGetSwapchainImagesKHR(vkGPUDevice->GetDevice(), swapchain, &imageCount, nullptr) != VK_SUCCESS)
        {
            return FramebufferResizeResult::Error;
        }

        vector<VkImage> swapchainImages;
        //textures.resize(imageCount);
        if (vkGetSwapchainImagesKHR(vkGPUDevice->GetDevice(), swapchain, &imageCount, swapchainImages.data()) != VK_SUCCESS)
            return FramebufferResizeResult::Error;

        for (uint32_t i = 0; i < imageCount; ++i)
        {

        }

        return FramebufferResizeResult::Success;
    }
}

#endif // TODO
