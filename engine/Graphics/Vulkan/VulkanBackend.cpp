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

#include "VulkanBackend.h"
#include "AlimerConfig.h"
#include "Platform/Window.h"
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

namespace alimer
{
    const std::string ToString(VkResult result)
    {
        switch (result)
        {
#define STR(r)   \
    case VK_##r: \
        return #r
            STR(NOT_READY);
            STR(TIMEOUT);
            STR(EVENT_SET);
            STR(EVENT_RESET);
            STR(INCOMPLETE);
            STR(ERROR_OUT_OF_HOST_MEMORY);
            STR(ERROR_OUT_OF_DEVICE_MEMORY);
            STR(ERROR_INITIALIZATION_FAILED);
            STR(ERROR_DEVICE_LOST);
            STR(ERROR_MEMORY_MAP_FAILED);
            STR(ERROR_LAYER_NOT_PRESENT);
            STR(ERROR_EXTENSION_NOT_PRESENT);
            STR(ERROR_FEATURE_NOT_PRESENT);
            STR(ERROR_INCOMPATIBLE_DRIVER);
            STR(ERROR_TOO_MANY_OBJECTS);
            STR(ERROR_FORMAT_NOT_SUPPORTED);
            STR(ERROR_SURFACE_LOST_KHR);
            STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
            STR(SUBOPTIMAL_KHR);
            STR(ERROR_OUT_OF_DATE_KHR);
            STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
            STR(ERROR_VALIDATION_FAILED_EXT);
            STR(ERROR_INVALID_SHADER_NV);
#undef STR
            default:
                return "UNKNOWN_ERROR";
        }
    }

    namespace
    {
#if defined(_DEBUG)
        VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
                                                                      VkDebugUtilsMessageTypeFlagsEXT             message_type,
                                                                      const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                                                                      void*                                       user_data)
        {
            // Log debug messge
            if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            {
                LOGW("{} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
            }
            else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            {
                LOGE("{} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
            }

            return VK_FALSE;
        }

        bool validate_layers(const std::vector<const char*>& required, const std::vector<VkLayerProperties>& available)
        {
            for (auto layer : required)
            {
                bool found = false;
                for (auto& available_layer : available)
                {
                    if (strcmp(available_layer.layerName, layer) == 0)
                    {
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    LOGE("Validation Layer '%s' not found", layer);
                    return false;
                }
            }

            return true;
        }

        std::vector<const char*> get_optimal_validation_layers(const std::vector<VkLayerProperties>& supportedInstanceLayers)
        {
            std::vector<std::vector<const char*>> validation_layer_priority_list = {
                // The preferred validation layer is "VK_LAYER_KHRONOS_validation"
                {"VK_LAYER_KHRONOS_validation"},

                // Otherwise we fallback to using the LunarG meta layer
                {"VK_LAYER_LUNARG_standard_validation"},

                // Otherwise we attempt to enable the individual layers that compose the LunarG meta layer since it doesn't exist
                {
                    "VK_LAYER_GOOGLE_threading",
                    "VK_LAYER_LUNARG_parameter_validation",
                    "VK_LAYER_LUNARG_object_tracker",
                    "VK_LAYER_LUNARG_core_validation",
                    "VK_LAYER_GOOGLE_unique_objects",
                },

                // Otherwise as a last resort we fallback to attempting to enable the LunarG core layer
                {"VK_LAYER_LUNARG_core_validation"}};

            for (auto& validation_layers : validation_layer_priority_list)
            {
                if (validate_layers(validation_layers, supportedInstanceLayers))
                {
                    return validation_layers;
                }

                LOGW("Couldn't enable validation layers (see log for error) - falling back");
            }

            // Else return nothing
            return {};
        }
#endif
    }

    VulkanGraphics::VulkanGraphics(Window& window, const GraphicsSettings& settings) : Graphics(window, settings)
    {
        VkResult result = volkInitialize();
        if (result)
        {
            VK_THROW(result, "Failed to initialize volk.");
        }

        // Create instance first.
        {
            const bool               headless = false;
            std::vector<const char*> enabledInstanceLayers;
            std::vector<const char*> enabledInstanceExtensions;

#if defined(_DEBUG)
            if (settings.enableDebugLayer)
            {
                uint32_t instance_layer_count;
                VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr));

                std::vector<VkLayerProperties> supportedInstanceLayers(instance_layer_count);
                VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, supportedInstanceLayers.data()));

                std::vector<const char*> optimal_validation_layers = get_optimal_validation_layers(supportedInstanceLayers);
                enabledInstanceLayers.insert(enabledInstanceLayers.end(), optimal_validation_layers.begin(),
                                             optimal_validation_layers.end());
            }
#endif

            uint32_t instanceExtensionCount;
            VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr));

            std::vector<VkExtensionProperties> availableInstanceExtensions(instanceExtensionCount);
            VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, availableInstanceExtensions.data()));

            // Check if VK_EXT_debug_utils is supported, which supersedes VK_EXT_Debug_Report
            for (auto& availableExtension : availableInstanceExtensions)
            {
                if (strcmp(availableExtension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
                {
                    instanceFeatues.debugUtils = true;
                }
                else if (strcmp(availableExtension.extensionName, VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME) == 0)
                {
                    instanceFeatues.headless = true;
                }
                else if (strcmp(availableExtension.extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0)
                {
                    // VK_KHR_get_physical_device_properties2 is a prerequisite of VK_KHR_performance_query
                    // which will be used for stats gathering where available.
                    instanceFeatues.getPhysicalDeviceProperties2 = true;
                    enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
                }
                else if (strcmp(availableExtension.extensionName, VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME) == 0)
                {
                    instanceFeatues.getSurfaceCapabilities2 = true;
                }
            }

            if (headless)
            {
                if (instanceFeatues.headless)
                {
                    enabledInstanceExtensions.push_back(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
                }
                else
                {
                    LOGW("'%s' is not available, disabling swapchain creation", VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
                }
            }
            else
            {
                enabledInstanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
                enabledInstanceExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
                enabledInstanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
                enabledInstanceExtensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
                enabledInstanceExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)
                enabledInstanceExtensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
                enabledInstanceExtensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#else
#    pragma error Platform not supported
#endif

                if (instanceFeatues.getSurfaceCapabilities2)
                {
                    enabledInstanceExtensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
                }
            }

#if defined(_DEBUG)
            if (settings.enableDebugLayer && instanceFeatues.debugUtils)
            {
                enabledInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
#endif

            VkApplicationInfo app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO};
            app_info.pApplicationName   = settings.applicationName.c_str();
            app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            app_info.pEngineName        = "Alimer Engine";
            app_info.engineVersion      = VK_MAKE_VERSION(ALIMER_VERSION_MAJOR, ALIMER_VERSION_MINOR, ALIMER_VERSION_PATCH);
            app_info.apiVersion         = volkGetInstanceVersion();

            VkInstanceCreateInfo instance_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
            instance_info.pApplicationInfo        = &app_info;
            instance_info.enabledLayerCount       = static_cast<uint32_t>(enabledInstanceLayers.size());
            instance_info.ppEnabledLayerNames     = enabledInstanceLayers.data();
            instance_info.enabledExtensionCount   = static_cast<uint32_t>(enabledInstanceExtensions.size());
            instance_info.ppEnabledExtensionNames = enabledInstanceExtensions.data();

#if defined(_DEBUG)
            VkDebugUtilsMessengerCreateInfoEXT debug_utils_create_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
            if (settings.enableDebugLayer && instanceFeatues.debugUtils)
            {
                debug_utils_create_info.messageSeverity =
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
                debug_utils_create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
                debug_utils_create_info.pfnUserCallback = debug_utils_messenger_callback;

                instance_info.pNext = &debug_utils_create_info;
            }
#endif

            // Create the Vulkan instance
            result = vkCreateInstance(&instance_info, nullptr, &instance);

            if (result != VK_SUCCESS)
            {
                VK_THROW(result, "Could not create Vulkan instance");
            }

            volkLoadInstance(instance);

#if defined(_DEBUG)
            if (settings.enableDebugLayer && instanceFeatues.debugUtils)
            {
                result = vkCreateDebugUtilsMessengerEXT(instance, &debug_utils_create_info, nullptr, &debug_utils_messenger);
                if (result != VK_SUCCESS)
                {
                    VK_THROW(result, "Could not create debug utils messenger");
                }
            }
#endif

            LOGI("Created VkInstance with version: {}.{}.{}", VK_VERSION_MAJOR(app_info.apiVersion), VK_VERSION_MINOR(app_info.apiVersion),
                 VK_VERSION_PATCH(app_info.apiVersion));
            if (instance_info.enabledLayerCount)
            {
                for (uint32_t i = 0; i < instance_info.enabledLayerCount; ++i)
                {
                    LOGI("Instance layer '{}'", instance_info.ppEnabledLayerNames[i]);
                }
            }

            for (uint32_t i = 0; i < instance_info.enabledExtensionCount; ++i)
            {
                LOGI("Instance extension '{}'", instance_info.ppEnabledExtensionNames[i]);
            }
        }

        // Create surface
        {
#ifdef VK_USE_PLATFORM_ANDROID_KHR
#elif VK_USE_PLATFORM_WIN32_KHR
            VkWin32SurfaceCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
            createInfo.hinstance = GetModuleHandle(nullptr);
            createInfo.hwnd = window.GetHandle();

            VkResult result = vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface);
#elif VK_USE_PLATFORM_MACOS_MVK
#endif

            if (result != VK_SUCCESS)
            {
                VK_THROW(result, "Could not create surface");
            }
        }

        // Querying valid physical devices on the machine and choose the best supported
        {
            uint32_t physical_device_count{0};
            VK_CHECK(vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr));

            if (physical_device_count < 1)
            {
                throw std::runtime_error("Couldn't find a physical device that supports Vulkan.");
            }

            std::vector<VkPhysicalDevice> physical_devices;
            physical_devices.resize(physical_device_count);
            VK_CHECK(vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices.data()));

            physical_device = physical_devices[0];
        }

        // Create device
        {
            // Prepare the device queues
            uint32_t queue_family_properties_count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_properties_count, nullptr);
            std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_properties_count);
            vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_properties_count, queue_family_properties.data());

            std::vector<VkDeviceQueueCreateInfo> queue_create_infos(queue_family_properties_count,
                                                                    {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO});
            std::vector<std::vector<float>>      queue_priorities(queue_family_properties_count);

            for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_properties_count; ++queue_family_index)
            {
                const VkQueueFamilyProperties& queue_family_property = queue_family_properties[queue_family_index];

                queue_priorities[queue_family_index].resize(queue_family_property.queueCount, 1.0f);

                VkDeviceQueueCreateInfo& queue_create_info = queue_create_infos[queue_family_index];

                queue_create_info.queueFamilyIndex = queue_family_index;
                queue_create_info.queueCount       = queue_family_property.queueCount;
                queue_create_info.pQueuePriorities = queue_priorities[queue_family_index].data();
            }

            // Query physical device extensions
            uint32_t device_extension_count;
            VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &device_extension_count, nullptr));
            std::vector<VkExtensionProperties> device_extensions(device_extension_count);
            VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &device_extension_count, device_extensions.data()));

            // Display supported extensions
            if (device_extensions.size() > 0)
            {
                LOGD("Device supports the following extensions:");
                for (auto& extension : device_extensions)
                {
                    LOGD("  \t{}", extension.extensionName);
                }
            }

            std::vector<const char*> enabled_extensions;

            VkDeviceCreateInfo create_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
            create_info.pNext                   = nullptr;
            create_info.pQueueCreateInfos       = queue_create_infos.data();
            create_info.queueCreateInfoCount    = static_cast<uint32_t>(queue_create_infos.size());
            create_info.enabledExtensionCount   = static_cast<uint32_t>(enabled_extensions.size());
            create_info.ppEnabledExtensionNames = enabled_extensions.data();

            VkPhysicalDeviceFeatures requested_features{};
            create_info.pEnabledFeatures = &requested_features;

            VkResult result = vkCreateDevice(physical_device, &create_info, nullptr, &device);

            if (result != VK_SUCCESS)
            {
                VK_THROW(result, "Cannot create device");
            }
        }

        // Create memory allocator (VMA)
        {
            VmaVulkanFunctions vma_vulkan_func{};
            vma_vulkan_func.vkAllocateMemory                    = vkAllocateMemory;
            vma_vulkan_func.vkBindBufferMemory                  = vkBindBufferMemory;
            vma_vulkan_func.vkBindImageMemory                   = vkBindImageMemory;
            vma_vulkan_func.vkCreateBuffer                      = vkCreateBuffer;
            vma_vulkan_func.vkCreateImage                       = vkCreateImage;
            vma_vulkan_func.vkDestroyBuffer                     = vkDestroyBuffer;
            vma_vulkan_func.vkDestroyImage                      = vkDestroyImage;
            vma_vulkan_func.vkFlushMappedMemoryRanges           = vkFlushMappedMemoryRanges;
            vma_vulkan_func.vkFreeMemory                        = vkFreeMemory;
            vma_vulkan_func.vkGetBufferMemoryRequirements       = vkGetBufferMemoryRequirements;
            vma_vulkan_func.vkGetImageMemoryRequirements        = vkGetImageMemoryRequirements;
            vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
            vma_vulkan_func.vkGetPhysicalDeviceProperties       = vkGetPhysicalDeviceProperties;
            vma_vulkan_func.vkInvalidateMappedMemoryRanges      = vkInvalidateMappedMemoryRanges;
            vma_vulkan_func.vkMapMemory                         = vkMapMemory;
            vma_vulkan_func.vkUnmapMemory                       = vkUnmapMemory;
            vma_vulkan_func.vkCmdCopyBuffer                     = vkCmdCopyBuffer;

            VmaAllocatorCreateInfo allocator_info{};
            allocator_info.physicalDevice = physical_device;
            allocator_info.device         = device;
            allocator_info.instance       = instance;

            /*if (can_get_memory_requirements && has_dedicated_allocation)
            {
                allocator_info.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
                vma_vulkan_func.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR;
                vma_vulkan_func.vkGetImageMemoryRequirements2KHR  = vkGetImageMemoryRequirements2KHR;
            }

            if (is_extension_supported(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) &&
            is_enabled(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
            {
                allocator_info.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
            }*/

            allocator_info.pVulkanFunctions = &vma_vulkan_func;

            result = vmaCreateAllocator(&allocator_info, &memory_allocator);

            if (result != VK_SUCCESS)
            {
                VK_THROW(result, "Cannot create allocator");
            }
        }
    }

    VulkanGraphics::~VulkanGraphics()
    {
        VK_CHECK(vkDeviceWaitIdle(device));

        if (memory_allocator != VK_NULL_HANDLE)
        {
            VmaStats stats;
            vmaCalculateStats(memory_allocator, &stats);

            LOGI("Total device memory leaked: {} bytes.", stats.total.usedBytes);

            vmaDestroyAllocator(memory_allocator);
        }

        if (device != VK_NULL_HANDLE)
        {
            vkDestroyDevice(device, nullptr);
        }

        if (surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(instance, surface, nullptr);
        }

#if defined(_DEBUG)
        if (debug_utils_messenger != VK_NULL_HANDLE)
        {
            vkDestroyDebugUtilsMessengerEXT(instance, debug_utils_messenger, nullptr);
        }
#endif

        if (instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(instance, nullptr);
        }
    }
}
