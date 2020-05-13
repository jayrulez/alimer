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

#include "graphics/GraphicsDevice.h"
#include "VulkanBackend.h"

namespace alimer
{
    class VulkanGraphicsAdapter;

    struct QueueFamilyIndices
    {
        uint32_t graphicsFamily = VK_QUEUE_FAMILY_IGNORED;
        uint32_t computeFamily = VK_QUEUE_FAMILY_IGNORED;
        uint32_t transferFamily = VK_QUEUE_FAMILY_IGNORED;

        bool IsComplete()
        {
            return (graphicsFamily != VK_QUEUE_FAMILY_IGNORED) && (computeFamily != VK_QUEUE_FAMILY_IGNORED) && (transferFamily != VK_QUEUE_FAMILY_IGNORED);
        }
    };

    /// Vulkan GraphicsDevice.
    class ALIMER_API GraphicsDeviceVK final : public IGraphicsDevice
    {
    public:
        static bool IsAvailable();

        /// Constructor.
        GraphicsDeviceVK();
        /// Destructor.
        ~GraphicsDeviceVK() override;

        bool Init(const GraphicsDeviceDesc* pDesc);

        void Destroy();

        const VulkanDeviceFeatures& GetVulkanFeatures() const { return vk_features; }
        VkInstance GetInstance() const { return instance; }
        VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
        const QueueFamilyIndices& GetQueueFamilyIndices() const { return queueFamilyIndices; }
        VkDevice GetHandle() const { return handle; }
        VmaAllocator GetMemoryAllocator() const { return memoryAllocator; }
        VkQueue GetGraphicsQueue() const { return graphicsQueue; }

    private:
        bool InitInstance(const GraphicsDeviceDesc* pDesc);
        bool InitPhysicalDevice();
        bool InitLogicalDevice(const GraphicsDeviceDesc* pDesc);
        bool InitMemoryAllocator();

        RefPtr<ISwapChain> CreateSwapChain(window_t* window, const SwapChainDesc* pDesc) override;
        ITexture* CreateTexture(const TextureDesc* pDesc, const void* initialData) override;

        void WaitForIdle() override;

        VulkanDeviceFeatures vk_features{};
        VkInstance instance{ VK_NULL_HANDLE };
        VkDebugUtilsMessengerEXT debugUtilsMessenger{ VK_NULL_HANDLE };

        VkPhysicalDevice physicalDevice{ VK_NULL_HANDLE };
        QueueFamilyIndices queueFamilyIndices{};
        PhysicalDeviceExtensions physicalDeviceExts;
        VkPhysicalDeviceProperties physicalDeviceProperties{};

        VkDevice handle = VK_NULL_HANDLE;

        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue computeQueue = VK_NULL_HANDLE;
        VkQueue copyQueue = VK_NULL_HANDLE;

        VmaAllocator memoryAllocator{ VK_NULL_HANDLE };
    };
}
