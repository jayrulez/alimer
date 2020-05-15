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

#include "VulkanBackend.h"
#include "Containers/Array.h"

namespace alimer
{
    std::string to_string(VkResult result)
    {
        if (result == VK_SUCCESS)
        {
            return "VK_SUCCESS";
        }
        if (result == VK_NOT_READY)
        {
            return "VK_NOT_READY";
        }
        if (result == VK_TIMEOUT)
        {
            return "VK_TIMEOUT";
        }
        if (result == VK_EVENT_SET)
        {
            return "VK_EVENT_SET";
        }
        if (result == VK_EVENT_RESET)
        {
            return "VK_EVENT_RESET";
        }
        if (result == VK_INCOMPLETE)
        {
            return "VK_INCOMPLETE";
        }
        if (result == VK_ERROR_OUT_OF_HOST_MEMORY)
        {
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        }
        if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY)
        {
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        }
        if (result == VK_ERROR_INITIALIZATION_FAILED)
        {
            return "VK_ERROR_INITIALIZATION_FAILED";
        }
        if (result == VK_ERROR_DEVICE_LOST)
        {
            return "VK_ERROR_DEVICE_LOST";
        }
        if (result == VK_ERROR_MEMORY_MAP_FAILED)
        {
            return "VK_ERROR_MEMORY_MAP_FAILED";
        }
        if (result == VK_ERROR_LAYER_NOT_PRESENT)
        {
            return "VK_ERROR_LAYER_NOT_PRESENT";
        }
        if (result == VK_ERROR_EXTENSION_NOT_PRESENT)
        {
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        }
        if (result == VK_ERROR_FEATURE_NOT_PRESENT)
        {
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        }
        if (result == VK_ERROR_INCOMPATIBLE_DRIVER)
        {
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        }
        if (result == VK_ERROR_TOO_MANY_OBJECTS)
        {
            return "VK_ERROR_TOO_MANY_OBJECTS";
        }
        if (result == VK_ERROR_FORMAT_NOT_SUPPORTED)
        {
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        }
        if (result == VK_ERROR_FRAGMENTED_POOL)
        {
            return "VK_ERROR_FRAGMENTED_POOL";
        }
        if (result == VK_ERROR_OUT_OF_POOL_MEMORY)
        {
            return "VK_ERROR_OUT_OF_POOL_MEMORY";
        }
        if (result == VK_ERROR_INVALID_EXTERNAL_HANDLE)
        {
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        }
        if (result == VK_ERROR_SURFACE_LOST_KHR)
        {
            return "VK_ERROR_SURFACE_LOST_KHR";
        }
        if (result == VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)
        {
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        }
        if (result == VK_SUBOPTIMAL_KHR)
        {
            return "VK_SUBOPTIMAL_KHR";
        }
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            return "VK_ERROR_OUT_OF_DATE_KHR";
        }
        if (result == VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)
        {
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        }
        if (result == VK_ERROR_VALIDATION_FAILED_EXT)
        {
            return "VK_ERROR_VALIDATION_FAILED_EXT";
        }
        if (result == VK_ERROR_INVALID_SHADER_NV)
        {
            return "VK_ERROR_INVALID_SHADER_NV";
        }
        if (result == VK_ERROR_NOT_PERMITTED_EXT)
        {
            return "VK_ERROR_NOT_PERMITTED_EXT";
        }
        return "UNKNOWN";
    }

    uint32_t GetQueueFamilyIndex(VkQueueFlagBits queueFlags, const Vector<VkQueueFamilyProperties>& queueFamilyProperties)
    {
        // Dedicated queue for compute
        // Try to find a queue family index that supports compute but not graphics
        if (queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
            {
                if ((queueFamilyProperties[i].queueFlags & queueFlags)
                    && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
                {
                    return i;
                }
            }
        }

        // Dedicated queue for transfer
        // Try to find a queue family index that supports transfer but not graphics and compute
        if (queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
            {
                if ((queueFamilyProperties[i].queueFlags & queueFlags)
                    && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
                    && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
                {
                    return i;
                }
            }
        }

        // For other queue types or if no separate compute queue is present, return the first one to support the requested flags
        for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
        {
            if (queueFamilyProperties[i].queueFlags & queueFlags)
            {
                return i;
            }
        }

        return UINT32_MAX;
    }

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

        Vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

        QueueFamilyIndices indices = {};
        indices.graphicsFamily = GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT, queueFamilyProperties);
        indices.computeFamily = GetQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT, queueFamilyProperties);
        indices.transferFamily = GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT, queueFamilyProperties);

        for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
        {
            VkBool32 supportPresent = true;
            if (surface != VK_NULL_HANDLE)
            {
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportPresent);
            }
            else
            {

#if defined(VK_USE_PLATFORM_WIN32_KHR)
                supportPresent = vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, i);
#elif ALIMER_LINUX || ALIMER_OSX
                supportPresent = glfwGetPhysicalDevicePresentationSupport(instance, physicalDevice, i);
#endif
            }

            static const VkQueueFlags required = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT;
            if (supportPresent && ((queueFamilyProperties[i].queueFlags & required) == required))
            {
                indices.presentFamily = i;
                break;
            }
        }

        return indices;
    }
}
