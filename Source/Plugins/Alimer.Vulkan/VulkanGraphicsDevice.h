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

#include "graphics/GraphicsDevice.h"
#include "VulkanBackend.h"

namespace alimer
{
    class VulkanGraphicsDevice final : public GraphicsDevice
    {
    public:
        /// Constructor.
        VulkanGraphicsDevice(bool validation);
        /// Destructor.
        ~VulkanGraphicsDevice() override;

        VkInstance GetInstance() const { return instance; }

    private:
        void InitCapabilities();

        GraphicsContext* CreateContext(const GraphicsContextDescription& desc) override;
        Texture* CreateTexture(const TextureDescription& desc, const void* initialData) override;

        struct {
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
        } features;

        VkInstance instance{ VK_NULL_HANDLE };
        VkDebugUtilsMessengerEXT debugUtilsMessenger{ VK_NULL_HANDLE };
    };

    class VulkanGraphicsDeviceFactory final : public GraphicsDeviceFactory
    {
    public:
        BackendType GetBackendType() const override { return BackendType::Vulkan; }
        GraphicsDevice* CreateDevice(bool validation) override;
    };
}
