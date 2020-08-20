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

#include "Graphics/GPUDevice.h"
#include "VulkanBackend.h"
#include "VulkanGPUAdapter.h"
#include <mutex>
#include <unordered_map>

namespace alimer
{
    class VulkanGPUContext;

    class VulkanGPUDevice final : public GPUDevice
    {
    public:
        static bool IsAvailable();

        VulkanGPUDevice(const String& appName, const GPUDeviceDescriptor& descriptor);
        ~VulkanGPUDevice() override;

        void WaitForGPU();

        VkSemaphore RequestSemaphore();
        void ReturnSemaphore(VkSemaphore semaphore);

        void SetObjectName(VkObjectType type, uint64_t handle, const String& name);
        VkRenderPass GetRenderPass(uint32_t numColorAttachments, const RenderPassColorAttachment* colorAttachments, const RenderPassDepthStencilAttachment* depthStencil);
        VkFramebuffer GetFramebuffer(VkRenderPass renderPass, uint32_t numColorAttachments, const RenderPassColorAttachment* colorAttachments, const RenderPassDepthStencilAttachment* depthStencil);

        GPUAdapter* GetAdapter() const override;
        GPUContext* GetMainContext() const override;
        VulkanGPUContext* GetVulkanMainContext() const;

        /* Resource creation methods */
        GPUContext* CreateContextCore(const GPUContextDescription& desc) override;

        VkInstance GetVkInstance() const { return instance; }
        const VulkanInstanceExtensions& GetInstanceExtensions() const { return instanceExts; }

        VkDevice GetHandle() const { return device; }
        VmaAllocator GetAllocator() const { return allocator; }
        VkPhysicalDevice GetVkPhysicalDevice() const { return adapter->GetHandle(); }
        const PhysicalDeviceExtensions& GetPhysicalDeviceExtensions() const { return physicalDeviceExts; }
        VkQueue GetGraphicsQueue() const { return graphicsQueue; }
        uint32 GetGraphicsQueueFamilyIndex() const { return queueFamilies.graphicsQueueFamilyIndex; }

    private:
        void Shutdown();
        VkSurfaceKHR CreateSurface(WindowHandle windowHandle);
        bool InitPhysicalDevice(GPUPowerPreference powerPreference, VkSurfaceKHR surface);
        bool InitLogicalDevice();
        void InitCapabilities();
       
        void ClearRenderPassCache();
        void ClearFramebufferCache();

        bool headless{ false };
        VulkanInstanceExtensions instanceExts{};
        VkInstance instance{ VK_NULL_HANDLE };

        /// Debug utils messenger callback for VK_EXT_Debug_Utils
        VkDebugUtilsMessengerEXT debugUtilsMessenger{ VK_NULL_HANDLE };
        
        VulkanGPUAdapter* adapter = nullptr;

        QueueFamilyIndices queueFamilies;
        PhysicalDeviceExtensions physicalDeviceExts;

        /* Device + queue's  */
        VkDevice device{ VK_NULL_HANDLE };
        VkQueue graphicsQueue{ VK_NULL_HANDLE };
        VkQueue computeQueue{ VK_NULL_HANDLE };
        VkQueue copyQueue{ VK_NULL_HANDLE };

        /* Memory allocator */
        VmaAllocator allocator{ VK_NULL_HANDLE };

        VulkanGPUContext* mainContext = nullptr;

        /// A set of semaphores that can be reused.
        std::vector<VkSemaphore> recycledSemaphores;

        std::unordered_map<Hash, VkRenderPass> renderPasses;
        std::unordered_map<Hash, VkFramebuffer> framebuffers;
    };
}
