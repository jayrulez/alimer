//
// Copyright (c) 2020 Amer Koleci and contributors.
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

#include "graphics/GraphicsProvider.h"
#include "VulkanBackend.h"

namespace alimer
{
    class ALIMER_API VulkanGraphicsProvider final : public GraphicsProvider
    {
    public:
        static bool IsAvailable();

        /// Constructor.
        VulkanGraphicsProvider(GraphicsProviderFlags flags);
        /// Destructor.
        ~VulkanGraphicsProvider() override;

        std::vector<std::unique_ptr<GraphicsAdapter>> EnumerateGraphicsAdapters() override;

        struct Features
        {
            /// Instance api version.
            uint32_t apiVersion;

            /// VK_KHR_get_physical_device_properties2
            bool physicalDeviceProperties2 = false;
            /// VK_KHR_external_memory_capabilities
            bool externalMemoryCapabilities = false;
            /// VK_KHR_external_semaphore_capabilities
            bool externalSemaphoreCapabilities = false;
            /// VK_EXT_debug_utils
            bool debugUtils = false;
            /// VK_EXT_headless_surface
            bool headless = false;
            /// VK_KHR_surface
            bool surface = false;
            /// VK_KHR_get_surface_capabilities2
            bool surfaceCapabilities2 = false;
        };

    private:
        VkInstance instance{ VK_NULL_HANDLE };
        VkDebugUtilsMessengerEXT debugUtilsMessenger{ VK_NULL_HANDLE };
        VkDebugReportCallbackEXT debugReportCallback{ VK_NULL_HANDLE };
        Features features{};
    };
}
