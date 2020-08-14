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

#include "Graphics/GraphicsImpl.h"
#include "VulkanBackend.h"
#include <mutex>
#include <queue>
#include <unordered_map>

namespace alimer
{
    struct VulkanTexture
    {
        enum { MAX_COUNT = 4096 };

        VkImage handle;
        VmaAllocation memory;
    };

    struct VulkanBuffer
    {
        enum { MAX_COUNT = 4096 };

        VkBuffer handle;
        VmaAllocation memory;
    };

    class VulkanGraphicsImpl final : public GraphicsImpl
    {
    public:
        static bool IsAvailable();

        VulkanGraphicsImpl();
        ~VulkanGraphicsImpl() override;

        bool Initialize(WindowHandle windowHandle, uint32_t width, uint32_t height, bool isFullscreen) override;
        void WaitForGPU();
        bool BeginFrame() override;
        void EndFrame(uint64_t frameIndex) override;

        void SetVerticalSync(bool value) override;

        /* Resource creation methods */
        TextureHandle AllocTextureHandle();
        TextureHandle CreateTexture(TextureDimension dimension, uint32_t width, uint32_t height, const void* data, void* externalHandle) override;
        void Destroy(TextureHandle handle) override;
        void SetName(TextureHandle handle, const char* name) override;

        BufferHandle AllocBufferHandle();
        BufferHandle CreateBuffer(BufferUsage usage, uint32_t size, uint32_t stride, const void* data) override;
        void Destroy(BufferHandle handle) override;
        void SetName(BufferHandle handle, const char* name) override;

        /* Commands */
        void PushDebugGroup(const String& name, CommandList commandList) override;
        void PopDebugGroup(CommandList commandList) override;
        void InsertDebugMarker(const String& name, CommandList commandList) override;

        void BeginRenderPass(CommandList commandList, uint32_t numColorAttachments, const RenderPassColorAttachment* colorAttachments, const RenderPassDepthStencilAttachment* depthStencil) override;
        void EndRenderPass(CommandList commandList) override;

        void TextureBarrier(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

        void SetObjectName(VkObjectType type, uint64_t handle, const String& name);

        VkInstance GetVkInstance() const { return instance; }
        VkPhysicalDevice GetVkPhysicalDevice() const { return physicalDevice; }
        VkDevice GetVkDevice() const { return device; }

    private:
        void Shutdown();
        bool InitInstance();
        bool InitSurface(WindowHandle windowHandle);
        bool InitPhysicalDevice();
        bool InitLogicalDevice();
        void InitCapabilities();
        bool UpdateSwapchain();
        VkRenderPass GetRenderPass(uint32_t numColorAttachments, const RenderPassColorAttachment* colorAttachments, const RenderPassDepthStencilAttachment* depthStencil);
        VkFramebuffer GetFramebuffer(VkRenderPass renderPass, uint32_t numColorAttachments, const RenderPassColorAttachment* colorAttachments, const RenderPassDepthStencilAttachment* depthStencil);
        void ClearRenderPassCache();
        void ClearFramebufferCache();

        VkResult AcquireNextImage(uint32_t* imageIndex);
        VkResult PresentImage(uint32_t imageIndex);

        VulkanInstanceExtensions instanceExts{};
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
        VmaAllocator allocator{ VK_NULL_HANDLE };

        VkSwapchainKHR swapchain{ VK_NULL_HANDLE };
        uint32_t backbufferIndex = 0;

        std::vector<VkImageLayout> swapChainImageLayouts;
        std::vector<VkImage> swapChainImages;

        /// The image view for each swapchain image.
        std::vector<VkImageView> swapchainImageViews;

        /// The framebuffer for each swapchain image view.
        std::vector<VkFramebuffer> swapchain_framebuffers;

        /* Frame data */
        bool frameActive{ false };

        struct ResourceRelease
        {
            VkObjectType type;
            void* handle;
            VmaAllocation memory;
        };

        struct PerFrame
        {
            VkFence fence = VK_NULL_HANDLE;

            VkCommandPool primaryCommandPool = VK_NULL_HANDLE;
            VkCommandBuffer primaryCommandBuffer = VK_NULL_HANDLE;

            VkSemaphore swapchainAcquireSemaphore = VK_NULL_HANDLE;
            VkSemaphore swapchainReleaseSemaphore = VK_NULL_HANDLE;

            std::queue<ResourceRelease> deferredReleases;
        };

        /// A set of semaphores that can be reused.
        std::vector<VkSemaphore> recycledSemaphores;
        std::vector<PerFrame> frame;

        void TeardownPerFrame(PerFrame& frame);
        void Purge(PerFrame& frame);

        /* Resource pools */
        std::mutex handle_mutex;
        GPUResourcePool<VulkanTexture, VulkanTexture::MAX_COUNT> textures;
        GPUResourcePool<VulkanBuffer, VulkanBuffer::MAX_COUNT> buffers;

        std::unordered_map<Hash, VkRenderPass> renderPasses;
        std::unordered_map<Hash, VkFramebuffer> framebuffers;
    };
}
