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

#include "GraphicsDeviceVK.h"
#include "GraphicsContextVK.h"
#include "TextureVK.h"
#include "core/Utils.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "math/math.h"
#include <algorithm>
#include <map>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

using namespace std;

namespace alimer
{
    namespace
    {
        VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
            const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
            void* user_data)
        {
            // Log debug messge
            if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            {
                ALIMER_LOGW("%u - %s: %s", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
            }
            else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            {
                ALIMER_LOGE("%u - %s: %s", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
            }
            return VK_FALSE;
        }

        static bool HasLayers(const std::vector<const char*>& required, const std::vector<VkLayerProperties>& available)
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
                    return false;
                }
            }

            return true;
        }

        std::vector<const char*> GetOptimalValidationLayers(const std::vector<VkLayerProperties>& supported_instance_layers)
        {
            std::vector<std::vector<const char*>> validationLayerPriorityList =
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
                {"VK_LAYER_LUNARG_core_validation"}
            };

            for (auto& validation_layers : validationLayerPriorityList)
            {
                if (HasLayers(validation_layers, supported_instance_layers))
                {
                    return validation_layers;
                }

                ALIMER_LOGW("Couldn't enable validation layers (see log for error) - falling back");
            }

            // Else return nothing
            return {};
        }

        PhysicalDeviceExtensions CheckDeviceExtensionSupport(VkPhysicalDevice device) {
            uint32_t extensionCount;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

            std::vector<VkExtensionProperties> availableExtensions(extensionCount);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

            PhysicalDeviceExtensions exts = {};
            for (const auto& extension : availableExtensions) {
                if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
                    exts.swapchain = true;
                }
                else if (strcmp(extension.extensionName, VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME) == 0) {
                    exts.depth_clip_enable = true;
                }
                else if (strcmp(extension.extensionName, VK_KHR_MAINTENANCE1_EXTENSION_NAME) == 0) {
                    exts.maintenance_1 = true;
                }
                else if (strcmp(extension.extensionName, VK_KHR_MAINTENANCE2_EXTENSION_NAME) == 0) {
                    exts.maintenance_2 = true;
                }
                else if (strcmp(extension.extensionName, VK_KHR_MAINTENANCE3_EXTENSION_NAME) == 0) {
                    exts.maintenance_3 = true;
                }
                else if (strcmp(extension.extensionName, VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME) == 0) {
                    exts.KHR_get_memory_requirements2 = true;
                }
                else if (strcmp(extension.extensionName, VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME) == 0) {
                    exts.KHR_dedicated_allocation = true;
                }
                else if (strcmp(extension.extensionName, VK_KHR_BIND_MEMORY_2_EXTENSION_NAME) == 0) {
                    exts.KHR_bind_memory2 = true;
                }
                else if (strcmp(extension.extensionName, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME) == 0) {
                    exts.EXT_memory_budget = true;
                }
                else if (strcmp(extension.extensionName, VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME) == 0) {
                    exts.image_format_list = true;
                }
                else if (strcmp(extension.extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0) {
                    exts.debug_marker = true;
                }
                else if (strcmp(extension.extensionName, VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME) == 0) {
                    exts.win32_full_screen_exclusive = true;
                }
                else if (strcmp(extension.extensionName, VK_KHR_RAY_TRACING_EXTENSION_NAME) == 0) {
                    exts.raytracing = true;
                }
                else if (strcmp(extension.extensionName, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) == 0) {
                    exts.buffer_device_address = true;
                }
                else if (strcmp(extension.extensionName, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME) == 0) {
                    exts.deferred_host_operations = true;
                }
                else if (strcmp(extension.extensionName, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME) == 0) {
                    exts.descriptor_indexing = true;
                }
                else if (strcmp(extension.extensionName, VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME) == 0) {
                    exts.pipeline_library = true;
                }
            }

            return exts;
        }


        int32_t RatePhysicalDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
        {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

            VkPhysicalDeviceFeatures deviceFeatures;
            vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

            PhysicalDeviceExtensions exts = CheckDeviceExtensionSupport(physicalDevice);
            if (!exts.swapchain || !exts.maintenance_1)
            {
                return 0;
            }

            QueueFamilyIndices indices = FindQueueFamilies(physicalDevice, surface);

            if (!indices.IsComplete())
            {
                return 0;
            }

            int32_t score = 0;
            if (deviceProperties.apiVersion >= VK_API_VERSION_1_2) {
                score += 10000u;
            }
            else if (deviceProperties.apiVersion >= VK_API_VERSION_1_1) {
                score += 5000u;
            }

            switch (deviceProperties.deviceType) {
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

            return score;
        }

    }

    bool GraphicsDeviceVK::IsAvailable()
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

    GraphicsDeviceVK::GraphicsDeviceVK()
        : GraphicsDevice()
    {
    }

    GraphicsDeviceVK::~GraphicsDeviceVK()
    {
        waitForIdle();
        destroy();
    }

    bool GraphicsDeviceVK::Init(window_t* window, const GraphicsDeviceDesc& desc)
    {
        if (!IsAvailable()) {
            return false;
        }

        const bool headless = window == nullptr;
        if (!InitInstance(desc, headless)) {
            return false;
        }

        VkSurfaceKHR surface = VK_NULL_HANDLE;
        if (window != nullptr) {
            surface = createSurface(window_handle(window));
        }

        if (!InitPhysicalDevice(surface)) {
            ALIMER_LOGERROR("[Vulkan]: Cannot detect suitable physical device");
            return false;
        }

        if (!InitLogicalDevice(desc)) {
            return false;
        }

        if (!InitMemoryAllocator()) {
            return false;
        }

        // Create main context.
        mainContext = std::make_unique<GraphicsContextVK>(this, surface, window_width(window), window_height(window));

        for (uint32_t i = 0; i < maxInflightFrames; i++)
        {
            frames.push_back(std::make_unique<Frame>(this));
        }

        return true;
    }

    bool GraphicsDeviceVK::InitInstance(const GraphicsDeviceDesc& desc, bool headless)
    {
        const uint32_t apiVersion = volkGetInstanceVersion();

        VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
        appInfo.pApplicationName = desc.applicationName;
        appInfo.applicationVersion = 0;
        appInfo.pEngineName = "Alimer";
        appInfo.engineVersion = 0;
        appInfo.apiVersion = VK_API_VERSION_1_1;
        if (apiVersion >= VK_API_VERSION_1_2) {
            appInfo.apiVersion = VK_API_VERSION_1_2;
        }

        // Enumerate supported extensions and setup instance extensions.
        std::vector<const char*> enabledExtensions;
        uint32_t instanceExtensionCount;
        VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr));

        std::vector<VkExtensionProperties> availableInstanceExtensions(instanceExtensionCount);
        VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, availableInstanceExtensions.data()));

        for (auto& available_extension : availableInstanceExtensions)
        {
            if (strcmp(available_extension.extensionName, "VK_KHR_get_physical_device_properties2") == 0)
            {
                vk_features.physicalDeviceProperties2 = true;
                enabledExtensions.push_back("VK_KHR_get_physical_device_properties2");
            }
            else if (strcmp(available_extension.extensionName, "VK_KHR_external_memory_capabilities") == 0)
            {
                vk_features.externalMemoryCapabilities = true;
            }
            else if (strcmp(available_extension.extensionName, "VK_KHR_external_semaphore_capabilities") == 0)
            {
                vk_features.externalSemaphoreCapabilities = true;
            }
            else if (strcmp(available_extension.extensionName, "VK_EXT_debug_utils") == 0)
            {
                vk_features.debugUtils = true;
            }
            else if (strcmp(available_extension.extensionName, "VK_EXT_headless_surface") == 0)
            {
                vk_features.headless = true;
            }
            else if (strcmp(available_extension.extensionName, "VK_KHR_surface"))
            {
                vk_features.surface = true;
            }
            else if (strcmp(available_extension.extensionName, "VK_KHR_get_surface_capabilities2"))
            {
                vk_features.surfaceCapabilities2 = true;
            }
        }

        if (vk_features.physicalDeviceProperties2 &&
            vk_features.externalMemoryCapabilities &&
            vk_features.externalSemaphoreCapabilities)
        {
            enabledExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
            enabledExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
        }

        if (vk_features.debugUtils)
        {
            enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        // Try to enable headless surface extension if it exists
        if (headless)
        {
            if (!vk_features.headless)
            {
                ALIMER_LOGW("%s is not available, disabling swapchain creation", VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
            }
            else
            {
                ALIMER_LOGI("%s is available, enabling it", VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
                enabledExtensions.push_back(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
            }
        }
        else
        {
            enabledExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
            // Enable surface extensions depending on os
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
            enabledExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
            enabledExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(_DIRECT2DISPLAY)
            enabledExtensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
            enabledExtensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
            enabledExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_IOS_MVK)
            enabledExtensions.push_back(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
                // TODO: Support VK_EXT_metal_surface
            enabledExtensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#endif

            if (vk_features.surfaceCapabilities2)
            {
                enabledExtensions.push_back("VK_KHR_get_surface_capabilities2");
            }
        }

        const bool validation = any(desc.flags & GraphicsDeviceFlags::Debug);
        std::vector<const char*> enabledLayers;

        if (validation)
        {
            uint32_t layerCount;
            VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

            std::vector<VkLayerProperties> queriedLayers(layerCount);
            VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, queriedLayers.data()));

            std::vector<const char*> optimalValidationLayers = GetOptimalValidationLayers(queriedLayers);
            enabledLayers.insert(enabledLayers.end(), optimalValidationLayers.begin(), optimalValidationLayers.end());
        }

        VkInstanceCreateInfo instanceInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        instanceInfo.pApplicationInfo = &appInfo;
        instanceInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
        instanceInfo.ppEnabledLayerNames = enabledLayers.data();
        instanceInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
        instanceInfo.ppEnabledExtensionNames = enabledExtensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
        VkDebugReportCallbackCreateInfoEXT debugReportCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT };
        if (vk_features.debugUtils)
        {
            debugUtilsCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
            debugUtilsCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
            debugUtilsCreateInfo.pfnUserCallback = DebugUtilsMessengerCallback;

            instanceInfo.pNext = &debugUtilsCreateInfo;
        }

        // Create the Vulkan instance
        VkResult result = vkCreateInstance(&instanceInfo, nullptr, &instance);

        if (result != VK_SUCCESS)
        {
            VK_THROW(result, "Could not create Vulkan instance");
        }

        volkLoadInstance(instance);

        ALIMER_LOGI("Created VkInstance with version: %u.%u.%u", VK_VERSION_MAJOR(appInfo.apiVersion), VK_VERSION_MINOR(appInfo.apiVersion), VK_VERSION_PATCH(appInfo.apiVersion));
        if (enabledLayers.size()) {
            for (auto* layer_name : enabledLayers)
                ALIMER_LOGI("Instance layer '%s'", layer_name);
        }

        for (auto* ext_name : enabledExtensions)
            ALIMER_LOGI("Instance extension '%s'", ext_name);

        if (vk_features.debugUtils)
        {
            result = vkCreateDebugUtilsMessengerEXT(instance, &debugUtilsCreateInfo, nullptr, &debugUtilsMessenger);
            if (result != VK_SUCCESS)
            {
                ALIMER_LOGE("Could not create debug utils messenger");
            }
        }

        return true;
    }

    bool GraphicsDeviceVK::InitPhysicalDevice(VkSurfaceKHR surface)
    {
        uint32_t deviceCount;
        if (vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr) != VK_SUCCESS ||
            deviceCount == 0)
        {
            return false;
        }

        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        if (vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data()) != VK_SUCCESS) {
            return false;
        }

        std::multimap<int32_t, VkPhysicalDevice> physicalDeviceCandidates;
        for (VkPhysicalDevice physicalDevice : physicalDevices)
        {
            int32_t score = RatePhysicalDevice(physicalDevice, surface);
            physicalDeviceCandidates.insert(std::make_pair(score, physicalDevice));
        }

        // Check if the best candidate is suitable at all
        if (physicalDeviceCandidates.rbegin()->first <= 0)
        {
            ALIMER_LOGERROR("[Vulkan]: Failed to find a suitable GPU!");
            return false;
        }

        physicalDevice = physicalDeviceCandidates.rbegin()->second;
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

        // Store the properties of each queuefamily
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        queueFamilyProperties.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

        queueFamilyIndices = FindQueueFamilies(physicalDevice, surface);
        physicalDeviceExts = CheckDeviceExtensionSupport(physicalDevice);

        ALIMER_TRACE("Physical device:");
        ALIMER_TRACE("\t          Name: %s", physicalDeviceProperties.deviceName);
        ALIMER_TRACE("\t   API version: %x", physicalDeviceProperties.apiVersion);
        ALIMER_TRACE("\tDriver version: %x", physicalDeviceProperties.driverVersion);
        ALIMER_TRACE("\t      VendorId: %x", physicalDeviceProperties.vendorID);
        ALIMER_TRACE("\t      DeviceId: %x", physicalDeviceProperties.deviceID);
        ALIMER_TRACE("\t          Type: %d", physicalDeviceProperties.deviceType);

        return true;
    }

    bool GraphicsDeviceVK::InitLogicalDevice(const GraphicsDeviceDesc& desc)
    {
        // Setup queues first.
        uint32_t universal_queue_index = 1;
        const uint32_t graphicsQueueIndex = 0;
        uint32_t computeQueueIndex = 0;
        uint32_t copyQueueIndex = 0;

        if (queueFamilyIndices.computeFamily == VK_QUEUE_FAMILY_IGNORED)
        {
            queueFamilyIndices.computeFamily = queueFamilyIndices.graphicsFamily;
            computeQueueIndex = min(queueFamilyProperties[queueFamilyIndices.graphicsFamily].queueCount - 1, universal_queue_index);
            universal_queue_index++;
        }

        if (queueFamilyIndices.transferFamily == VK_QUEUE_FAMILY_IGNORED)
        {
            queueFamilyIndices.transferFamily = queueFamilyIndices.graphicsFamily;
            copyQueueIndex = min(queueFamilyProperties[queueFamilyIndices.graphicsFamily].queueCount - 1, universal_queue_index);
            universal_queue_index++;
        }
        else if (queueFamilyIndices.transferFamily == queueFamilyIndices.computeFamily)
        {
            copyQueueIndex = min(queueFamilyProperties[queueFamilyIndices.computeFamily].queueCount - 1, 1u);
        }

        static const float graphics_queue_prio = 0.5f;
        static const float compute_queue_prio = 1.0f;
        static const float transfer_queue_prio = 1.0f;
        float prio[3] = { graphics_queue_prio, compute_queue_prio, transfer_queue_prio };

        uint32_t queueFamilyCount = 0;
        VkDeviceQueueCreateInfo queueCreateInfo[3] = {};

        queueCreateInfo[queueFamilyCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo[queueFamilyCount].queueFamilyIndex = queueFamilyIndices.graphicsFamily;
        queueCreateInfo[queueFamilyCount].queueCount = min(universal_queue_index, queueFamilyProperties[queueFamilyIndices.graphicsFamily].queueCount);
        queueCreateInfo[queueFamilyCount].pQueuePriorities = prio;
        queueFamilyCount++;

        if (queueFamilyIndices.computeFamily != queueFamilyIndices.graphicsFamily)
        {
            queueCreateInfo[queueFamilyCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo[queueFamilyCount].queueFamilyIndex = queueFamilyIndices.computeFamily;
            queueCreateInfo[queueFamilyCount].queueCount = min(queueFamilyIndices.transferFamily == queueFamilyIndices.computeFamily ? 2u : 1u, queueFamilyProperties[queueFamilyIndices.computeFamily].queueCount);
            queueCreateInfo[queueFamilyCount].pQueuePriorities = prio + 1;
            queueFamilyCount++;
        }

        if (queueFamilyIndices.transferFamily != queueFamilyIndices.graphicsFamily
            && queueFamilyIndices.transferFamily != queueFamilyIndices.computeFamily)
        {
            queueCreateInfo[queueFamilyCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo[queueFamilyCount].queueFamilyIndex = queueFamilyIndices.transferFamily;
            queueCreateInfo[queueFamilyCount].queueCount = 1;
            queueCreateInfo[queueFamilyCount].pQueuePriorities = prio + 2;
            queueFamilyCount++;
        }

        std::vector<const char*> enabledExtensions;

        if (!vk_features.headless)
            enabledExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        if (physicalDeviceExts.KHR_get_memory_requirements2)
        {
            enabledExtensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
        }

        if (physicalDeviceExts.KHR_get_memory_requirements2
            && physicalDeviceExts.KHR_dedicated_allocation)
        {
            enabledExtensions.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
        }

        if (physicalDeviceProperties.apiVersion >= VK_API_VERSION_1_1
            || physicalDeviceExts.KHR_bind_memory2)
        {
            enabledExtensions.push_back(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME);
        }

        if (physicalDeviceProperties.apiVersion >= VK_API_VERSION_1_1
            || physicalDeviceExts.EXT_memory_budget)
        {
            enabledExtensions.push_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
        }

        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.queueCreateInfoCount = queueFamilyCount;
        createInfo.pQueueCreateInfos = queueCreateInfo;
        createInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
        createInfo.ppEnabledExtensionNames = enabledExtensions.data();

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &handle) != VK_SUCCESS)
        {
            return false;
        }

        ALIMER_LOGI("Created VkDevice with adapter '%s' API version: %u.%u.%u", physicalDeviceProperties.deviceName, VK_VERSION_MAJOR(physicalDeviceProperties.apiVersion), VK_VERSION_MINOR(physicalDeviceProperties.apiVersion), VK_VERSION_PATCH(physicalDeviceProperties.apiVersion));
        for (auto* ext_name : enabledExtensions)
        {
            ALIMER_LOGI("Device extension '%s'", ext_name);
        }

        volkLoadDeviceTable(&deviceTable, handle);
        deviceTable.vkGetDeviceQueue(handle, queueFamilyIndices.graphicsFamily, graphicsQueueIndex, &graphicsQueue);
        deviceTable.vkGetDeviceQueue(handle, queueFamilyIndices.computeFamily, computeQueueIndex, &computeQueue);
        deviceTable.vkGetDeviceQueue(handle, queueFamilyIndices.transferFamily, copyQueueIndex, &copyQueue);

        return true;
    }

    bool GraphicsDeviceVK::InitMemoryAllocator()
    {
        VmaAllocatorCreateInfo createInfo{};
        createInfo.physicalDevice = physicalDevice;
        createInfo.device = handle;
        createInfo.instance = instance;

        if (physicalDeviceExts.KHR_get_memory_requirements2
            && physicalDeviceExts.KHR_dedicated_allocation)
        {
            createInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
        }

        if (physicalDeviceExts.KHR_bind_memory2)
        {
            createInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;
        }

        if (physicalDeviceExts.EXT_memory_budget)
        {
            createInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
        }

        VkResult result = vmaCreateAllocator(&createInfo, &memoryAllocator);
        if (result != VK_SUCCESS)
        {
            VK_THROW(result, "Cannot create allocator");
            return false;
        }

        return true;
    }

    void GraphicsDeviceVK::destroy()
    {
        if (instance == VK_NULL_HANDLE) {
            return;
        }

        mainContext.reset();

        if (memoryAllocator != VK_NULL_HANDLE)
        {
            VmaStats stats;
            vmaCalculateStats(memoryAllocator, &stats);

            if (stats.total.usedBytes > 0) {
                ALIMER_LOGI("Total device memory leaked: {} bytes.", stats.total.usedBytes);
            }

            vmaDestroyAllocator(memoryAllocator);
        }

        if (handle != VK_NULL_HANDLE) {
            vkDestroyDevice(handle, nullptr);
        }

        if (debugUtilsMessenger != VK_NULL_HANDLE) {
            vkDestroyDebugUtilsMessengerEXT(instance, debugUtilsMessenger, nullptr);
        }

        vkDestroyInstance(instance, nullptr);
        instance = VK_NULL_HANDLE;
    }

    VkSurfaceKHR GraphicsDeviceVK::createSurface(uintptr_t handle)
    {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkResult result = VK_ERROR_UNKNOWN;

#if defined(VK_USE_PLATFORM_WIN32_KHR)
        VkWin32SurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
        createInfo.hinstance = GetModuleHandle(NULL);
        createInfo.hwnd = reinterpret_cast<HWND>(handle);
        result = vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface);
#endif

        if (result != VK_SUCCESS)
        {
            ALIMER_LOGERROR("Failed to create surface for SwapChain");
            return VK_NULL_HANDLE;
        }

        return surface;
    }

    void GraphicsDeviceVK::SetObjectName(VkObjectType objectType, uint64_t objectHandle, const char* objectName)
    {
        if (!vk_features.debugUtils)
        {
            return;
        }

        VkDebugUtilsObjectNameInfoEXT info;
        info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        info.pNext = nullptr;
        info.objectType = objectType;
        info.objectHandle = objectHandle;
        info.pObjectName = objectName;
        vkSetDebugUtilsObjectNameEXT(handle, &info);
    }

    void GraphicsDeviceVK::waitForIdle()
    {
        VK_CHECK(vkDeviceWaitIdle(handle));
    }

    GraphicsContext& GraphicsDeviceVK::getMainContext() const
    {
        return *mainContext.get();
    }

    RefPtr<Texture> GraphicsDeviceVK::CreateTexture(const TextureDesc* pDesc, const void* initialData)
    {
        ALIMER_ASSERT(pDesc != nullptr);

        TextureVK* texture = new TextureVK(this);
        if (!texture->Init(pDesc, initialData))
        {
            texture->Destroy();
            return nullptr;
        }

        return RefPtr<Texture>(texture);
    }

    VkSemaphore GraphicsDeviceVK::RequestSemaphore()
    {
        return frame().syncPool.RequestSemaphore();
    }

    VkFence GraphicsDeviceVK::RequestFence()
    {
        return frame().syncPool.RequestFence();
    }

    /* Frame */
    GraphicsDeviceVK::Frame::Frame(GraphicsDeviceVK* device)
        : syncPool(*device)
        //, commandPool(new CommandPoolVK(device, device->graphicsQueue, device->queueFamilyIndices.graphicsFamily))
    {

    }

    GraphicsDeviceVK::Frame::~Frame()
    {
        Begin();
    }

    void GraphicsDeviceVK::Frame::Begin()
    {
        VK_CHECK(syncPool.Wait());
        syncPool.Reset();
        //commandPool->Reset();
    }
}
