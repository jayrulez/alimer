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

#include "SwapChainVK.h"
#include "TextureVK.h"
#include "CommandQueueVK.h"
#include "GraphicsDeviceVK.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "math/math.h"
#include <vector>

#if defined(VK_USE_PLATFORM_WIN32_KHR)
#include <foundation/windows.h>
#endif

namespace alimer
{
    namespace {
        static VkPresentModeKHR ChooseSwapPresentMode(
            const std::vector<VkPresentModeKHR>& availablePresentModes, bool vsyncEnabled)
        {
            // Try to match the correct present mode to the vsync state.
            std::vector<VkPresentModeKHR> desiredModes;
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

    SwapChainVK::SwapChainVK(GraphicsDeviceVK* device_)
        : device(device_)
        , vkFormat{ VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }
        , desc()
    {

    }

    SwapChainVK::~SwapChainVK()
    {
        Destroy();
    }

    bool SwapChainVK::Init(window_t* window, ICommandQueue* commandQueue_, const SwapChainDesc* pDesc)
    {
        VkResult result = VK_ERROR_UNKNOWN;

#if defined(VK_USE_PLATFORM_WIN32_KHR)
        VkWin32SurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
        createInfo.hinstance = GetModuleHandle(NULL);
        createInfo.hwnd = reinterpret_cast<HWND>(window_handle(window));
        result = vkCreateWin32SurfaceKHR(device->GetInstance(), &createInfo, nullptr, &surface);
#endif

        if (result != VK_SUCCESS)
        {
            ALIMER_LOGERROR("Failed to create surface for SwapChain");
            return false;
        }

        commandQueue = static_cast<CommandQueueVK*>(commandQueue_);
        if (!commandQueue->SupportPresent(surface))
        {
            ALIMER_LOGERROR("[Vulkan]: Queue does not support presentation");
            return false;
        }


        VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
            nullptr,
            surface
        };

        uint32_t format_count;
        std::vector<VkSurfaceFormatKHR> formats;

        if (device->GetVulkanFeatures().surfaceCapabilities2)
        {
            if (vkGetPhysicalDeviceSurfaceFormats2KHR(device->GetPhysicalDevice(), &surfaceInfo, &format_count, nullptr) != VK_SUCCESS)
                return false;

            std::vector<VkSurfaceFormat2KHR> formats2(format_count);

            for (VkSurfaceFormat2KHR& format2 : formats2)
            {
                format2 = {};
                format2.sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
            }

            if (vkGetPhysicalDeviceSurfaceFormats2KHR(device->GetPhysicalDevice(), &surfaceInfo, &format_count, formats2.data()) != VK_SUCCESS)
                return false;

            formats.reserve(format_count);
            for (auto& f : formats2)
                formats.push_back(f.surfaceFormat);
        }
        else
        {
            if (vkGetPhysicalDeviceSurfaceFormatsKHR(device->GetPhysicalDevice(), surface, &format_count, nullptr) != VK_SUCCESS)
                return false;
            formats.resize(format_count);
            if (vkGetPhysicalDeviceSurfaceFormatsKHR(device->GetPhysicalDevice(), surface, &format_count, formats.data()) != VK_SUCCESS)
                return false;
        }

        const bool srgb = false;
        if (format_count == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
        {
            vkFormat = formats[0];
            vkFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
        }
        else
        {
            if (format_count == 0)
            {
                ALIMER_LOGE("Vulkan: Surface has no formats.");
                return false;
            }

            bool found = false;
            for (uint32_t i = 0; i < format_count; i++)
            {
                if (srgb)
                {
                    if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB ||
                        formats[i].format == VK_FORMAT_B8G8R8A8_SRGB ||
                        formats[i].format == VK_FORMAT_A8B8G8R8_SRGB_PACK32)
                    {
                        vkFormat = formats[i];
                        found = true;
                    }
                }
                else
                {
                    if (formats[i].format == VK_FORMAT_R8G8B8A8_UNORM ||
                        formats[i].format == VK_FORMAT_B8G8R8A8_UNORM ||
                        formats[i].format == VK_FORMAT_A8B8G8R8_UNORM_PACK32)
                    {
                        vkFormat = formats[i];
                        found = true;
                    }
                }
            }

            if (!found)
                vkFormat = formats[0];
        }

        memcpy(&desc, pDesc, sizeof(desc));
        return InitSwapChain(desc.width, desc.height);
    }

    bool SwapChainVK::InitSwapChain(uint32_t width, uint32_t height)
    {
        VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
            nullptr,
            surface
        };

        VkSurfaceCapabilitiesKHR capabilities;
        if (device->GetVulkanFeatures().surfaceCapabilities2)
        {
            VkSurfaceCapabilities2KHR surfaceCapabilities2 = { VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR };

            // TODO: Add fullscreen exclusive.

            if (vkGetPhysicalDeviceSurfaceCapabilities2KHR(device->GetPhysicalDevice(), &surfaceInfo, &surfaceCapabilities2) != VK_SUCCESS)
                return false;

            capabilities = surfaceCapabilities2.surfaceCapabilities;
        }
        else
        {
            if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->GetPhysicalDevice(), surface, &capabilities) != VK_SUCCESS)
                return false;
        }

        if (capabilities.maxImageExtent.width == 0
            && capabilities.maxImageExtent.height == 0)
        {
            return false;
        }

        /* Choose present mode. */
        uint32_t num_present_modes;
        std::vector<VkPresentModeKHR> presentModes;
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(device->GetPhysicalDevice(), surface, &num_present_modes, nullptr) != VK_SUCCESS)
            return false;
        presentModes.resize(num_present_modes);
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(device->GetPhysicalDevice(), surface, &num_present_modes, presentModes.data()) != VK_SUCCESS)
            return false;

        const bool tripleBuffer = false;
        uint32_t minImageCount = (tripleBuffer) ? 3 : capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 &&
            minImageCount > capabilities.maxImageCount)
        {
            minImageCount = capabilities.maxImageCount;
        }

        // Choose swapchain extent (Size)
        VkExtent2D newExtent = {};
        if (capabilities.currentExtent.width != UINT32_MAX ||
            capabilities.currentExtent.height != UINT32_MAX ||
            width == 0 || height == 0)
        {
            newExtent = capabilities.currentExtent;
        }
        else
        {
            newExtent.width = max(capabilities.minImageExtent.width, min(capabilities.maxImageExtent.width, width));
            newExtent.height = max(capabilities.minImageExtent.height, min(capabilities.maxImageExtent.height, height));
        }
        newExtent.width = max(newExtent.width, 1u);
        newExtent.height = max(newExtent.height, 1u);

        // Enable transfer source and destination on swap chain images if supported
        VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
            imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        if (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
            imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }

        VkSurfaceTransformFlagBitsKHR preTransform;
        if ((capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) != 0) {
            // We prefer a non-rotated transform
            preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        }
        else {
            preTransform = capabilities.currentTransform;
        }

        VkCompositeAlphaFlagBitsKHR compositeMode = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
            compositeMode = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
        if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
            compositeMode = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
            compositeMode = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
        if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
            compositeMode = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;

        VkPresentModeKHR presentMode = ChooseSwapPresentMode(presentModes, true);

        VkSwapchainKHR oldSwapchain = handle;

        VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        createInfo.surface = surface;
        createInfo.minImageCount = minImageCount;
        createInfo.imageFormat = vkFormat.format;
        createInfo.imageColorSpace = vkFormat.colorSpace;
        createInfo.imageExtent = newExtent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = imageUsage;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.preTransform = preTransform;
        createInfo.compositeAlpha = compositeMode;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = oldSwapchain;

        VkResult  result = vkCreateSwapchainKHR(device->GetHandle(), &createInfo, nullptr, &handle);
        if (result != VK_SUCCESS)
        {
            VK_THROW(result, "Cannot create Swapchain");
            return false;
        }

        ALIMER_LOGDEBUG("[Vulkan]: Created SwapChain");

        if (oldSwapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(device->GetHandle(), oldSwapchain, nullptr);
        }

        backBufferIndex = 0;
        semaphoreIndex = 0;

        // Get SwapChain images
        uint32_t imageCount = 0;
        vkGetSwapchainImagesKHR(device->GetHandle(), handle, &imageCount, nullptr);

        std::vector<VkImage> images(imageCount);
        result = vkGetSwapchainImagesKHR(device->GetHandle(), handle, &imageCount, images.data());
        if (result != VK_SUCCESS)
        {
            VK_THROW(result, "[Vulkan]: Failed to retrive SwapChain-Images");
            return false;
        }

        const char* names[] =
        {
            "BackBuffer[0]",
            "BackBuffer[1]",
            "BackBuffer[2]",
            "BackBuffer[3]",
            "BackBuffer[4]",
        };

        for (uint32_t i = 0; i < imageCount; i++)
        {
            TextureDesc desc = {};
            desc.name = names[i];
            desc.type = TextureType::Type2D;
            desc.usage = TextureUsage::OutputAttachment;
            desc.format = PixelFormat::Bgra8Unorm;
            desc.extent.width = newExtent.width;
            desc.extent.height = newExtent.height;
            desc.extent.depth = 1;
            desc.mipLevels = 1;
            desc.sampleCount = TextureSampleCount::Count1;

            TextureVK* pTexture = new TextureVK(device);
            pTexture->InitExternal(images[i], &desc);
            buffers.emplace_back(pTexture);
        }

        return result == VK_SUCCESS;
    }

    void SwapChainVK::Destroy()
    {
        // Destroy swapchain
        if (handle != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(device->GetHandle(), handle, nullptr);
            handle = VK_NULL_HANDLE;
        }

        // Destroy surface
        if (surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(device->GetInstance(), surface, nullptr);
            surface = VK_NULL_HANDLE;
        }
    }

    IGraphicsDevice* SwapChainVK::GetDevice() const
    {
        return device;
    }

    ICommandQueue* SwapChainVK::GetCommandQueue() const
    {
        return commandQueue;
    }

    ITexture* SwapChainVK::GetNextTexture()
    {
        VkSemaphore aquiredSemaphore = device->RequestSemaphore();

        VkResult result = vkAcquireNextImageKHR(
            device->GetHandle(),
            handle,
            UINT64_MAX,
            aquiredSemaphore,
            VK_NULL_HANDLE,
            &backBufferIndex);

        if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            return nullptr;
        }

        commandQueue->AddWaitSemaphore(aquiredSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

        return buffers[backBufferIndex];
    }
}
