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

#pragma once

#include "VulkanUtils.h"
#include <vector>

namespace alimer
{
    struct VulkanTexture;
    class VulkanGraphics;

    class VulkanSwapchain final
    {
    public:
        VulkanSwapchain(VulkanGraphics& graphics, VkSurfaceKHR surface, bool verticalSync);
        ~VulkanSwapchain();

        void CreateOrResize();

        VkResult AcquireNextImage(uint32_t& imageIndex, VkSemaphore imageAcquiredSemaphore, VkFence fence = VK_NULL_HANDLE);

        uint32_t GetImageCount() const
        {
            return imageCount;
        }

        PixelFormat GetColorFormat() const
        {
            return colorFormat;
        }

        VkSwapchainKHR GetHandle() const
        {
            return handle;
        }

    private:
        static constexpr uint32_t kImageCount = 3u;

        VulkanGraphics& graphics;
        VkSurfaceKHR surface;
        VkPresentModeKHR desiredPresentMode;
        VkSurfaceTransformFlagBitsKHR transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

        PixelFormat colorFormat{PixelFormat::Invalid};
        uint32_t imageCount{0u};
        VkSwapchainKHR handle{VK_NULL_HANDLE};

        // A list of present modes in order of priority (vector[0] has high priority, vector[size-1] has low priority)
        std::vector<VkPresentModeKHR> presentModePriorityList = {VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_MAILBOX_KHR};

        // A list of surface formats in order of priority (vector[0] has high priority, vector[size-1] has low priority)
        std::vector<VkSurfaceFormatKHR> surfaceFormatPriorityList = {
            {VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
            {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
            {VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
            {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}};

        std::vector<std::unique_ptr<VulkanTexture>> colorTextures{};
    };
}