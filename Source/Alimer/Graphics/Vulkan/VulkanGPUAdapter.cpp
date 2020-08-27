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

#include "VulkanGPUAdapter.h"

namespace Alimer
{
    VulkanGPUAdapter::VulkanGPUAdapter(VkPhysicalDevice handle)
        : handle{ handle }
    {
        vkGetPhysicalDeviceFeatures(handle, &features);
        vkGetPhysicalDeviceProperties(handle, &properties);
        vkGetPhysicalDeviceMemoryProperties(handle, &memoryProperties);

        //VkPhysicalDeviceProperties2 gpuProps = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
        //vkGetPhysicalDeviceProperties2(handle, &gpuProps);

        name = properties.deviceName;

        uint32_t queueFamilyPropertiesCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(handle, &queueFamilyPropertiesCount, nullptr);
        queueFamilyProperties.resize(queueFamilyPropertiesCount);
        vkGetPhysicalDeviceQueueFamilyProperties(handle, &queueFamilyPropertiesCount, queueFamilyProperties.data());

        switch (properties.deviceType)
        {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            adapterType = GPUAdapterType::IntegratedGPU;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            adapterType = GPUAdapterType::DiscreteGPU;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            adapterType = GPUAdapterType::CPU;
            break;
        default:
            adapterType = GPUAdapterType::Unknown;
            break;
        }
    }

    const VkPhysicalDeviceFeatures& VulkanGPUAdapter::GetFeatures() const
    {
        return features;
    }

    const VkPhysicalDeviceProperties VulkanGPUAdapter::GetProperties() const
    {
        return properties;
    }

    const VkPhysicalDeviceMemoryProperties VulkanGPUAdapter::GetMemoryProperties() const
    {
        return memoryProperties;
    }

    const std::vector<VkQueueFamilyProperties>& VulkanGPUAdapter::GetQueueFamilyProperties() const
    {
        return queueFamilyProperties;
    }
}
