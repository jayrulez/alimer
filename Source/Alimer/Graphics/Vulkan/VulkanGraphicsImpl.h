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

#include "Graphics/GraphicsDevice.h"
#include "VulkanBackend.h"

namespace alimer
{
    class VulkanTexture;

    class VulkanGraphicsImpl final : public GraphicsDevice
    {
    public:
        static bool IsAvailable();

        VulkanGraphicsImpl(const std::string& applicationName, GPUFlags flags);
        ~VulkanGraphicsImpl() override;

        void SetObjectName(VkObjectType type, uint64_t handle, const std::string& name);

        VkInstance GetVkInstance() const { return instance; }
        VkPhysicalDevice GetVkPhysicalDevice() const { return physicalDevice; }
        VkDevice GetVkDevice() const { return device; }
        VmaAllocator GetMemoryAllocator() const { return memoryAllocator; }

    private:
        void Shutdown();
        void InitCapabilities();
        bool UpdateSwapchain();

        void WaitForGPU() override;
        bool BeginFrame() override;
        void EndFrame() override;
        Texture* GetBackbufferTexture() const override;
        CommandBuffer& BeginCommandBuffer(const std::string_view id) override;

        InstanceExtensions instanceExts{};
        VkInstance instance{ VK_NULL_HANDLE };

        /// Debug utils messenger callback for VK_EXT_Debug_Utils
        VkDebugUtilsMessengerEXT debugUtilsMessenger{ VK_NULL_HANDLE };

        VkSurfaceKHR surface{ VK_NULL_HANDLE };

        VkPhysicalDevice physicalDevice{ VK_NULL_HANDLE };
        VkPhysicalDeviceProperties2 physicalDeviceProperties{};
        QueueFamilyIndices queueFamilies;
        PhysicalDeviceExtensions physicalDeviceExts;

        /* Device + queue's  */
        VkDevice device{ VK_NULL_HANDLE };
        VkQueue graphicsQueue{ VK_NULL_HANDLE };
        VkQueue computeQueue{ VK_NULL_HANDLE };
        VkQueue copyQueue{ VK_NULL_HANDLE };

        /* Memory allocator */
        VmaAllocator memoryAllocator{ VK_NULL_HANDLE };

        VkSwapchainKHR swapchain{ VK_NULL_HANDLE };
        uint32_t backbufferIndex{ 0 };
        SharedPtr<VulkanTexture> backbufferTextures[kInflightFrameCount] = {};

        /// A set of semaphores that can be reused.
        std::vector<VkSemaphore> recycledSemaphores;

        /* Frame data */
        u32 maxInflightFrames = 3u;

        bool frameActive{ false };

        struct PerFrame
        {
            VkFence queueSubmitFence = VK_NULL_HANDLE;
            VkCommandPool primaryCommandPool = VK_NULL_HANDLE;
            VkCommandBuffer primaryCommandBuffer = VK_NULL_HANDLE;

            VkSemaphore swapchainAcquireSemaphore = VK_NULL_HANDLE;
            VkSemaphore swapchainReleaseSemaphore = VK_NULL_HANDLE;
        };
        std::vector<PerFrame> perFrame;
    };
}
