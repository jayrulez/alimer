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
#include "SyncPrimitivesPool.h"

namespace alimer
{
    class GraphicsContextVK;

    /// Vulkan GraphicsDevice.
    class ALIMER_API GraphicsDeviceVK final : public GraphicsDevice
    {
    public:
        static bool IsAvailable();

        /// Constructor.
        GraphicsDeviceVK();
        /// Destructor.
        ~GraphicsDeviceVK() override;

        bool Init(window_t* window, const GraphicsDeviceDesc& desc);
        void destroy();

        VkSurfaceKHR createSurface(uintptr_t handle);
        void SetObjectName(VkObjectType objectType, uint64_t objectHandle, const char* objectName);

        const VulkanDeviceFeatures& GetVulkanFeatures() const { return vk_features; }
        VkInstance GetInstance() const { return instance; }
        VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
        const QueueFamilyIndices& GetQueueFamilyIndices() const { return queueFamilyIndices; }
        VkDevice getHandle() const { return handle; }
        const VolkDeviceTable& getDeviceTable() const { return deviceTable; }
        VkQueue getGraphicsQueue() const { return graphicsQueue; }
        VkQueue getComputeQueue() const { return computeQueue; }
        VkQueue getCopyQueue() const { return copyQueue; }
        VmaAllocator getMemoryAllocator() const { return memoryAllocator; }
        GraphicsContext& getMainContext() const override;

        VkSemaphore RequestSemaphore();
        VkFence RequestFence();

    private:
        bool InitInstance(const GraphicsDeviceDesc& desc, bool headless);
        bool InitPhysicalDevice(VkSurfaceKHR surface);
        bool InitLogicalDevice(const GraphicsDeviceDesc& desc);
        bool InitMemoryAllocator();

        RefPtr<Texture> CreateTexture(const TextureDesc* pDesc, const void* initialData) override;
        void waitForIdle();

        VulkanDeviceFeatures vk_features{};
        VkInstance instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugUtilsMessenger{ VK_NULL_HANDLE };

        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        // The GPU properties
        VkPhysicalDeviceProperties physicalDeviceProperties{};
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;

        QueueFamilyIndices queueFamilyIndices{};
        PhysicalDeviceExtensions physicalDeviceExts;

        VkDevice handle = VK_NULL_HANDLE;
        VolkDeviceTable deviceTable = {};
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue computeQueue = VK_NULL_HANDLE;
        VkQueue copyQueue = VK_NULL_HANDLE;

        VmaAllocator memoryAllocator{ VK_NULL_HANDLE };

        std::unique_ptr<GraphicsContextVK> mainContext;

        struct Frame
        {
            explicit Frame(GraphicsDeviceVK* device);
            ~Frame();
            void operator=(const Frame&) = delete;
            Frame(const Frame&) = delete;

            void Begin();

            SyncPrimitivesPool syncPool;
            //std::unique_ptr<CommandPoolVK> commandPool;
        };

        std::vector<std::unique_ptr<Frame>> frames;
        uint32_t frameIndex = 0;
        uint32_t maxInflightFrames{ 3 };

        Frame& frame()
        {
            ALIMER_ASSERT(frameIndex < frames.size());
            ALIMER_ASSERT(frames[frameIndex]);
            return *frames[frameIndex];
        }

        const Frame& frame() const
        {
            ALIMER_ASSERT(frameIndex < frames.size());
            ALIMER_ASSERT(frames[frameIndex]);
            return *frames[frameIndex];
        }
    };
}
