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
#include <mutex>

namespace Alimer
{
    class VulkanGraphicsDevice final : public GraphicsDevice
    {
    public:
        static bool IsAvailable();

        VulkanGraphicsDevice(const GraphicsDeviceDescription& desc);
        ~VulkanGraphicsDevice() override;

        void WaitForGPU() override;
        bool BeginFrame() override;
        void EndFrame(const std::vector<SwapChain*>& swapChains) override;

        void Submit(VkCommandBuffer buffer);
        VkFence RequestFence();
        VkSemaphore RequestSemaphore();
        void SetObjectName(VkObjectType type, uint64_t handle, const std::string& name);
        //VkRenderPass GetRenderPass(uint32_t numColorAttachments, const RenderPassColorAttachment* colorAttachments, const RenderPassDepthStencilAttachment* depthStencil);
        //VkFramebuffer GetFramebuffer(VkRenderPass renderPass, uint32_t numColorAttachments, const RenderPassColorAttachment* colorAttachments, const RenderPassDepthStencilAttachment* depthStencil);

        VkInstance GetVkInstance() const { return instance; }
        const VulkanInstanceExtensions& GetInstanceExtensions() const { return instanceExts; }

        VkDevice GetHandle() const { return device; }
        VmaAllocator GetAllocator() const { return allocator; }
        VkPhysicalDevice GetVkPhysicalDevice() const { return physicalDevice; }
        const PhysicalDeviceExtensions& GetPhysicalDeviceExtensions() const { return physicalDeviceExts; }
        VkQueue GetGraphicsQueue() const { return graphicsQueue; }
        uint32 GetGraphicsQueueFamilyIndex() const { return queueFamilies.graphicsQueueFamilyIndex; }

    private:
        void Shutdown();
        VkSurfaceKHR CreateSurface(void* windowHandle);
        bool InitInstance(const std::string& applicationName, bool headless);
        bool InitPhysicalDevice(VkSurfaceKHR surface);
        bool InitLogicalDevice();
        void InitCapabilities();
        VulkanRenderFrame& GetActiveFrame();
        void WaitFrame();

        RefPtr<GPUBuffer> CreateBufferCore(const BufferDescription& desc, const void* initialData) override;

        /* CommandList */
        CommandList BeginCommandList() override;
        void PushDebugGroup(CommandList commandList, const char* name) override;
        void PopDebugGroup(CommandList commandList) override;
        void InsertDebugMarker(CommandList commandList, const char* name) override;

        void BeginRenderPass(CommandList commandList, const RenderPassDescription* renderPass) override;
        void EndRenderPass(CommandList commandList) override;

        void SetScissorRect(CommandList commandList, const RectI& scissorRect) override;
        void SetScissorRects(CommandList commandList, const RectI* scissorRects, uint32_t count) override;
        void SetViewport(CommandList commandList, const Viewport& viewport) override;
        void SetViewports(CommandList commandList, const Viewport* viewports, uint32_t count) override;
        void SetBlendColor(CommandList commandList, const Color& color) override;

        void ClearRenderPassCache();
        void ClearFramebufferCache();

        bool headless{ false };
        VulkanInstanceExtensions instanceExts{};
        VkInstance instance{ VK_NULL_HANDLE };

        /// Debug utils messenger callback for VK_EXT_Debug_Utils
        VkDebugUtilsMessengerEXT debugUtilsMessenger{ VK_NULL_HANDLE };

        VkPhysicalDevice physicalDevice{ VK_NULL_HANDLE };
        VkPhysicalDeviceFeatures physicalDeviceFeatures{};
        VkPhysicalDeviceProperties physicalDeviceProperties{};
        VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties{};
        QueueFamilyIndices queueFamilies;
        PhysicalDeviceExtensions physicalDeviceExts;

        /* Device + queue's  */
        VkDevice device{ VK_NULL_HANDLE };
        VkQueue graphicsQueue{ VK_NULL_HANDLE };
        VkQueue computeQueue{ VK_NULL_HANDLE };
        VkQueue copyQueue{ VK_NULL_HANDLE };

        /* Memory allocator */
        VmaAllocator allocator{ VK_NULL_HANDLE };

        /// Current active frame index
        uint32_t activeFrameIndex{ 0 };

        /// Whether a frame is active or not
        bool frameActive{ false };

        std::vector<std::unique_ptr<VulkanRenderFrame>> frames;

        std::unordered_map<Hash, VkRenderPass> renderPasses;
        std::unordered_map<Hash, VkFramebuffer> framebuffers;
    };
}
