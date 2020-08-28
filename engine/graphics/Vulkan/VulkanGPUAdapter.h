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

#include "Graphics/GPUAdapter.h"
#include "VulkanBackend.h"

namespace Alimer
{
    class VulkanGPUAdapter final : public GPUAdapter
    {
    public:
        VulkanGPUAdapter(VkPhysicalDevice handle);

        /// Gets the GPU device identifier.
        ALIMER_FORCE_INLINE uint32 GetDeviceId() const override
        {
            return properties.deviceID;
        }

        /// Gets the GPU vendor identifier.
        ALIMER_FORCE_INLINE uint32 GetVendorId() const override
        {
            return properties.vendorID;
        }

        /// Gets the adapter name.
        ALIMER_FORCE_INLINE String GetName() const override
        {
            return name;
        }

        /// Gets the adapter type.
        ALIMER_FORCE_INLINE GPUAdapterType GetAdapterType() const override
        {
            return adapterType;
        }

        VkPhysicalDevice GetHandle() const { return handle; }
        const VkPhysicalDeviceFeatures& GetFeatures() const;
        const VkPhysicalDeviceProperties GetProperties() const;
        const VkPhysicalDeviceMemoryProperties GetMemoryProperties() const;
        const std::vector<VkQueueFamilyProperties>& GetQueueFamilyProperties() const;

    private:
        // Handle to the Vulkan physical device
        VkPhysicalDevice handle{ VK_NULL_HANDLE };

        // The features that this GPU supports
        VkPhysicalDeviceFeatures features{};

        // The GPU properties
        VkPhysicalDeviceProperties properties;

        // The GPU memory properties
        VkPhysicalDeviceMemoryProperties memoryProperties;

        // The GPU queue family properties
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;

        String name;
        GPUAdapterType adapterType;
    };
}
