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

#include "config.h"
#include "VulkanGraphicsImpl.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

using namespace eastl;

namespace alimer
{
    namespace
    {
#if defined(GPU_DEBUG) || defined(VULKAN_VALIDATION_LAYERS)
        VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsMessengerCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageTypes,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData)
        {
            // Log debug messge
            if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            {
                LOGW("{} - {}: {}", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
            }
            else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            {
                LOGE("{} - {}: {}", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
            }

            return VK_FALSE;
        }

        bool ValidateLayers(const vector<const char*>& required, const vector<VkLayerProperties>& available)
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

        vector<const char*> GetOptimalValidationLayers(const vector<VkLayerProperties>& supportedInstanceLayers)
        {
            vector<vector<const char*>> validation_layer_priority_list =
            {
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
                {"VK_LAYER_LUNARG_core_validation"} };

            for (auto& validation_layers : validation_layer_priority_list)
            {
                if (ValidateLayers(validation_layers, supportedInstanceLayers))
                {
                    return validation_layers;
                }

                LOGW("Couldn't enable validation layers (see log for error) - falling back");
            }

            // Else return nothing
            return {};
        }
#endif

        QueueFamilyIndices QueryQueueFamilies(VkInstance instance, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
        {
            uint32_t queueCount = 0u;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);

            vector<VkQueueFamilyProperties> queue_families(queueCount);
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queue_families.data());

            QueueFamilyIndices result;
            result.graphicsQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            result.computeQueueFamily = VK_QUEUE_FAMILY_IGNORED;
            result.copyQueueFamily = VK_QUEUE_FAMILY_IGNORED;

            for (uint32_t i = 0; i < queueCount; i++)
            {
                VkBool32 presentSupport = VK_TRUE;
                if (surface != VK_NULL_HANDLE) {
                    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
                }

                static const VkQueueFlags required = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT;
                if (presentSupport && ((queue_families[i].queueFlags & required) == required))
                {
                    result.graphicsQueueFamilyIndex = i;
                    break;
                }
            }

            /* Dedicated compute queue. */
            for (uint32_t i = 0; i < queueCount; i++)
            {
                static const VkQueueFlags required = VK_QUEUE_COMPUTE_BIT;
                if (i != result.graphicsQueueFamilyIndex &&
                    (queue_families[i].queueFlags & required) == required)
                {
                    result.computeQueueFamily = i;
                    break;
                }
            }

            /* Dedicated transfer queue. */
            for (uint32_t i = 0; i < queueCount; i++)
            {
                static const VkQueueFlags required = VK_QUEUE_TRANSFER_BIT;
                if (i != result.graphicsQueueFamilyIndex &&
                    i != result.computeQueueFamily &&
                    (queue_families[i].queueFlags & required) == required)
                {
                    result.copyQueueFamily = i;
                    break;
                }
            }

            if (result.copyQueueFamily == VK_QUEUE_FAMILY_IGNORED)
            {
                for (uint32_t i = 0; i < queueCount; i++)
                {
                    static const VkQueueFlags required = VK_QUEUE_TRANSFER_BIT;
                    if (i != result.graphicsQueueFamilyIndex &&
                        (queue_families[i].queueFlags & required) == required)
                    {
                        result.copyQueueFamily = i;
                        break;
                    }
                }
            }

            return result;
        }

        PhysicalDeviceExtensions QueryPhysicalDeviceExtensions(const VulkanInstanceExtensions& instanceExts, VkPhysicalDevice physicalDevice)
        {
            uint32_t count = 0;
            VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, nullptr));

            vector<VkExtensionProperties> extensions(count);
            VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, extensions.data()));

            PhysicalDeviceExtensions result = {};
            for (uint32_t i = 0; i < count; ++i) {
                if (strcmp(extensions[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
                    result.swapchain = true;
                }
                else if (strcmp(extensions[i].extensionName, VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME) == 0) {
                    result.depth_clip_enable = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_KHR_maintenance1") == 0) {
                    result.maintenance_1 = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_KHR_maintenance2") == 0) {
                    result.maintenance_2 = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_KHR_maintenance3") == 0) {
                    result.maintenance_3 = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_KHR_get_memory_requirements2") == 0) {
                    result.get_memory_requirements2 = true;
                }
                else if (strcmp(extensions[i].extensionName, VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME) == 0) {
                    result.dedicated_allocation = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_KHR_bind_memory2") == 0) {
                    result.bind_memory2 = true;
                }
                else if (strcmp(extensions[i].extensionName, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME) == 0) {
                    result.memory_budget = true;
                }
                else if (strcmp(extensions[i].extensionName, VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME) == 0) {
                    result.image_format_list = true;
                }
                else if (strcmp(extensions[i].extensionName, VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME) == 0) {
                    result.sampler_mirror_clamp_to_edge = true;
                }
                else if (strcmp(extensions[i].extensionName, VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME) == 0) {
                    result.win32_full_screen_exclusive = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_KHR_ray_tracing") == 0) {
                    result.raytracing = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_KHR_buffer_device_address") == 0) {
                    result.buffer_device_address = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_KHR_deferred_host_operations") == 0) {
                    result.deferred_host_operations = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_EXT_descriptor_indexing") == 0) {
                    result.descriptor_indexing = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_KHR_pipeline_library") == 0) {
                    result.pipeline_library = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_KHR_multiview") == 0) {
                    result.multiview = true;
                }
            }

            // Return promoted to version 1.1
            VkPhysicalDeviceProperties2 gpuProps = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
            vkGetPhysicalDeviceProperties2(physicalDevice, &gpuProps);

            /* We run on vulkan 1.1 or higher. */
            if (gpuProps.properties.apiVersion >= VK_API_VERSION_1_1)
            {
                result.maintenance_1 = true;
                result.maintenance_2 = true;
                result.maintenance_3 = true;
                result.get_memory_requirements2 = true;
                result.bind_memory2 = true;
                result.multiview = true;
            }

            return result;
        }

        bool IsDeviceSuitable(VkInstance instance, const VulkanInstanceExtensions& instanceExts, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
        {
            QueueFamilyIndices indices = QueryQueueFamilies(instance, physicalDevice, surface);

            if (indices.graphicsQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED)
                return false;

            PhysicalDeviceExtensions features = QueryPhysicalDeviceExtensions(instanceExts, physicalDevice);
            if (surface != VK_NULL_HANDLE && !features.swapchain) {
                return false;
            }

            /* We required maintenance_1 to support viewport flipping to match DX style. */
            if (!features.maintenance_1) {
                return false;
            }

            return true;
        }

        struct SwapChainSupportDetails {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        };

        SwapChainSupportDetails QuerySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, bool getSurfaceCapabilities2, bool win32_full_screen_exclusive)
        {
            SwapChainSupportDetails details;

            VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR };
            surfaceInfo.surface = surface;

            if (getSurfaceCapabilities2)
            {
                VkSurfaceCapabilities2KHR surfaceCaps2 = { VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR };

                if (vkGetPhysicalDeviceSurfaceCapabilities2KHR(physicalDevice, &surfaceInfo, &surfaceCaps2) != VK_SUCCESS)
                {
                    return details;
                }
                details.capabilities = surfaceCaps2.surfaceCapabilities;

                uint32_t formatCount;
                if (vkGetPhysicalDeviceSurfaceFormats2KHR(physicalDevice, &surfaceInfo, &formatCount, nullptr) != VK_SUCCESS)
                {
                    return details;
                }

                vector<VkSurfaceFormat2KHR> formats2(formatCount);

                for (auto& format2 : formats2)
                {
                    format2 = {};
                    format2.sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
                }

                if (vkGetPhysicalDeviceSurfaceFormats2KHR(physicalDevice, &surfaceInfo, &formatCount, formats2.data()) != VK_SUCCESS)
                {
                    return details;
                }

                details.formats.reserve(formatCount);
                for (auto& format2 : formats2)
                {
                    details.formats.push_back(format2.surfaceFormat);
                }
            }
            else
            {
                if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities) != VK_SUCCESS)
                {
                    return details;
                }

                uint32_t formatCount;
                if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr) != VK_SUCCESS)
                {
                    return details;
                }

                details.formats.resize(formatCount);
                if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data()) != VK_SUCCESS)
                {
                    return details;
                }
            }

            uint32_t presentModeCount = 0;
#ifdef _WIN32
            if (getSurfaceCapabilities2 && win32_full_screen_exclusive)
            {
                if (vkGetPhysicalDeviceSurfacePresentModes2EXT(physicalDevice, &surfaceInfo, &presentModeCount, nullptr) != VK_SUCCESS)
                {
                    return details;
                }

                details.presentModes.resize(presentModeCount);
                if (vkGetPhysicalDeviceSurfacePresentModes2EXT(physicalDevice, &surfaceInfo, &presentModeCount, details.presentModes.data()) != VK_SUCCESS)
                {
                    return details;
                }
            }
            else
#endif
            {
                if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr) != VK_SUCCESS)
                {
                    return details;
                }

                details.presentModes.resize(presentModeCount);
                if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data()) != VK_SUCCESS)
                {
                    return details;
                }
            }

            return details;
        }
    }

    bool VulkanGraphicsImpl::IsAvailable()
    {
        static bool available_initialized = false;
        static bool available = false;
        if (available_initialized) {
            return available;
        }

        available_initialized = true;
        VkResult result = volkInitialize();
        if (result != VK_SUCCESS)
        {
            return false;
        }

        // We require Vulkan 1.1 at least
        uint32_t apiVersion = volkGetInstanceVersion();
        if (apiVersion <= VK_API_VERSION_1_1) {
            return false;
        }

        VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        appInfo.apiVersion = volkGetInstanceVersion();

        VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        createInfo.pApplicationInfo = &appInfo;

        VkInstance instance;
        result = vkCreateInstance(&createInfo, nullptr, &instance);
        if (result != VK_SUCCESS) {
            return false;
        }

        vkDestroyInstance = (PFN_vkDestroyInstance)vkGetInstanceProcAddr(instance, "vkDestroyInstance");
        vkDestroyInstance(instance, nullptr);
        available = true;
        return true;
    }

    VulkanGraphicsImpl::VulkanGraphicsImpl()
    {
        ALIMER_VERIFY(IsAvailable());

        // Create instance
        {
            vector<const char*> enabledExtensions;
            vector<const char*> enabledLayers;

            uint32_t instanceExtensionCount;
            VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr));

            vector<VkExtensionProperties> availableInstanceExtensions(instanceExtensionCount);
            VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, availableInstanceExtensions.data()));
            for (auto& availableExtension : availableInstanceExtensions)
            {
                if (strcmp(availableExtension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
                {
                    instanceExts.debugUtils = true;
#if defined(GPU_DEBUG) || defined(VULKAN_VALIDATION_LAYERS)
                    enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
                }
                else if (strcmp(availableExtension.extensionName, VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME) == 0)
                {
                    instanceExts.headless = true;
                }
                else if (strcmp(availableExtension.extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0)
                {
                    // VK_KHR_get_physical_device_properties2 is a prerequisite of VK_KHR_performance_query
                    // which will be used for stats gathering where available.
                    instanceExts.get_physical_device_properties2 = true;
                    enabledExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
                }
                else if (strcmp(availableExtension.extensionName, VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME) == 0)
                {
                    instanceExts.get_surface_capabilities2 = true;
                }
            }


#if defined(GPU_DEBUG) || defined(VULKAN_VALIDATION_LAYERS)
            uint32_t instanceLayerCount;
            VK_CHECK(vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr));

            vector<VkLayerProperties> supportedInstanceLayers(instanceLayerCount);
            VK_CHECK(vkEnumerateInstanceLayerProperties(&instanceLayerCount, supportedInstanceLayers.data()));

            vector<const char*> optimalValidationLayers = GetOptimalValidationLayers(supportedInstanceLayers);
            enabledLayers.insert(enabledLayers.end(), optimalValidationLayers.begin(), optimalValidationLayers.end());
#endif

            const bool headless = false;
            if (headless)
            {
                enabledExtensions.push_back(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
            }
            else
            {
                enabledExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

                if (instanceExts.get_surface_capabilities2)
                {
                    enabledExtensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
                }
            }

            VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
            appInfo.pApplicationName = "Alimer";
            appInfo.applicationVersion = 0;
            appInfo.pEngineName = "Alimer";
            appInfo.engineVersion = VK_MAKE_VERSION(ALIMER_VERSION_MAJOR, ALIMER_VERSION_MINOR, ALIMER_VERSION_PATCH);
            appInfo.apiVersion = volkGetInstanceVersion();

            VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };

#if defined(GPU_DEBUG) || defined(VULKAN_VALIDATION_LAYERS)
            VkDebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };

            if (instanceExts.debugUtils)
            {
                debugUtilsCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
                debugUtilsCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
                debugUtilsCreateInfo.pfnUserCallback = DebugUtilsMessengerCallback;
                debugUtilsCreateInfo.pUserData = this;

                createInfo.pNext = &debugUtilsCreateInfo;
            }
#endif


            createInfo.pApplicationInfo = &appInfo;
            createInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
            createInfo.ppEnabledLayerNames = enabledLayers.data();
            createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
            createInfo.ppEnabledExtensionNames = enabledExtensions.data();

            // Create the Vulkan instance.
            VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

            if (result != VK_SUCCESS)
            {
                VK_LOG_ERROR(result, "Could not create Vulkan instance");
            }

            volkLoadInstance(instance);

#if defined(GPU_DEBUG) || defined(VULKAN_VALIDATION_LAYERS)
            if (instanceExts.debugUtils)
            {
                result = vkCreateDebugUtilsMessengerEXT(instance, &debugUtilsCreateInfo, nullptr, &debugUtilsMessenger);
                if (result != VK_SUCCESS)
                {
                    VK_LOG_ERROR(result, "Could not create debug utils messenger");
                }
            }
#endif

            LOGI("Created VkInstance with version: {}.{}.{}", VK_VERSION_MAJOR(appInfo.apiVersion), VK_VERSION_MINOR(appInfo.apiVersion), VK_VERSION_PATCH(appInfo.apiVersion));
            if (createInfo.enabledLayerCount) {
                for (uint32_t i = 0; i < createInfo.enabledLayerCount; ++i)
                    LOGI("Instance layer '{}'", createInfo.ppEnabledLayerNames[i]);
            }

            for (uint32_t i = 0; i < createInfo.enabledExtensionCount; ++i) {
                LOGI("Instance extension '{}'", createInfo.ppEnabledExtensionNames[i]);
            }
        }

#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
        //VK_CHECK(glfwCreateWindowSurface(instance, (GLFWwindow*)window->GetWindow(), nullptr, &surface));
#endif

        // Enumerating and creating devices:
        {
            uint32_t physicalDevicesCount = 0;
            VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, nullptr));

            if (physicalDevicesCount == 0) {
                LOGE("failed to find GPUs with Vulkan support!");
                assert(0);
            }

            std::vector<VkPhysicalDevice> physicalDevices(physicalDevicesCount);
            vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, physicalDevices.data());

            uint32_t bestDeviceScore = 0u;
            uint32_t bestDeviceIndex = VK_QUEUE_FAMILY_IGNORED;
            for (uint32_t i = 0; i < physicalDevicesCount; ++i)
            {
                if (!IsDeviceSuitable(instance, instanceExts, physicalDevices[i], surface)) {
                    continue;
                }

                VkPhysicalDeviceProperties physical_device_props;
                vkGetPhysicalDeviceProperties(physicalDevices[i], &physical_device_props);

                uint32_t score = 0u;

                if (physical_device_props.apiVersion >= VK_API_VERSION_1_2) {
                    score += 10000u;
                }

                switch (physical_device_props.deviceType) {
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    score += 1000u;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    score += 100u;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    score += 80U;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    score += 70U;
                    break;
                default: score += 10U;
                }

                if (score > bestDeviceScore) {
                    bestDeviceIndex = i;
                    bestDeviceScore = score;
                }
            }

            if (bestDeviceIndex == VK_QUEUE_FAMILY_IGNORED) {
                LOGE("Vulkan: Cannot find suitable physical device.");
                return;
            }

            physicalDevice = physicalDevices[bestDeviceIndex];
            physicalDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProperties);
            queueFamilies = QueryQueueFamilies(instance, physicalDevice, surface);
            physicalDeviceExts = QueryPhysicalDeviceExtensions(instanceExts, physicalDevice);
        }

        /* Setup device queue's. */
        {
            uint32_t queue_count;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queue_count, nullptr);
            vector<VkQueueFamilyProperties> queue_families(queue_count);
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queue_count, queue_families.data());

            uint32_t universal_queue_index = 1;
            uint32_t compute_queue_index = 0;
            uint32_t copy_queue_index = 0;

            if (queueFamilies.computeQueueFamily == VK_QUEUE_FAMILY_IGNORED)
            {
                queueFamilies.computeQueueFamily = queueFamilies.graphicsQueueFamilyIndex;
                compute_queue_index = Min(queue_families[queueFamilies.graphicsQueueFamilyIndex].queueCount - 1, universal_queue_index);
                universal_queue_index++;
            }

            if (queueFamilies.copyQueueFamily == VK_QUEUE_FAMILY_IGNORED)
            {
                queueFamilies.copyQueueFamily = queueFamilies.graphicsQueueFamilyIndex;
                copy_queue_index = Min(queue_families[queueFamilies.graphicsQueueFamilyIndex].queueCount - 1, universal_queue_index);
                universal_queue_index++;
            }
            else if (queueFamilies.copyQueueFamily == queueFamilies.computeQueueFamily)
            {
                copy_queue_index = Min(queue_families[queueFamilies.computeQueueFamily].queueCount - 1, 1u);
            }

            static const float graphics_queue_prio = 0.5f;
            static const float compute_queue_prio = 1.0f;
            static const float transfer_queue_prio = 1.0f;
            float prio[3] = { graphics_queue_prio, compute_queue_prio, transfer_queue_prio };

            uint32_t queueCreateCount = 0;
            VkDeviceQueueCreateInfo queue_info[3] = { };
            queue_info[queueCreateCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_info[queueCreateCount].queueFamilyIndex = queueFamilies.graphicsQueueFamilyIndex;
            queue_info[queueCreateCount].queueCount = Min(universal_queue_index, queue_families[queueFamilies.graphicsQueueFamilyIndex].queueCount);
            queue_info[queueCreateCount].pQueuePriorities = prio;
            queueCreateCount++;

            if (queueFamilies.computeQueueFamily != queueFamilies.graphicsQueueFamilyIndex)
            {
                queue_info[queueCreateCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_info[queueCreateCount].queueFamilyIndex = queueFamilies.computeQueueFamily;
                queue_info[queueCreateCount].queueCount = Min(queueFamilies.copyQueueFamily == queueFamilies.computeQueueFamily ? 2u : 1u,
                    queue_families[queueFamilies.computeQueueFamily].queueCount);
                queue_info[queueCreateCount].pQueuePriorities = prio + 1;
                queueCreateCount++;
            }

            // Dedicated copy queue
            if (queueFamilies.copyQueueFamily != queueFamilies.graphicsQueueFamilyIndex
                && queueFamilies.copyQueueFamily != queueFamilies.computeQueueFamily)
            {
                queue_info[queueCreateCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_info[queueCreateCount].queueFamilyIndex = queueFamilies.copyQueueFamily;
                queue_info[queueCreateCount].queueCount = 1;
                queue_info[queueCreateCount].pQueuePriorities = prio + 2;
                queueCreateCount++;
            }

            /* Setup device extensions now. */
            const bool deviceApiVersion11 = physicalDeviceProperties.properties.apiVersion >= VK_API_VERSION_1_1;
            vector<const char*> enabledDeviceExtensions;

            if (surface != VK_NULL_HANDLE) {
                enabledDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
            }

            if (!deviceApiVersion11) {
                if (physicalDeviceExts.maintenance_1)
                {
                    enabledDeviceExtensions.push_back("VK_KHR_maintenance1");
                }

                if (physicalDeviceExts.maintenance_2) {
                    enabledDeviceExtensions.push_back("VK_KHR_maintenance2");
                }

                if (physicalDeviceExts.maintenance_3) {
                    enabledDeviceExtensions.push_back("VK_KHR_maintenance3");
                }
            }

            if (physicalDeviceExts.image_format_list)
            {
                enabledDeviceExtensions.push_back(VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME);
            }

            if (physicalDeviceExts.sampler_mirror_clamp_to_edge)
            {
                enabledDeviceExtensions.push_back(VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME);
            }

            if (physicalDeviceExts.depth_clip_enable)
            {
                enabledDeviceExtensions.push_back(VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME);
            }

            /*if (vk.physical_device_features.buffer_device_address)
            {
                enabled_device_exts[enabled_device_ext_count++] = VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME;
            }*/

#ifdef _WIN32
            if (instanceExts.get_surface_capabilities2 && physicalDeviceExts.win32_full_screen_exclusive)
            {
                enabledDeviceExtensions.push_back(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME);
            }
#endif

            VkPhysicalDeviceFeatures2 features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
            VkPhysicalDeviceMultiviewFeatures multiview_features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES };
            void** ppNext = &features.pNext;

            if (physicalDeviceExts.multiview)
            {
                if (!deviceApiVersion11) {
                    enabledDeviceExtensions.push_back("VK_KHR_multiview");
                }

                *ppNext = &multiview_features;
                ppNext = &multiview_features.pNext;
            }

            vkGetPhysicalDeviceFeatures2(physicalDevice, &features);

            // Enable device features we might care about.
            {
                VkPhysicalDeviceFeatures enabled_features = {};

                if (features.features.textureCompressionBC)
                    enabled_features.textureCompressionBC = VK_TRUE;
                else if (features.features.textureCompressionASTC_LDR)
                    enabled_features.textureCompressionASTC_LDR = VK_TRUE;
                else if (features.features.textureCompressionETC2)
                    enabled_features.textureCompressionETC2 = VK_TRUE;

                if (features.features.fullDrawIndexUint32)
                    enabled_features.fullDrawIndexUint32 = VK_TRUE;
                if (features.features.multiDrawIndirect)
                    enabled_features.multiDrawIndirect = VK_TRUE;
                if (features.features.imageCubeArray)
                    enabled_features.imageCubeArray = VK_TRUE;
                if (features.features.fillModeNonSolid)
                    enabled_features.fillModeNonSolid = VK_TRUE;
                if (features.features.independentBlend)
                    enabled_features.independentBlend = VK_TRUE;
                if (features.features.shaderSampledImageArrayDynamicIndexing)
                    enabled_features.shaderSampledImageArrayDynamicIndexing = VK_TRUE;

                features.features = enabled_features;
            }

            VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
            createInfo.pNext = &features;
            createInfo.queueCreateInfoCount = queueCreateCount;
            createInfo.pQueueCreateInfos = queue_info;
            createInfo.enabledExtensionCount = (uint32)enabledDeviceExtensions.size();
            createInfo.ppEnabledExtensionNames = enabledDeviceExtensions.data();

            VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
            if (result != VK_SUCCESS) {
                VK_LOG_ERROR(result, "Failed to create device");
                return;
            }

            vkGetDeviceQueue(device, queueFamilies.graphicsQueueFamilyIndex, 0, &graphicsQueue);
            vkGetDeviceQueue(device, queueFamilies.computeQueueFamily, compute_queue_index, &computeQueue);
            vkGetDeviceQueue(device, queueFamilies.copyQueueFamily, copy_queue_index, &copyQueue);

            LOGI("Created VkDevice using '{}' adapter with API version: {}.{}.{}",
                physicalDeviceProperties.properties.deviceName,
                VK_VERSION_MAJOR(physicalDeviceProperties.properties.apiVersion),
                VK_VERSION_MINOR(physicalDeviceProperties.properties.apiVersion),
                VK_VERSION_PATCH(physicalDeviceProperties.properties.apiVersion));
            for (uint32_t i = 0; i < createInfo.enabledExtensionCount; ++i) {
                LOGI("Device extension '{}'", createInfo.ppEnabledExtensionNames[i]);
            }
        }

        // Create vma allocator.
        {
            VmaAllocatorCreateInfo allocatorInfo = {};
            allocatorInfo.physicalDevice = physicalDevice;
            allocatorInfo.device = device;
            allocatorInfo.instance = instance;

            if (physicalDeviceExts.get_memory_requirements2 && physicalDeviceExts.dedicated_allocation)
            {
                allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
            }

            /* if (is_extension_supported(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) && is_enabled(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
             {
                 allocator_info.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
             }*/


            VkResult result = vmaCreateAllocator(&allocatorInfo, &memoryAllocator);
            if (result != VK_SUCCESS)
            {
                VK_LOG_ERROR(result, "Cannot create allocator");
                return;
            }
        }

        InitCapabilities();
        UpdateSwapchain();

        // Create frame data.
        for (size_t i = 0, count = perFrame.size(); i < count; i++)
        {
            VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            VK_CHECK(vkCreateFence(device, &fenceInfo, nullptr, &perFrame[i].queueSubmitFence));

            VkCommandPoolCreateInfo commandPoolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
            commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            commandPoolInfo.queueFamilyIndex = queueFamilies.graphicsQueueFamilyIndex;
            VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &perFrame[i].primaryCommandPool));

            VkCommandBufferAllocateInfo cmdAllocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
            cmdAllocateInfo.commandPool = perFrame[i].primaryCommandPool;
            cmdAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdAllocateInfo.commandBufferCount = 1;
            VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocateInfo, &perFrame[i].primaryCommandBuffer));
        }
    }

    VulkanGraphicsImpl::~VulkanGraphicsImpl()
    {
        WaitForGPU();
        Shutdown();
    }

    void VulkanGraphicsImpl::Shutdown()
    {
        vkDestroySwapchainKHR(device, swapchain, nullptr);

        if (memoryAllocator != VK_NULL_HANDLE)
        {
            VmaStats stats;
            vmaCalculateStats(memoryAllocator, &stats);

            if (stats.total.usedBytes > 0) {
                LOGE("Total device memory leaked: {} bytes.", stats.total.usedBytes);
            }

            vmaDestroyAllocator(memoryAllocator);
        }

        vkDestroyDevice(device, nullptr);

#if defined(GPU_DEBUG) || defined(VULKAN_VALIDATION_LAYERS)
        if (debugUtilsMessenger != VK_NULL_HANDLE)
        {
            vkDestroyDebugUtilsMessengerEXT(instance, debugUtilsMessenger, nullptr);
        }
#endif

        if (instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(instance, nullptr);
        }
    }

    void VulkanGraphicsImpl::InitCapabilities()
    {
        caps.rendererType = RendererType::Vulkan;
        caps.vendorId = physicalDeviceProperties.properties.vendorID;
        caps.deviceId = physicalDeviceProperties.properties.deviceID;
        caps.adapterName = physicalDeviceProperties.properties.deviceName;

        switch (physicalDeviceProperties.properties.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            caps.adapterType = GPUAdapterType::IntegratedGPU;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            caps.adapterType = GPUAdapterType::DiscreteGPU;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            caps.adapterType = GPUAdapterType::CPU;
            break;
        default:
            caps.adapterType = GPUAdapterType::Unknown;
            break;
        }
    }

    bool VulkanGraphicsImpl::UpdateSwapchain()
    {
        WaitForGPU();

        SwapChainSupportDetails surfaceCaps = QuerySwapchainSupport(physicalDevice, surface, instanceExts.get_surface_capabilities2, physicalDeviceExts.win32_full_screen_exclusive);

        /* Detect image count. */
        uint32_t imageCount = surfaceCaps.capabilities.minImageCount + 1;
        if ((surfaceCaps.capabilities.maxImageCount > 0) && (imageCount > surfaceCaps.capabilities.maxImageCount))
        {
            imageCount = surfaceCaps.capabilities.maxImageCount;
        }

        /* Surface format. */
        VkSurfaceFormatKHR format;
        if (surfaceCaps.formats.size() == 1 &&
            surfaceCaps.formats[0].format == VK_FORMAT_UNDEFINED)
        {
            format = surfaceCaps.formats[0];
            format.format = VK_FORMAT_B8G8R8A8_UNORM;
        }
        else
        {
            if (surfaceCaps.formats.size() == 0)
            {
                //vgpu_log(VGPU_LOG_LEVEL_ERROR, "Vulkan: Surface has no formats.");
                return false;
            }

            const bool srgb = false;
            bool found = false;
            for (uint32_t i = 0; i < surfaceCaps.formats.size(); i++)
            {
                if (srgb)
                {
                    if (surfaceCaps.formats[i].format == VK_FORMAT_R8G8B8A8_SRGB ||
                        surfaceCaps.formats[i].format == VK_FORMAT_B8G8R8A8_SRGB ||
                        surfaceCaps.formats[i].format == VK_FORMAT_A8B8G8R8_SRGB_PACK32)
                    {
                        format = surfaceCaps.formats[i];
                        found = true;
                        break;
                    }
                }
                else
                {
                    if (surfaceCaps.formats[i].format == VK_FORMAT_R8G8B8A8_UNORM ||
                        surfaceCaps.formats[i].format == VK_FORMAT_B8G8R8A8_UNORM ||
                        surfaceCaps.formats[i].format == VK_FORMAT_A8B8G8R8_UNORM_PACK32)
                    {
                        format = surfaceCaps.formats[i];
                        found = true;
                        break;
                    }
                }
            }

            if (!found)
            {
                format = surfaceCaps.formats[0];
            }
        }

        /* Extent */
        VkExtent2D swapchainSize{}; // = { backbufferWidth, backbufferHeight };
        if (swapchainSize.width < 1 || swapchainSize.height < 1)
        {
            swapchainSize = surfaceCaps.capabilities.currentExtent;
        }
        else
        {
            swapchainSize.width = Max(swapchainSize.width, surfaceCaps.capabilities.minImageExtent.width);
            swapchainSize.width = Min(swapchainSize.width, surfaceCaps.capabilities.maxImageExtent.width);
            swapchainSize.height = Max(swapchainSize.height, surfaceCaps.capabilities.minImageExtent.height);
            swapchainSize.height = Min(swapchainSize.height, surfaceCaps.capabilities.maxImageExtent.height);
        }

        VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        // Enable transfer source on swap chain images if supported
        if (surfaceCaps.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
            imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            //textureUsage |= VGPUTextureUsage_CopySrc;
        }

        // Enable transfer destination on swap chain images if supported
        if (surfaceCaps.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
            imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            //textureUsage |= VGPUTextureUsage_CopyDst;
        }

        VkSurfaceTransformFlagBitsKHR pre_transform;
        if ((surfaceCaps.capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) != 0)
            pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        else
            pre_transform = surfaceCaps.capabilities.currentTransform;

        VkCompositeAlphaFlagBitsKHR composite_mode = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        if (surfaceCaps.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
            composite_mode = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
        if (surfaceCaps.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
            composite_mode = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        if (surfaceCaps.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
            composite_mode = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
        if (surfaceCaps.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
            composite_mode = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;

        VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
        //if (!vSync)
        {
            // The immediate present mode is not necessarily supported:
            for (auto& presentMode : surfaceCaps.presentModes)
            {
                if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                {
                    presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                    break;
                }
            }
        }

        VkSwapchainKHR oldSwapchain = swapchain;

        /* We use same family for graphics and present so no sharing is necessary. */
        VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = format.format;
        createInfo.imageColorSpace = format.colorSpace;
        createInfo.imageExtent = swapchainSize;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = imageUsage;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = NULL;
        createInfo.preTransform = pre_transform;
        createInfo.compositeAlpha = composite_mode;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = oldSwapchain;

        VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain);
        if (result != VK_SUCCESS) {
            return false;
        }

        if (oldSwapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
        }

        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
        std::vector<VkImage> swapChainImages(imageCount);
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapChainImages.data());

        perFrame.clear();
        perFrame.resize(imageCount);

        for (uint32 i = 0; i < imageCount; i++)
        {
            //backbufferTextures[backbufferIndex] = new VulkanTexture(this, swapChainImages[i]);
            //backbufferTextures[backbufferIndex]->SetName(fmt::format("Back Buffer {}", i));
            //SetObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)swapChainImages[i], fmt::format("Back Buffer {}", i));
        }

        return true;
    }

    bool VulkanGraphicsImpl::Initialize(WindowHandle windowHandle, uint32_t width, uint32_t height, bool isFullscreen)
    {
        VkResult result = VK_SUCCESS;

#if defined(VK_USE_PLATFORM_WIN32_KHR)
        VkWin32SurfaceCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
        createInfo.hinstance = GetModuleHandle(NULL);
        createInfo.hwnd = (HWND)windowHandle;

        result = vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface);

#else
        LOGE("Cannot create Win32 surfare on non windows platform");
        return nullptr;
#endif

        if (result != VK_SUCCESS)
        {
            VK_LOG_ERROR(result, "Vulkan: Failed to create surface");
            return false;
        }

        return true;
    }

    void VulkanGraphicsImpl::SetObjectName(VkObjectType type, uint64_t handle, const eastl::string& name)
    {
        if (!instanceExts.debugUtils)
            return;

        VkDebugUtilsObjectNameInfoEXT info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        info.objectType = type;
        info.objectHandle = handle;
        info.pObjectName = name.c_str();
        VK_CHECK(vkSetDebugUtilsObjectNameEXT(device, &info));
    }

    void VulkanGraphicsImpl::WaitForGPU()
    {
        VK_CHECK(vkDeviceWaitIdle(device));
    }

    bool VulkanGraphicsImpl::BeginFrame()
    {
        ALIMER_ASSERT_MSG(!frameActive, "Frame is still active, please call EndFrame first");

        VkSemaphore acquireSemaphore;
        if (recycledSemaphores.empty())
        {
            VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
            VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &acquireSemaphore));
        }
        else
        {
            acquireSemaphore = recycledSemaphores.back();
            recycledSemaphores.pop_back();
        }

        VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, acquireSemaphore, VK_NULL_HANDLE, &backbufferIndex);

        if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recycledSemaphores.push_back(acquireSemaphore);
            //handle_surface_changes();
            //result = swapchain->acquire_next_image(active_frame_index, aquired_semaphore, fence);
        }

        if (perFrame[backbufferIndex].queueSubmitFence != VK_NULL_HANDLE)
        {
            vkWaitForFences(device, 1, &perFrame[backbufferIndex].queueSubmitFence, VK_TRUE, UINT64_MAX);
            vkResetFences(device, 1, &perFrame[backbufferIndex].queueSubmitFence);
        }

        if (perFrame[backbufferIndex].primaryCommandPool != VK_NULL_HANDLE)
        {
            vkResetCommandPool(device, perFrame[backbufferIndex].primaryCommandPool, 0);
        }

        // Recycle the old semaphore back into the semaphore manager.
        VkSemaphore oldSemaphore = perFrame[backbufferIndex].swapchainAcquireSemaphore;
        if (oldSemaphore != VK_NULL_HANDLE)
        {
            recycledSemaphores.push_back(oldSemaphore);
        }

        perFrame[backbufferIndex].swapchainAcquireSemaphore = acquireSemaphore;


        /* TODO: Lazy release resources */

        // We will only submit this once before it's recycled.
        VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        // Begin command recording
        VK_CHECK(vkBeginCommandBuffer(perFrame[backbufferIndex].primaryCommandBuffer, &begin_info));

        // Now the frame is active again.
        frameActive = true;

        return true;
    }

    void VulkanGraphicsImpl::EndFrame(uint64_t frameIndex)
    {
        //ALIMER_ASSERT_MSG(frameActive, "Frame is not active, please call BeginFrame first.");

        // Complete the command buffer.
        VK_CHECK(vkEndCommandBuffer(perFrame[backbufferIndex].primaryCommandBuffer));

        // Submit it to the queue with a release semaphore.
        if (perFrame[backbufferIndex].swapchainReleaseSemaphore == VK_NULL_HANDLE)
        {
            VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
            VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &perFrame[backbufferIndex].swapchainReleaseSemaphore));
        }

        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &perFrame[backbufferIndex].swapchainAcquireSemaphore;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &perFrame[backbufferIndex].primaryCommandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &perFrame[backbufferIndex].swapchainReleaseSemaphore;

        VkResult result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, perFrame[backbufferIndex].queueSubmitFence);
        VK_CHECK(result);

        VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &perFrame[backbufferIndex].swapchainReleaseSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &backbufferIndex;

        result = vkQueuePresentKHR(graphicsQueue, &presentInfo);

        if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            //handle_surface_changes();
        }

        // Frame is not active anymore
        frameActive = false;
    }
}
