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

#include <vector>

#if defined(__linux__) || defined(__APPLE__)
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#endif

using namespace std;

namespace alimer
{
    namespace
    {
        VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsMessengerCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageTypes,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData)
        {
            // Log debug messge
            if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            {
                LOG_WARN("%u - %s: %s", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
            }
            else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            {
                LOG_ERROR("%u - %s: %s", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
            }
            return VK_FALSE;
        }

        bool ValidateLayers(const std::vector<const char*>& required, const std::vector<VkLayerProperties>& available)
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
                    LOG_ERROR("Validation Layer '%s' not found", layer);
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

                LOG_WARN("Couldn't enable validation layers (see log for error) - falling back");
            }

            // Else return nothing
            return {};
        }

        bool QueryPresentationSupport(VkInstance instance, VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex)
        {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
            return vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, queueFamilyIndex);
#elif defined(__linux__) || defined(__APPLE__)
            return glfwGetPhysicalDevicePresentationSupport(instance, physicalDevice, queueFamilyIndex);
#else
            return true;
#endif
        }

        QueueFamilyIndices QueryQueueFamilies(VkInstance instance, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
        {
            uint32_t queueCount = 0u;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);

            vector<VkQueueFamilyProperties> queue_families(queueCount);
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queue_families.data());

            QueueFamilyIndices result;
            result.graphicsQueueFamily = VK_QUEUE_FAMILY_IGNORED;
            result.computeQueueFamily = VK_QUEUE_FAMILY_IGNORED;
            result.copyQueueFamily = VK_QUEUE_FAMILY_IGNORED;

            for (uint32_t i = 0; i < queueCount; i++)
            {
                VkBool32 present_support = VK_TRUE;
                if (surface != VK_NULL_HANDLE) {
                    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &present_support);
                }
                else {
                    present_support = QueryPresentationSupport(instance, physicalDevice, i);
                }

                static const VkQueueFlags required = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT;
                if (present_support && ((queue_families[i].queueFlags & required) == required))
                {
                    result.graphicsQueueFamily = i;
                    break;
                }
            }

            /* Dedicated compute queue. */
            for (uint32_t i = 0; i < queueCount; i++)
            {
                static const VkQueueFlags required = VK_QUEUE_COMPUTE_BIT;
                if (i != result.graphicsQueueFamily &&
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
                if (i != result.graphicsQueueFamily &&
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
                    if (i != result.graphicsQueueFamily &&
                        (queue_families[i].queueFlags & required) == required)
                    {
                        result.copyQueueFamily = i;
                        break;
                    }
                }
            }

            return result;
        }

        PhysicalDeviceExtensions QueryPhysicalDeviceExtensions(const InstanceExtensions& instanceExts, VkPhysicalDevice physicalDevice)
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
                else if (strcmp(extensions[i].extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0) {
                    result.debug_marker = true;
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
                result.multiview = instanceExts.get_physical_device_properties2;
            }

            return result;
        }

        bool IsDeviceSuitable(VkInstance instance, const InstanceExtensions& instanceExts, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, bool headless)
        {
            QueueFamilyIndices indices = QueryQueueFamilies(instance, physicalDevice, surface);

            if (indices.graphicsQueueFamily == VK_QUEUE_FAMILY_IGNORED)
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
    }

    GraphicsImpl::GraphicsImpl(bool enableValidationLayer, PowerPreference powerPreference, bool headless)
    {
        VkResult result = volkInitialize();
        if (result)
        {
            VK_LOG_ERROR(result, "Failed to initialize volk.");
        }

        uint32_t apiVersion = volkGetInstanceVersion();
        if (apiVersion >= VK_API_VERSION_1_1) {
            instanceExts.apiVersion11 = true;
            instanceExts.get_physical_device_properties2 = true;
            instanceExts.external_memory_capabilities = true;
            instanceExts.external_semaphore_capabilities = true;
        }

        // Create instance
        {
            vector<const char*> enabledInstanceExtensions;
            vector<const char*> enabledInstanceLayers;

            uint32_t instanceExtensionCount;
            VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr));

            vector<VkExtensionProperties> availableInstanceExtensions(instanceExtensionCount);
            VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, availableInstanceExtensions.data()));
            for (auto& availableExtension : availableInstanceExtensions)
            {
                if (strcmp(availableExtension.extensionName, "VK_KHR_get_physical_device_properties2") == 0) {
                    instanceExts.get_physical_device_properties2 = true;
                }
                else if (strcmp(availableExtension.extensionName, "VK_KHR_external_memory_capabilities") == 0)
                {
                    instanceExts.external_memory_capabilities = true;
                    //enabled_exts[enabled_exts_count++] = VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME;
                }
                else if (strcmp(availableExtension.extensionName, "VK_KHR_external_semaphore_capabilities") == 0)
                {
                    instanceExts.external_semaphore_capabilities = true;
                    //enabled_exts[enabled_exts_count++] = VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME;
                }
                else if (strcmp(availableExtension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
                    instanceExts.debugUtils = true;
                }
                else if (strcmp(availableExtension.extensionName, VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME) == 0)
                {
                    instanceExts.headless = true;
                }
                else if (strcmp(availableExtension.extensionName, VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME) == 0)
                {
                    instanceExts.getSurfaceCapabilities2 = true;
                }
            }

            if (!instanceExts.apiVersion11)
            {
                enabledInstanceExtensions.push_back("VK_KHR_get_physical_device_properties2");
                enabledInstanceExtensions.push_back("VK_KHR_external_memory_capabilities");
                enabledInstanceExtensions.push_back("VK_KHR_external_semaphore_capabilities");
            }

            if (headless)
            {
                enabledInstanceExtensions.push_back(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
            }
            else
            {
                enabledInstanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#if defined(_WIN32)
                enabledInstanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

                if (instanceExts.getSurfaceCapabilities2) {
                    enabledInstanceExtensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
                }
            }


            if (enableValidationLayer && instanceExts.debugUtils)
            {
                enabledInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }

            // Layers
            if (enableValidationLayer)
            {
                uint32_t instanceLayerCount;
                VK_CHECK(vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr));

                vector<VkLayerProperties> availableInstanceLayers(instanceLayerCount);
                VK_CHECK(vkEnumerateInstanceLayerProperties(&instanceLayerCount, availableInstanceLayers.data()));

                // Determine the optimal validation layers to enable that are necessary for useful debugging
                vector<const char*> optimalValidationLayers = GetOptimalValidationLayers(availableInstanceLayers);
                enabledInstanceLayers.insert(enabledInstanceLayers.end(), optimalValidationLayers.begin(), optimalValidationLayers.end());
            }

            VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
            appInfo.pApplicationName = "Alimer";
            appInfo.applicationVersion = 0;
            appInfo.pEngineName = "Alimer";
            appInfo.engineVersion = VK_MAKE_VERSION(ALIMER_VERSION_MAJOR, ALIMER_VERSION_MINOR, ALIMER_VERSION_PATCH);
            appInfo.apiVersion = apiVersion;

            VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
            VkDebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };

            if (enableValidationLayer)
            {
                if (instanceExts.debugUtils)
                {
                    debugUtilsCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
                    debugUtilsCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
                    debugUtilsCreateInfo.pfnUserCallback = DebugUtilsMessengerCallback;
                    debugUtilsCreateInfo.pUserData = this;

                    createInfo.pNext = &debugUtilsCreateInfo;
                }
            }

            createInfo.pApplicationInfo = &appInfo;

            createInfo.enabledLayerCount = static_cast<uint32_t>(enabledInstanceLayers.size());
            createInfo.ppEnabledLayerNames = enabledInstanceLayers.data();
            createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledInstanceExtensions.size());
            createInfo.ppEnabledExtensionNames = enabledInstanceExtensions.data();

            // Create the Vulkan instance.
            result = vkCreateInstance(&createInfo, nullptr, &instance);

            if (result != VK_SUCCESS)
            {
                VK_LOG_ERROR(result, "Could not create Vulkan instance");
            }

            volkLoadInstance(instance);

            if (enableValidationLayer && instanceExts.debugUtils)
            {
                result = vkCreateDebugUtilsMessengerEXT(instance, &debugUtilsCreateInfo, nullptr, &debugUtilsMessenger);
                if (result != VK_SUCCESS)
                {
                    VK_LOG_ERROR(result, "Could not create debug utils messenger");
                }
            }

            LOG_INFO("Created VkInstance with version: %u.%u.%u", VK_VERSION_MAJOR(appInfo.apiVersion), VK_VERSION_MINOR(appInfo.apiVersion), VK_VERSION_PATCH(appInfo.apiVersion));
            if (createInfo.enabledLayerCount) {
                for (uint32_t i = 0; i < createInfo.enabledLayerCount; ++i)
                    LOG_INFO("Instance layer '%s'", createInfo.ppEnabledLayerNames[i]);
            }

            for (uint32_t i = 0; i < createInfo.enabledExtensionCount; ++i) {
                LOG_INFO("Instance extension '%s'", createInfo.ppEnabledExtensionNames[i]);
            }
        }

        // Enumerating and creating devices:
        {
            uint32_t physicalDevicesCount = 0;
            VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, nullptr));

            if (physicalDevicesCount == 0) {
                LOG_ERROR("failed to find GPUs with Vulkan support!");
                assert(0);
            }

            std::vector<VkPhysicalDevice> physicalDevices(physicalDevicesCount);
            vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, physicalDevices.data());

            VkSurfaceKHR surface = VK_NULL_HANDLE;
            uint32_t bestDeviceScore = 0U;
            uint32_t bestDeviceIndex = VK_QUEUE_FAMILY_IGNORED;
            for (uint32_t i = 0; i < physicalDevicesCount; ++i)
            {
                if (!IsDeviceSuitable(instance, instanceExts, physicalDevices[i], surface, headless)) {
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
                    score += 100U;
                    if (powerPreference == PowerPreference::Default || powerPreference == PowerPreference::HighPerformance) {
                        score += 1000u;
                    }
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    score += 90U;
                    if (powerPreference == PowerPreference::LowPower) {
                        score += 1000u;
                    }
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
                LOG_ERROR("Vulkan: Cannot find suitable physical device.");
                return;
            }

            physicalDevice = physicalDevices[bestDeviceIndex];
            if (instanceExts.get_physical_device_properties2)
            {
                physicalDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
                vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProperties);
            }
            else
            {
                vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties.properties);
            }
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
                queueFamilies.computeQueueFamily = queueFamilies.graphicsQueueFamily;
                compute_queue_index = min(queue_families[queueFamilies.graphicsQueueFamily].queueCount - 1, universal_queue_index);
                universal_queue_index++;
            }

            if (queueFamilies.copyQueueFamily == VK_QUEUE_FAMILY_IGNORED)
            {
                queueFamilies.copyQueueFamily = queueFamilies.graphicsQueueFamily;
                copy_queue_index = min(queue_families[queueFamilies.graphicsQueueFamily].queueCount - 1, universal_queue_index);
                universal_queue_index++;
            }
            else if (queueFamilies.copyQueueFamily == queueFamilies.computeQueueFamily)
            {
                copy_queue_index = min(queue_families[queueFamilies.computeQueueFamily].queueCount - 1, 1u);
            }

            static const float graphics_queue_prio = 0.5f;
            static const float compute_queue_prio = 1.0f;
            static const float transfer_queue_prio = 1.0f;
            float prio[3] = { graphics_queue_prio, compute_queue_prio, transfer_queue_prio };

            uint32_t queueCreateCount = 0;
            VkDeviceQueueCreateInfo queue_info[3] = { };
            queue_info[queueCreateCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_info[queueCreateCount].queueFamilyIndex = queueFamilies.graphicsQueueFamily;
            queue_info[queueCreateCount].queueCount = min(universal_queue_index, queue_families[queueFamilies.graphicsQueueFamily].queueCount);
            queue_info[queueCreateCount].pQueuePriorities = prio;
            queueCreateCount++;

            if (queueFamilies.computeQueueFamily != queueFamilies.graphicsQueueFamily)
            {
                queue_info[queueCreateCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_info[queueCreateCount].queueFamilyIndex = queueFamilies.computeQueueFamily;
                queue_info[queueCreateCount].queueCount = min(queueFamilies.copyQueueFamily == queueFamilies.computeQueueFamily ? 2u : 1u,
                    queue_families[queueFamilies.computeQueueFamily].queueCount);
                queue_info[queueCreateCount].pQueuePriorities = prio + 1;
                queueCreateCount++;
            }

            // Dedicated copy queue
            if (queueFamilies.copyQueueFamily != queueFamilies.graphicsQueueFamily
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


            if (!headless) {
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

            /*

            if (vk.physical_device_features.get_memory_requirements2 &&
                vk.physical_device_features.dedicated_allocation)
            {
                enabled_device_exts[enabled_device_ext_count++] = VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME;
                enabled_device_exts[enabled_device_ext_count++] = VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME;
            }*/

            /*if (vk.physical_device_features.buffer_device_address)
            {
                enabled_device_exts[enabled_device_ext_count++] = VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME;
            }*/

#ifdef _WIN32
            /*if (vk.get_surface_capabilities2 && vk.physical_device_features.win32_full_screen_exclusive)
            {
                enabledDeviceExtensions.push_back(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME);
            }*/
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

            if (deviceApiVersion11 && instanceExts.apiVersion11)
                vkGetPhysicalDeviceFeatures2(physicalDevice, &features);
            else if (instanceExts.get_physical_device_properties2)
                vkGetPhysicalDeviceFeatures2KHR(physicalDevice, &features);
            else
                vkGetPhysicalDeviceFeatures(physicalDevice, &features.features);

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
            createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledDeviceExtensions.size());
            createInfo.ppEnabledExtensionNames = enabledDeviceExtensions.data();

            result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
            if (result != VK_SUCCESS) {
                return;
            }

            volkLoadDeviceTable(&deviceTable, device);

            deviceTable.vkGetDeviceQueue(device, queueFamilies.graphicsQueueFamily, 0, &graphicsQueue);
            deviceTable.vkGetDeviceQueue(device, queueFamilies.computeQueueFamily, compute_queue_index, &computeQueue);
            deviceTable.vkGetDeviceQueue(device, queueFamilies.copyQueueFamily, copy_queue_index, &copyQueue);

            LOG_INFO("Created VkDevice using '%s' adapter with API version: %u.%u.%u",
                physicalDeviceProperties.properties.deviceName,
                VK_VERSION_MAJOR(physicalDeviceProperties.properties.apiVersion),
                VK_VERSION_MINOR(physicalDeviceProperties.properties.apiVersion),
                VK_VERSION_PATCH(physicalDeviceProperties.properties.apiVersion));
            for (uint32_t i = 0; i < createInfo.enabledExtensionCount; ++i) {
                LOG_INFO("Device extension '%s'", createInfo.ppEnabledExtensionNames[i]);
            }
        }
    }

    GraphicsImpl::~GraphicsImpl()
    {
        WaitForGPU();

        if (allocator != VK_NULL_HANDLE)
        {
            VmaStats stats;
            vmaCalculateStats(allocator, &stats);

            if (stats.total.usedBytes > 0) {
                LOG_ERROR("Total device memory leaked: %llx bytes.", stats.total.usedBytes);
            }

            vmaDestroyAllocator(allocator);
        }

        deviceTable.vkDestroyDevice(device, nullptr);

        if (debugUtilsMessenger != VK_NULL_HANDLE)
        {
            vkDestroyDebugUtilsMessengerEXT(instance, debugUtilsMessenger, nullptr);
        }

        if (instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(instance, nullptr);
        }
    }

    void GraphicsImpl::WaitForGPU()
    {
        VK_CHECK(deviceTable.vkDeviceWaitIdle(device));
    }
}
