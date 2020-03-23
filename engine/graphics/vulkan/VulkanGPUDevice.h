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

#include "graphics/GPUDevice.h"
#include "VulkanBackend.h"

namespace alimer
{
    /// Vulkan GPU backend.
    class ALIMER_API VulkanGPUDevice final : public GPUDevice
    {
    public:
        static bool isAvailable();

        /// Constructor.
        VulkanGPUDevice(bool validation, bool headless);
        /// Destructor.
        ~VulkanGPUDevice() override;

        void Destroy();
        void WaitIdle() override;
        //bool BeginFrame() override;
        //void EndFrame() override;

        const VulkanDeviceFeatures& GetVulkanFeatures() const { return vk_features; }
        VkInstance GetInstance() const { return instance; }
        VkPhysicalDevice GetPhysicalDevice() const { return physical_device; }
        VkDevice GetDevice() const { return device; }
        VmaAllocator GetMemoryAllocator() const { return memoryAllocator; }

    private:
        VkSurfaceKHR createSurface(void* nativeWindowHandle, uint32_t* width, uint32_t* height);
        SharedPtr<Texture> CreateTexture() override;
        //std::shared_ptr<Framebuffer> createFramebufferCore(const SwapChainDescriptor* descriptor) override;

    private:
        VulkanDeviceFeatures vk_features{};
        VkInstance instance{ VK_NULL_HANDLE };
        VkDebugUtilsMessengerEXT debug_messenger{ VK_NULL_HANDLE };
        VkPhysicalDevice physical_device{ VK_NULL_HANDLE };

        VkDevice device{ VK_NULL_HANDLE };

        uint32_t graphicsQueueFamily = VK_QUEUE_FAMILY_IGNORED;
        uint32_t computeQueueFamily = VK_QUEUE_FAMILY_IGNORED;
        uint32_t copyQueueFamily = VK_QUEUE_FAMILY_IGNORED;
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue computeQueue = VK_NULL_HANDLE;
        VkQueue copyQueue = VK_NULL_HANDLE;

        VmaAllocator memoryAllocator{ VK_NULL_HANDLE };
    };
}
