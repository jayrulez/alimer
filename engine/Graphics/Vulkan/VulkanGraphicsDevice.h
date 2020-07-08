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
    class VulkanGraphicsDevice final : public GraphicsDevice
    {
    public:
        static bool IsAvailable();

        VulkanGraphicsDevice();
        ~VulkanGraphicsDevice() override;

        const VolkDeviceTable& GetDeviceTable() const { return deviceTable; }

    private:
        //bool BackendInitialize(const PresentationParameters& presentationParameters) override;
        void BackendShutdown() override;
        void WaitForGPU() override;
        bool BeginFrameImpl() override;
        void EndFrameImpl() override;

        // Resource creation methods.
        RefPtr<SwapChain> CreateSwapChain(const SwapChainDescription& desc) override;
        RefPtr<Texture> CreateTexture(const TextureDescription& desc, const void* initialData) override;

        CommandList BeginCommandList(const char* name) override;
        void InsertDebugMarker(CommandList commandList, const char* name) override;
        void PushDebugGroup(CommandList commandList, const char* namet) override;
        void PopDebugGroup(CommandList commandList) override;

        void SetScissorRect(CommandList commandList, const Rect& scissorRect) override;
        void SetScissorRects(CommandList commandList, const Rect* scissorRects, uint32_t count) override;
        void SetViewport(CommandList commandList, const Viewport& viewport) override;
        void SetViewports(CommandList commandList, const Viewport* viewports, uint32_t count) override;
        void SetBlendColor(CommandList commandList, const Color& color) override;

        InstanceExtensions instanceExts{};
        VkInstance instance{ VK_NULL_HANDLE };

        /// Debug utils messenger callback for VK_EXT_Debug_Utils
        VkDebugUtilsMessengerEXT debugUtilsMessenger{ VK_NULL_HANDLE };

        VkPhysicalDevice physicalDevice{ VK_NULL_HANDLE };
        VkPhysicalDeviceProperties2 physicalDeviceProperties{};
        QueueFamilyIndices queueFamilies;
        PhysicalDeviceExtensions physicalDeviceExts;

        /* Device + queue's  */
        VkDevice device{ VK_NULL_HANDLE };
        VolkDeviceTable deviceTable = {};
        VkQueue graphicsQueue{ VK_NULL_HANDLE };
        VkQueue computeQueue{ VK_NULL_HANDLE };
        VkQueue copyQueue{ VK_NULL_HANDLE };

        /* Memory allocator */
        VmaAllocator allocator{ VK_NULL_HANDLE };

        VkCommandBuffer commandBuffers[kMaxCommandLists] = {};
    };
}
