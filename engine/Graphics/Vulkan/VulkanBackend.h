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

#include "Graphics/Graphics.h"
#include "VulkanUtils.h"
#include "vk_mem_alloc.h"
#include <array>

#define VK_DEFAULT_FENCE_TIMEOUT 100000000000

namespace alimer
{
    struct QueueFamilyIndices
    {
        uint32_t graphicsQueueFamilyIndex;
        uint32_t computeQueueFamily;
        uint32_t copyQueueFamily;
    };

    struct PhysicalDeviceExtensions
    {
        bool swapchain;
        bool depthClipEnable;
        bool maintenance_1;
        bool maintenance_2;
        bool maintenance_3;
        bool get_memory_requirements2;
        bool dedicated_allocation;
        bool bind_memory2;
        bool memory_budget;
        bool image_format_list;
        bool sampler_mirror_clamp_to_edge;
        bool win32_full_screen_exclusive;
        bool performance_query;
        bool host_query_reset;
    };

    class VulkanSwapchain;

    class VulkanGraphics final : public Graphics
    {
    public:
        VulkanGraphics(WindowHandle windowHandle, const GraphicsSettings& settings);
        ~VulkanGraphics() override;

        bool BeginFrame() override;
        void EndFrame() override;

        ImGuiRenderer* GetImGuiRenderer() override;

        VkInstance GetVkInstance() const
        {
            return instance;
        }

        VkPhysicalDevice GetVkPhysicalDevice() const
        {
            return physicalDevice;
        }

        VkDevice GetVkDevice() const
        {
            return device;
        }

        uint32_t GetGraphicsQueueFamilyIndex() const
        {
            return queueFamilies.graphicsQueueFamilyIndex;
        }

        VkQueue GetGraphicsQueue() const
        {
            return graphicsQueue;
        }

    private:
        struct InstanceFeatures
        {
            bool debugUtils = false;
            bool headless = false;
            bool getPhysicalDeviceProperties2 = false;
            bool getSurfaceCapabilities2 = false;
        } instanceFeatues;

        struct VulkanQueue
        {
            uint32_t familyIndex;
            VkQueue queue;
        };

        VkInstance instance{VK_NULL_HANDLE};
#if defined(_DEBUG)
        VkDebugUtilsMessengerEXT debug_utils_messenger{VK_NULL_HANDLE};
#endif
        VkSurfaceKHR surface{VK_NULL_HANDLE};

        VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
        VkPhysicalDeviceProperties2 physicalDeviceProperties = {};
        QueueFamilyIndices queueFamilies{};
        PhysicalDeviceExtensions physicalDeviceExtensions{};

        VkDevice device{VK_NULL_HANDLE};
        VkQueue graphicsQueue{VK_NULL_HANDLE};
        VkQueue computeQueue{VK_NULL_HANDLE};
        VkQueue copyQueue{VK_NULL_HANDLE};

        VmaAllocator memoryAllocator{VK_NULL_HANDLE};

        std::unique_ptr<VulkanSwapchain> swapchain{nullptr};

        /// Whether a frame is active or not
        bool frameActive{false};

        VkSemaphore acquiredSemaphore{VK_NULL_HANDLE};

        /// Current active frame index
        uint32_t activeFrameIndex{0};

        std::vector<std::unique_ptr<VulkanRenderFrame>> frames;

        ImGuiRenderer* imguiRenderer{nullptr};
    };
}