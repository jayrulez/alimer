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
#include "VulkanTexture.h"
#include "Core/Window.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#endif

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

        std::vector<const char*> GetOptimalValidationLayers(const std::vector<VkLayerProperties>& supportedInstanceLayers)
        {
            std::vector<std::vector<const char*>> validation_layer_priority_list =
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
#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
            return glfwGetPhysicalDevicePresentationSupport(instance, physicalDevice, queueFamilyIndex);
#else
            return true;
#endif
        }


        QueueFamilyIndices QueryQueueFamilies(VkInstance instance, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
        {
            uint32_t queueCount = 0u;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);

            Vector<VkQueueFamilyProperties> queue_families(queueCount);
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queue_families.Data());

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

            Vector<VkExtensionProperties> extensions(count);
            VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, extensions.Data()));

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

        bool IsDeviceSuitable(VkInstance instance, const InstanceExtensions& instanceExts, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
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

        struct SurfaceCapsVk {
            bool success;
            VkSurfaceCapabilitiesKHR capabilities;
            Vector<VkSurfaceFormatKHR> formats;
            Vector<VkPresentModeKHR> presentModes;
        };

        SurfaceCapsVk QuerySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, bool getSurfaceCapabilities2, bool win32_full_screen_exclusive)
        {
            SurfaceCapsVk caps;
            memset(&caps, 0, sizeof(SurfaceCapsVk));

            VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR };
            surfaceInfo.surface = surface;

            if (getSurfaceCapabilities2)
            {
                VkSurfaceCapabilities2KHR surfaceCaps2 = { VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR };

                if (vkGetPhysicalDeviceSurfaceCapabilities2KHR(physicalDevice, &surfaceInfo, &surfaceCaps2) != VK_SUCCESS)
                {
                    return caps;
                }
                caps.capabilities = surfaceCaps2.surfaceCapabilities;

                uint32_t formatCount;
                if (vkGetPhysicalDeviceSurfaceFormats2KHR(physicalDevice, &surfaceInfo, &formatCount, nullptr) != VK_SUCCESS)
                {
                    return caps;
                }

                Vector<VkSurfaceFormat2KHR> formats2(formatCount);

                for (auto& format2 : formats2)
                {
                    format2 = {};
                    format2.sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
                }

                if (vkGetPhysicalDeviceSurfaceFormats2KHR(physicalDevice, &surfaceInfo, &formatCount, formats2.Data()) != VK_SUCCESS)
                {
                    return caps;
                }

                caps.formats.Reserve(formatCount);
                for (auto& format2 : formats2)
                {
                    caps.formats.Push(format2.surfaceFormat);
                }
            }
            else
            {
                if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &caps.capabilities) != VK_SUCCESS)
                {
                    return caps;
                }

                uint32_t formatCount;
                if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr) != VK_SUCCESS)
                {
                    return caps;
                }

                caps.formats.Resize(formatCount);
                if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, caps.formats.Data()) != VK_SUCCESS)
                {
                    return caps;
                }
            }

            uint32_t presentModeCount = 0;
#ifdef _WIN32
            if (getSurfaceCapabilities2 && win32_full_screen_exclusive)
            {
                if (vkGetPhysicalDeviceSurfacePresentModes2EXT(physicalDevice, &surfaceInfo, &presentModeCount, nullptr) != VK_SUCCESS)
                {
                    return caps;
                }

                caps.presentModes.Resize(presentModeCount);
                if (vkGetPhysicalDeviceSurfacePresentModes2EXT(physicalDevice, &surfaceInfo, &presentModeCount, caps.presentModes.Data()) != VK_SUCCESS)
                {
                    return caps;
                }
            }
            else
#endif
            {
                if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr) != VK_SUCCESS)
                {
                    return caps;
                }

                caps.presentModes.Resize(presentModeCount);
                if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, caps.presentModes.Data()) != VK_SUCCESS)
                {
                    return caps;
                }
            }

            caps.success = true;
            return caps;
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

    VulkanGraphicsImpl::VulkanGraphicsImpl(Window* window, GPUFlags flags)
        : Graphics(window)
    {
        ALIMER_VERIFY(IsAvailable());

        // Create instance
        {
            Vector<const char*> enabledInstanceExtensions;
            std::vector<const char*> enabledInstanceLayers;

            uint32_t instanceExtensionCount;
            VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr));

            Vector<VkExtensionProperties> availableInstanceExtensions(instanceExtensionCount);
            VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, availableInstanceExtensions.Data()));
            for (auto& availableExtension : availableInstanceExtensions)
            {
                if (strcmp(availableExtension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
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

            if (window == nullptr)
            {
                enabledInstanceExtensions.Push(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
            }
            else
            {

#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
                uint32_t count;
                const char** ext = glfwGetRequiredInstanceExtensions(&count);
                for (uint32_t i = 0; i < count; i++)
                {
                    enabledInstanceExtensions.Push(ext[i]);
                }
#endif

                if (instanceExts.getSurfaceCapabilities2) {
                    enabledInstanceExtensions.Push(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
                }
            }

            if (any(flags & GPUFlags::DebugRuntime | GPUFlags::GPUBaseValidation))
            {
                if (instanceExts.debugUtils)
                {
                    enabledInstanceExtensions.Push(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                }

                // Layers
                uint32_t instanceLayerCount;
                VK_CHECK(vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr));

                std::vector<VkLayerProperties> availableInstanceLayers(instanceLayerCount);
                VK_CHECK(vkEnumerateInstanceLayerProperties(&instanceLayerCount, availableInstanceLayers.data()));

                // Determine the optimal validation layers to enable that are necessary for useful debugging
                std::vector<const char*> optimalValidationLayers = GetOptimalValidationLayers(availableInstanceLayers);
                enabledInstanceLayers.insert(enabledInstanceLayers.end(), optimalValidationLayers.begin(), optimalValidationLayers.end());
            }

            VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
            appInfo.pApplicationName = "Alimer";
            appInfo.applicationVersion = 0;
            appInfo.pEngineName = "Alimer";
            appInfo.engineVersion = VK_MAKE_VERSION(ALIMER_VERSION_MAJOR, ALIMER_VERSION_MINOR, ALIMER_VERSION_PATCH);
            appInfo.apiVersion = volkGetInstanceVersion();

            VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
            VkDebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };

            if (any(flags & GPUFlags::DebugRuntime | GPUFlags::GPUBaseValidation))
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
            createInfo.enabledExtensionCount = enabledInstanceExtensions.Size();
            createInfo.ppEnabledExtensionNames = enabledInstanceExtensions.Data();

            // Create the Vulkan instance.
            VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

            if (result != VK_SUCCESS)
            {
                VK_LOG_ERROR(result, "Could not create Vulkan instance");
            }

            volkLoadInstance(instance);

            if (any(flags & GPUFlags::DebugRuntime | GPUFlags::GPUBaseValidation))
            {
                if (instanceExts.debugUtils)
                {
                    result = vkCreateDebugUtilsMessengerEXT(instance, &debugUtilsCreateInfo, nullptr, &debugUtilsMessenger);
                    if (result != VK_SUCCESS)
                    {
                        VK_LOG_ERROR(result, "Could not create debug utils messenger");
                    }
                }
            }

            LOG_INFO("Created VkInstance with version: {}.{}.{}", VK_VERSION_MAJOR(appInfo.apiVersion), VK_VERSION_MINOR(appInfo.apiVersion), VK_VERSION_PATCH(appInfo.apiVersion));
            if (createInfo.enabledLayerCount) {
                for (uint32_t i = 0; i < createInfo.enabledLayerCount; ++i)
                    LOG_INFO("Instance layer '{}'", createInfo.ppEnabledLayerNames[i]);
            }

            for (uint32_t i = 0; i < createInfo.enabledExtensionCount; ++i) {
                LOG_INFO("Instance extension '{}'", createInfo.ppEnabledExtensionNames[i]);
            }
        }

#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
        VK_CHECK(glfwCreateWindowSurface(instance, (GLFWwindow*)window->GetWindow(), nullptr, &surface));
#endif

        // Enumerating and creating devices:
        {
            uint32_t physicalDevicesCount = 0;
            VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, nullptr));

            if (physicalDevicesCount == 0) {
                LOG_ERROR("failed to find GPUs with Vulkan support!");
                assert(0);
            }

            Vector<VkPhysicalDevice> physicalDevices(physicalDevicesCount);
            vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, physicalDevices.Data());

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
                LOG_ERROR("Vulkan: Cannot find suitable physical device.");
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
            Vector<VkQueueFamilyProperties> queue_families(queue_count);
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queue_count, queue_families.Data());

            uint32_t universal_queue_index = 1;
            uint32_t compute_queue_index = 0;
            uint32_t copy_queue_index = 0;

            if (queueFamilies.computeQueueFamily == VK_QUEUE_FAMILY_IGNORED)
            {
                queueFamilies.computeQueueFamily = queueFamilies.graphicsQueueFamily;
                compute_queue_index = Min(queue_families[queueFamilies.graphicsQueueFamily].queueCount - 1, universal_queue_index);
                universal_queue_index++;
            }

            if (queueFamilies.copyQueueFamily == VK_QUEUE_FAMILY_IGNORED)
            {
                queueFamilies.copyQueueFamily = queueFamilies.graphicsQueueFamily;
                copy_queue_index = Min(queue_families[queueFamilies.graphicsQueueFamily].queueCount - 1, universal_queue_index);
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
            queue_info[queueCreateCount].queueFamilyIndex = queueFamilies.graphicsQueueFamily;
            queue_info[queueCreateCount].queueCount = Min(universal_queue_index, queue_families[queueFamilies.graphicsQueueFamily].queueCount);
            queue_info[queueCreateCount].pQueuePriorities = prio;
            queueCreateCount++;

            if (queueFamilies.computeQueueFamily != queueFamilies.graphicsQueueFamily)
            {
                queue_info[queueCreateCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_info[queueCreateCount].queueFamilyIndex = queueFamilies.computeQueueFamily;
                queue_info[queueCreateCount].queueCount = Min(queueFamilies.copyQueueFamily == queueFamilies.computeQueueFamily ? 2u : 1u,
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
            Vector<const char*> enabledDeviceExtensions;

            if (surface != VK_NULL_HANDLE) {
                enabledDeviceExtensions.Push(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
            }

            if (!deviceApiVersion11) {
                if (physicalDeviceExts.maintenance_1)
                {
                    enabledDeviceExtensions.Push("VK_KHR_maintenance1");
                }

                if (physicalDeviceExts.maintenance_2) {
                    enabledDeviceExtensions.Push("VK_KHR_maintenance2");
                }

                if (physicalDeviceExts.maintenance_3) {
                    enabledDeviceExtensions.Push("VK_KHR_maintenance3");
                }
            }

            if (physicalDeviceExts.image_format_list)
            {
                enabledDeviceExtensions.Push(VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME);
            }

            if (physicalDeviceExts.sampler_mirror_clamp_to_edge)
            {
                enabledDeviceExtensions.Push(VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME);
            }

            if (physicalDeviceExts.depth_clip_enable)
            {
                enabledDeviceExtensions.Push(VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME);
            }

            /*if (vk.physical_device_features.buffer_device_address)
            {
                enabled_device_exts[enabled_device_ext_count++] = VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME;
            }*/

#ifdef _WIN32
            if (instanceExts.getSurfaceCapabilities2 && physicalDeviceExts.win32_full_screen_exclusive)
            {
                enabledDeviceExtensions.Push(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME);
            }
#endif

            VkPhysicalDeviceFeatures2 features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
            VkPhysicalDeviceMultiviewFeatures multiview_features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES };
            void** ppNext = &features.pNext;

            if (physicalDeviceExts.multiview)
            {
                if (!deviceApiVersion11) {
                    enabledDeviceExtensions.Push("VK_KHR_multiview");
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
            createInfo.enabledExtensionCount = enabledDeviceExtensions.Size();
            createInfo.ppEnabledExtensionNames = enabledDeviceExtensions.Data();

            VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
            if (result != VK_SUCCESS) {
                VK_LOG_ERROR(result, "Failed to create device");
                return;
            }

            volkLoadDeviceTable(&deviceTable, device);

            deviceTable.vkGetDeviceQueue(device, queueFamilies.graphicsQueueFamily, 0, &graphicsQueue);
            deviceTable.vkGetDeviceQueue(device, queueFamilies.computeQueueFamily, compute_queue_index, &computeQueue);
            deviceTable.vkGetDeviceQueue(device, queueFamilies.copyQueueFamily, copy_queue_index, &copyQueue);

            LOG_INFO("Created VkDevice using '{}' adapter with API version: {}.{}.{}",
                physicalDeviceProperties.properties.deviceName,
                VK_VERSION_MAJOR(physicalDeviceProperties.properties.apiVersion),
                VK_VERSION_MINOR(physicalDeviceProperties.properties.apiVersion),
                VK_VERSION_PATCH(physicalDeviceProperties.properties.apiVersion));
            for (uint32_t i = 0; i < createInfo.enabledExtensionCount; ++i) {
                LOG_INFO("Device extension '{}'", createInfo.ppEnabledExtensionNames[i]);
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
    }

    VulkanGraphicsImpl::~VulkanGraphicsImpl()
    {
        WaitForGPU();
        Shutdown();
    }

    void VulkanGraphicsImpl::Shutdown()
    {
        deviceTable.vkDestroySwapchainKHR(device, swapchain, nullptr);

        if (memoryAllocator != VK_NULL_HANDLE)
        {
            VmaStats stats;
            vmaCalculateStats(memoryAllocator, &stats);

            if (stats.total.usedBytes > 0) {
                LOG_ERROR("Total device memory leaked: {} bytes.", stats.total.usedBytes);
            }

            vmaDestroyAllocator(memoryAllocator);
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

    void VulkanGraphicsImpl::InitCapabilities()
    {
        caps.backendType = GPU::BackendType::Vulkan;
        caps.vendorId = physicalDeviceProperties.properties.vendorID;
        caps.deviceId = physicalDeviceProperties.properties.deviceID;
        caps.adapterName = physicalDeviceProperties.properties.deviceName;

        switch (physicalDeviceProperties.properties.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            caps.adapterType = GPU::AdapterType::IntegratedGPU;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            caps.adapterType = GPU::AdapterType::DiscreteGPU;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            caps.adapterType = GPU::AdapterType::CPU;
            break;
        default:
            caps.adapterType = GPU::AdapterType::Unknown;
            break;
        }
    }

    bool VulkanGraphicsImpl::UpdateSwapchain()
    {
        WaitForGPU();

        SurfaceCapsVk surfaceCaps = QuerySwapchainSupport(physicalDevice, surface, instanceExts.getSurfaceCapabilities2, physicalDeviceExts.win32_full_screen_exclusive);

        /* Detect image count. */
        uint32_t imageCount = backbufferCount;
        if (imageCount == 0)
        {
            imageCount = surfaceCaps.capabilities.minImageCount + 1;
            if ((surfaceCaps.capabilities.maxImageCount > 0) &&
                (imageCount > surfaceCaps.capabilities.maxImageCount))
            {
                imageCount = surfaceCaps.capabilities.maxImageCount;
            }
        }
        else
        {
            if (surfaceCaps.capabilities.maxImageCount != 0)
                imageCount = Min(imageCount, surfaceCaps.capabilities.maxImageCount);
            imageCount = Max(imageCount, surfaceCaps.capabilities.minImageCount);
        }

        /* Surface format. */
        VkSurfaceFormatKHR format;
        if (surfaceCaps.formats.Size() == 1 &&
            surfaceCaps.formats[0].format == VK_FORMAT_UNDEFINED)
        {
            format = surfaceCaps.formats[0];
            format.format = VK_FORMAT_B8G8R8A8_UNORM;
        }
        else
        {
            if (surfaceCaps.formats.Size() == 0)
            {
                //vgpu_log(VGPU_LOG_LEVEL_ERROR, "Vulkan: Surface has no formats.");
                return false;
            }

            const bool srgb = false;
            bool found = false;
            for (uint32_t i = 0; i < surfaceCaps.formats.Size(); i++)
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
        VkExtent2D swapchainSize = { backbufferWidth, backbufferHeight };
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
        if (!vSync)
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

        VkResult result = deviceTable.vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain);
        if (result != VK_SUCCESS) {
            return false;
        }

        if (oldSwapchain != VK_NULL_HANDLE)
        {
            deviceTable.vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
        }

        deviceTable.vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
        Vector<VkImage> swapChainImages(imageCount);
        deviceTable.vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapChainImages.Data());

        SetObjectName(VK_OBJECT_TYPE_SWAPCHAIN_KHR, (uint64_t)swapchain, "Swapchain");

        for (uint32 i = 0; i < imageCount; i++)
        {
            backbufferTextures[backbufferIndex] = new VulkanTexture(this, swapChainImages[i]);
            backbufferTextures[backbufferIndex]->SetName(fmt::format("Back Buffer {}", i));
            //SetObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)swapChainImages[i], fmt::format("Back Buffer {}", i));
        }

        return true;
    }

    void VulkanGraphicsImpl::SetObjectName(VkObjectType type, uint64_t handle, const std::string& name)
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
        VK_CHECK(deviceTable.vkDeviceWaitIdle(device));
    }

    bool VulkanGraphicsImpl::BeginFrame()
    {
        return true;
    }

    void VulkanGraphicsImpl::EndFrame()
    {
        /*VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &semaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &vk_swapchain;
        presentInfo.pImageIndices = &active_frame_index;

        VkResult result = queue.present(present_info);

        if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            handle_surface_changes();
        }*/
    }

    Texture* VulkanGraphicsImpl::GetBackbufferTexture() const
    {
        return backbufferTextures[backbufferIndex].Get();
    }
}
