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

#include "Graphics/GPUTexture.h"
#include "VulkanBackend.h"

namespace alimer
{
    class VulkanGPUSwapChain final 
    {
    public:
        VulkanGPUSwapChain(VulkanGPUDevice* device_, VkSurfaceKHR surface_, bool verticalSync_);
        ~VulkanGPUSwapChain();

        VkResult AcquireNextImage(VkSemaphore acquireSemaphore, uint32* imageIndex);
        VkResult Present(VkSemaphore semaphore, uint32 imageIndex);

        uint32 GetImageCount() const { return imageCount; }
        VkImage GetImage(uint32 index) const { return images[index]; }

    private:
        bool UpdateSwapchain();

        VulkanGPUDevice* device;

        VkQueue presentQueue;
        VkSurfaceKHR surface{ VK_NULL_HANDLE };
        VkSwapchainKHR handle{ VK_NULL_HANDLE };
        bool verticalSync;
        uint32 imageCount = 0;
        std::vector<VkImage> images;
    };
}
