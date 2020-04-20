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

#include "VulkanGraphicsDevice.h"
#include "graphics/SwapChain.h"
#include "core/Utils.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "math/math.h"
#include <algorithm>
#include <vector>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

using namespace std;

namespace alimer
{
    bool VulkanGraphicsDevice::IsAvailable()
    {
        static bool availableInitialized = false;
        static bool available = false;
        if (availableInitialized) {
            return available;
        }

        availableInitialized = true;
        VkResult result = volkInitialize();
        if (result != VK_SUCCESS)
        {
            ALIMER_LOGW("Failed to initialize volk, vulkan backend is not available.");
            return false;
        }

        available = true;
        return available;
    }

    VulkanGraphicsDevice::VulkanGraphicsDevice(const GraphicsDeviceDescriptor& desc_)
        : GraphicsDevice(desc_)
    {
#if TODO_VK
        // Enumerate physical device and create logical one.
        {
            // Pick a suitable physical device based on user's preference.
            uint32_t best_device_score = 0;
            uint32_t best_device_index = -1;
            for (uint32_t i = 0; i < deviceCount; ++i)
            {
                VkPhysicalDeviceProperties deviceProps;
                vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProps);
                ALIMER_TRACE("Physical device %d:", i);
                ALIMER_TRACE("\t          Name: %s", deviceProps.deviceName);
                ALIMER_TRACE("\t   API version: %x", deviceProps.apiVersion);
                ALIMER_TRACE("\tDriver version: %x", deviceProps.driverVersion);
                ALIMER_TRACE("\t      VendorId: %x", deviceProps.vendorID);
                ALIMER_TRACE("\t      DeviceId: %x", deviceProps.deviceID);
                ALIMER_TRACE("\t          Type: %d", deviceProps.deviceType);

                uint32_t score = 0U;

                if (deviceProps.apiVersion >= VK_API_VERSION_1_2) {
                    score += 10000u;
                }
                else if (deviceProps.apiVersion >= VK_API_VERSION_1_1) {
                    score += 5000u;
                }

                switch (deviceProps.deviceType) {
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    score += 1000u;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    score += 100u;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    score += 80u;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    score += 70u;
                    break;
                default: score += 10u;
                }
                if (score > best_device_score) {
                    best_device_index = i;
                    best_device_score = score;
                }
            }
            if (best_device_index == -1) {
                return;
            }

            physical_device = physicalDevices[best_device_index];
            VkPhysicalDeviceProperties deviceProps;
            vkGetPhysicalDeviceProperties(physical_device, &deviceProps);

            uint32_t queue_count;
            vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_count, nullptr);
            vector<VkQueueFamilyProperties> queue_props(queue_count);
            vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_count, queue_props.data());

            for (uint32_t i = 0; i < queue_count; i++)
            {
                VkBool32 supportPresent = true;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
                supportPresent = vkGetPhysicalDeviceWin32PresentationSupportKHR(physical_device, i);
#elif ALIMER_LINUX || ALIMER_OSX
                supportPresent = glfwGetPhysicalDevicePresentationSupport(instance, physical_device, i);
#endif

                static const VkQueueFlags required = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT;
                if (supportPresent && ((queue_props[i].queueFlags & required) == required))
                {
                    graphicsQueueFamily = i;
                    break;
                }
            }

            for (uint32_t i = 0; i < queue_count; i++)
            {
                static const VkQueueFlags required = VK_QUEUE_COMPUTE_BIT;
                if (i != graphicsQueueFamily && (queue_props[i].queueFlags & required) == required)
                {
                    computeQueueFamily = i;
                    break;
                }
            }

            for (uint32_t i = 0; i < queue_count; i++)
            {
                static const VkQueueFlags required = VK_QUEUE_TRANSFER_BIT;
                if (i != graphicsQueueFamily
                    && i != computeQueueFamily
                    && (queue_props[i].queueFlags & required) == required)
                {
                    copyQueueFamily = i;
                    break;
                }
            }

            if (copyQueueFamily == VK_QUEUE_FAMILY_IGNORED)
            {
                for (uint32_t i = 0; i < queue_count; i++)
                {
                    static const VkQueueFlags required = VK_QUEUE_TRANSFER_BIT;
                    if (i != graphicsQueueFamily
                        && (queue_props[i].queueFlags & required) == required)
                    {
                        copyQueueFamily = i;
                        break;
                    }
                }
            }

            if (graphicsQueueFamily == VK_QUEUE_FAMILY_IGNORED)
                return;

            uint32_t universal_queue_index = 1;
            const uint32_t graphicsQueueIndex = 0;
            uint32_t compute_queue_index = 0;
            uint32_t transfer_queue_index = 0;

            if (computeQueueFamily == VK_QUEUE_FAMILY_IGNORED)
            {
                computeQueueFamily = graphicsQueueFamily;
                compute_queue_index = min(queue_props[graphicsQueueFamily].queueCount - 1, universal_queue_index);
                universal_queue_index++;
            }

            if (copyQueueFamily == VK_QUEUE_FAMILY_IGNORED)
            {
                copyQueueFamily = graphicsQueueFamily;
                transfer_queue_index = min(queue_props[graphicsQueueFamily].queueCount - 1, universal_queue_index);
                universal_queue_index++;
            }
            else if (copyQueueFamily == computeQueueFamily)
            {
                transfer_queue_index = min(queue_props[computeQueueFamily].queueCount - 1, 1u);
            }

            static const float graphicsQueuePrio = 0.5f;
            static const float computeQueuePrio = 1.0f;
            static const float transferQueuePrio = 1.0f;
            float prio[3] = { graphicsQueuePrio, computeQueuePrio, transferQueuePrio };

            unsigned queue_family_count = 0;
            VkDeviceQueueCreateInfo queueCreateInfo[3] = {};
            queueCreateInfo[queue_family_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo[queue_family_count].queueFamilyIndex = graphicsQueueFamily;
            queueCreateInfo[queue_family_count].queueCount = min(universal_queue_index, queue_props[graphicsQueueFamily].queueCount);
            queueCreateInfo[queue_family_count].pQueuePriorities = prio;
            queue_family_count++;

            if (computeQueueFamily != graphicsQueueFamily)
            {
                queueCreateInfo[queue_family_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo[queue_family_count].queueFamilyIndex = computeQueueFamily;
                queueCreateInfo[queue_family_count].queueCount = min(copyQueueFamily == computeQueueFamily ? 2u : 1u,
                    queue_props[computeQueueFamily].queueCount);
                queueCreateInfo[queue_family_count].pQueuePriorities = prio + 1;
                queue_family_count++;
            }

            if (copyQueueFamily != graphicsQueueFamily
                && copyQueueFamily != computeQueueFamily)
            {
                queueCreateInfo[queue_family_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo[queue_family_count].queueFamilyIndex = copyQueueFamily;
                queueCreateInfo[queue_family_count].queueCount = 1;
                queueCreateInfo[queue_family_count].pQueuePriorities = prio + 2;
                queue_family_count++;
            }

            /* Setup device extensions to enable. */
            uint32_t ext_count = 0;
            vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &ext_count, nullptr);
            vector<VkExtensionProperties> queried_extensions(ext_count);
            if (ext_count)
                vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &ext_count, queried_extensions.data());

            const auto hasDeviceExtension = [&](const char* name) -> bool {
                auto itr = find_if(begin(queried_extensions), end(queried_extensions), [name](const VkExtensionProperties& e) -> bool {
                    return strcmp(e.extensionName, name) == 0;
                    });
                return itr != end(queried_extensions);
            };

            vector<const char*> enabledDeviceExtensions;
            if (!vk_features.headless)
                enabledDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

            if (hasDeviceExtension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME))
            {
                vk_features.get_memory_requirements2 = true;
                enabledDeviceExtensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
            }

            if (vk_features.get_memory_requirements2
                && hasDeviceExtension(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME))
            {
                vk_features.dedicated = true;
                enabledDeviceExtensions.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
            }

            if (deviceProps.apiVersion >= VK_API_VERSION_1_1
                || hasDeviceExtension(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME))
            {
                vk_features.bind_memory2 = true;
                enabledDeviceExtensions.push_back(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME);
            }

            if (deviceProps.apiVersion >= VK_API_VERSION_1_1
                || hasDeviceExtension(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME))
            {
                vk_features.memory_budget = true;
                enabledDeviceExtensions.push_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
            }

            VkDeviceCreateInfo deviceCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
            deviceCreateInfo.queueCreateInfoCount = queue_family_count;
            deviceCreateInfo.pQueueCreateInfos = queueCreateInfo;
            deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledDeviceExtensions.size());
            deviceCreateInfo.ppEnabledExtensionNames = enabledDeviceExtensions.data();
            deviceCreateInfo.enabledLayerCount = 0;
            deviceCreateInfo.ppEnabledLayerNames = nullptr;

            for (auto* enabled_extension : enabledDeviceExtensions)
            {
                ALIMER_LOGI("Enabling device extension: %s.", enabled_extension);
            }

            if (vkCreateDevice(physical_device, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS)
            {
                return;
            }

            vkGetDeviceQueue(device, graphicsQueueFamily, graphicsQueueIndex, &graphicsQueue);
            vkGetDeviceQueue(device, computeQueueFamily, compute_queue_index, &computeQueue);
            vkGetDeviceQueue(device, copyQueueFamily, transfer_queue_index, &copyQueue);
        }

        // Create VMA allocator.
        {
            VmaVulkanFunctions vma_vulkan_func{};
            vma_vulkan_func.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
            vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
            vma_vulkan_func.vkAllocateMemory = vkAllocateMemory;
            vma_vulkan_func.vkFreeMemory = vkFreeMemory;
            vma_vulkan_func.vkMapMemory = vkMapMemory;
            vma_vulkan_func.vkUnmapMemory = vkUnmapMemory;
            vma_vulkan_func.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
            vma_vulkan_func.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
            vma_vulkan_func.vkBindBufferMemory = vkBindBufferMemory;
            vma_vulkan_func.vkBindImageMemory = vkBindImageMemory;
            vma_vulkan_func.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
            vma_vulkan_func.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
            vma_vulkan_func.vkCreateBuffer = vkCreateBuffer;
            vma_vulkan_func.vkDestroyBuffer = vkDestroyBuffer;
            vma_vulkan_func.vkCreateImage = vkCreateImage;
            vma_vulkan_func.vkDestroyImage = vkDestroyImage;
            vma_vulkan_func.vkCmdCopyBuffer = vkCmdCopyBuffer;

            VmaAllocatorCreateInfo allocator_info{};
            allocator_info.physicalDevice = physical_device;
            allocator_info.device = device;
            allocator_info.instance = instance;

            if (vk_features.get_memory_requirements2
                && vk_features.dedicated)
            {
                allocator_info.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
                vma_vulkan_func.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR;
                vma_vulkan_func.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR;
            }

            if (vk_features.bind_memory2)
            {
                allocator_info.flags |= VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;
                vma_vulkan_func.vkBindBufferMemory2KHR = vkBindBufferMemory2KHR;
                vma_vulkan_func.vkBindImageMemory2KHR = vkBindImageMemory2KHR;
            }

            if (vk_features.memory_budget)
            {
                allocator_info.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
                vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR;
            }

            allocator_info.pVulkanFunctions = &vma_vulkan_func;

            VkResult result = vmaCreateAllocator(&allocator_info, &memoryAllocator);

            if (result != VK_SUCCESS)
            {
                VK_THROW(result, "Cannot create allocator");
                return;
            }
        }
#endif // TODO_VK

    }

    VulkanGraphicsDevice::~VulkanGraphicsDevice()
    {
        WaitForIdle();
        Destroy();
    }

    void VulkanGraphicsDevice::Destroy()
    {
        if (instance == VK_NULL_HANDLE) {
            return;
        }

        if (memoryAllocator != VK_NULL_HANDLE)
        {
            VmaStats stats;
            vmaCalculateStats(memoryAllocator, &stats);

            ALIMER_LOGI("Total device memory leaked: {} bytes.", stats.total.usedBytes);

            vmaDestroyAllocator(memoryAllocator);
        }

        if (device != VK_NULL_HANDLE) {
            vkDestroyDevice(device, nullptr);
        }
    }

    void VulkanGraphicsDevice::WaitForIdle()
    {
        VK_CHECK(vkDeviceWaitIdle(device));
    }

    void VulkanGraphicsDevice::Present(const std::vector<Swapchain*>& swapchains)
    {
    }

    VkSurfaceKHR VulkanGraphicsDevice::CreateSurface(void* nativeWindowHandle, uint32_t* width, uint32_t* height)
    {
        VkResult result = VK_SUCCESS;
        VkSurfaceKHR surface = VK_NULL_HANDLE;

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
        instanceExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
        VkWin32SurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
        createInfo.hinstance = GetModuleHandle(NULL);
        createInfo.hwnd = reinterpret_cast<HWND>(nativeWindowHandle);
        result = vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface);

        if (result != VK_SUCCESS)
        {
            VK_THROW(result, "Failed to create surface");
        }

        RECT rect;
        BOOL success = GetClientRect(createInfo.hwnd, &rect);
        ALIMER_ASSERT_MSG(success, "GetWindowRect error.");
        *width = rect.right - rect.left;
        *height = rect.bottom - rect.top;
#endif

        return surface;
    }
}
