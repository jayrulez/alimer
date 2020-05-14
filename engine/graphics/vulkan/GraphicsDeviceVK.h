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
#include "CommandQueueVK.h"
#include "SyncPrimitivesPool.h"

namespace alimer
{
    class CommandPoolVK;

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

        void SetObjectName(VkObjectType objectType, uint64_t objectHandle, const char* objectName);

        const VulkanDeviceFeatures& GetVulkanFeatures() const { return vk_features; }
        VkInstance GetInstance() const { return instance; }
        VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
        const QueueFamilyIndices& GetQueueFamilyIndices() const { return queueFamilyIndices; }
        VkDevice GetHandle() const { return handle; }
        VmaAllocator GetMemoryAllocator() const { return memoryAllocator; }

        ICommandBuffer& RequestCommandBuffer(CommandQueueType queueType);
        VkSemaphore RequestSemaphore();
        VkFence RequestFence();

    private:
        bool InitInstance(const GraphicsDeviceDesc* pDesc);
        bool InitPhysicalDevice();
        bool InitLogicalDevice(const GraphicsDeviceDesc* pDesc);
        bool InitMemoryAllocator();

        ICommandQueue* GetGraphicsQueue() const override {
            return graphicsQueue;
        }

        ICommandQueue* GetComputeQueue() const override {
            return computeQueue;
        }

        ICommandQueue* GetCopyQueue() const override {
            return copyQueue;
        }

        CommandQueueVK* CreateCommandQueue(const char* name, CommandQueueType type);
        RefPtr<ISwapChain> CreateSwapChain(window_t* window, ICommandQueue* commandQueue, const SwapChainDesc* pDesc) override;
        RefPtr<ITexture> CreateTexture(const TextureDesc* pDesc, const void* initialData) override;

        void WaitForIdle() override;
        bool BeginFrame() override;
        void EndFrame() override;

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
        VmaAllocator memoryAllocator{ VK_NULL_HANDLE };


        uint32_t nextGraphicsQueue = 0;
        uint32_t nextComputeQueue = 0;
        uint32_t nextTransferQueue = 0;
        CommandQueueVK* graphicsQueue = nullptr;
        CommandQueueVK* computeQueue = nullptr;
        CommandQueueVK* copyQueue = nullptr;

        struct Frame
        {
            explicit Frame(GraphicsDeviceVK* device);
            ~Frame();
            void operator=(const Frame&) = delete;
            Frame(const Frame&) = delete;

            void Begin();

            SyncPrimitivesPool syncPool;
            std::unique_ptr<CommandPoolVK> commandPool;
        };

        Vector<std::unique_ptr<Frame>> frames;
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
