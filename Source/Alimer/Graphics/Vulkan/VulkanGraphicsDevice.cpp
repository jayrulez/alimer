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
#include "VulkanGraphicsDevice.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

using namespace std;

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
            for (uint32_t i = 0; i < count; ++i)
            {
                if (strcmp(extensions[i].extensionName, VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME) == 0) {
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
                else if (strcmp(extensions[i].extensionName, "VK_KHR_dedicated_allocation") == 0) {
                    result.dedicated_allocation = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_KHR_bind_memory2") == 0) {
                    result.bind_memory2 = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_EXT_memory_budget") == 0) {
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

            /* We required maintenance_1 to support viewport flipping to match DX style. */
            if (!features.maintenance_1) {
                return false;
            }

            return true;
        }

        struct SwapChainSupportDetails {
            VkSurfaceCapabilitiesKHR capabilities;
            vector<VkSurfaceFormatKHR> formats;
            vector<VkPresentModeKHR> presentModes;
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

        VkAttachmentLoadOp VulkanAttachmentLoadOp(LoadAction action)
        {
            switch (action) {
            case LoadAction::DontCare:
                return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            case LoadAction::Load:
                return VK_ATTACHMENT_LOAD_OP_LOAD;
            case LoadAction::Clear:
                return VK_ATTACHMENT_LOAD_OP_CLEAR;
            default:
                ALIMER_UNREACHABLE();
            }
        }
    }

    bool VulkanGraphicsDevice::IsAvailable()
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

    VulkanGraphicsDevice::VulkanGraphicsDevice(const String& appName, const GPUDeviceDescriptor& descriptor)
        : GraphicsDevice(GPUBackendType::Vulkan)
    {
        ALIMER_VERIFY(IsAvailable());

        // Create instance.
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

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
                enabledExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
                enabledExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
                enabledExtensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
                enabledExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)
                enabledExtensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
                enabledExtensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#else
#	pragma error Platform not supported
#endif

                if (instanceExts.get_surface_capabilities2)
                {
                    enabledExtensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
                }
            }

            VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
            appInfo.pApplicationName = appName.c_str();
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
            if (createInfo.enabledLayerCount)
            {
                for (uint32_t i = 0; i < createInfo.enabledLayerCount; ++i)
                    LOGI("Instance layer '{}'", createInfo.ppEnabledLayerNames[i]);
            }

            for (uint32_t i = 0; i < createInfo.enabledExtensionCount; ++i)
            {
                LOGI("Instance extension '{}'", createInfo.ppEnabledExtensionNames[i]);
            }
        }

        if (!InitSurface(descriptor.swapChain.handle)) {
            return;
        }

        if (!InitPhysicalDevice(descriptor.powerPreference)) {
            return;
        }

        if (!InitLogicalDevice()) {
            return;
        }

        InitCapabilities();

    }

    VulkanGraphicsDevice::~VulkanGraphicsDevice()
    {
        Shutdown();
    }

    void VulkanGraphicsDevice::Shutdown()
    {
        vkDeviceWaitIdle(device);

        for (auto& per_frame : frame)
        {
            TeardownPerFrame(per_frame);
        }
        frame.clear();

        for (auto semaphore : recycledSemaphores)
        {
            vkDestroySemaphore(device, semaphore, nullptr);
        }

        if (swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(device, swapchain, nullptr);
            swapchain = VK_NULL_HANDLE;
        }

        if (surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(instance, surface, nullptr);
            surface = VK_NULL_HANDLE;
        }

        // Clear caches
        ClearRenderPassCache();
        ClearFramebufferCache();

        if (allocator != VK_NULL_HANDLE)
        {
            VmaStats stats;
            vmaCalculateStats(allocator, &stats);

            if (stats.total.usedBytes > 0) {
                LOGE("Total device memory leaked: {} bytes.", stats.total.usedBytes);
            }

            vmaDestroyAllocator(allocator);
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

    GPUAdapter* VulkanGraphicsDevice::GetAdapter() const
    {
        return adapter;
    }

    GPUContext* VulkanGraphicsDevice::GetMainContext() const
    {
        return nullptr;
    }

    GPUSwapChain* VulkanGraphicsDevice::GetMainSwapChain() const
    {
        return nullptr;
    }

    void VulkanGraphicsDevice::InitCapabilities()
    {
        /*caps.rendererType = RendererType::Vulkan;
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
        }*/
    }

    bool VulkanGraphicsDevice::UpdateSwapchain()
    {
        if (swapchain != VK_NULL_HANDLE)
        {
            WaitForGPU();
        }


        SwapChainSupportDetails surfaceCaps = QuerySwapchainSupport(adapter->GetHandle(), surface, instanceExts.get_surface_capabilities2, physicalDeviceExts.win32_full_screen_exclusive);

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
                LOGE("Vulkan: Surface has no formats.");
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
        VkExtent2D swapchainSize = {}; // { backbufferSize.x, backbufferSize.y };
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

        VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
        const bool verticalSync = true;
        if (!verticalSync)
        {
            // The immediate present mode is not necessarily supported:
            for (auto& presentMode : surfaceCaps.presentModes)
            {
                if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                    break;
                }
                if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) && (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR))
                {
                    swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
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
        createInfo.presentMode = swapchainPresentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = oldSwapchain;

        VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain);
        if (result != VK_SUCCESS) {
            return false;
        }

        if (oldSwapchain != VK_NULL_HANDLE)
        {
            for (VkImageView view : swapchainImageViews)
            {
                vkDestroyImageView(device, view, nullptr);
            }

            VK_CHECK(vkGetSwapchainImagesKHR(device, oldSwapchain, &imageCount, nullptr));

            for (size_t i = 0; i < imageCount; i++)
            {
                TeardownPerFrame(frame[i]);
            }

            swapchainImageViews.clear();
            vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
        }

        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
        std::vector<VkImage> swapchainImages(imageCount);
        swapChainImageLayouts.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

        frame.clear();
        frame.resize(imageCount);

        for (uint32_t i = 0; i < imageCount; i++)
        {
            swapChainImageLayouts[i] = VK_IMAGE_LAYOUT_UNDEFINED;

            VkImageViewCreateInfo view_info{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
            view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view_info.format = format.format;
            view_info.image = swapchainImages[i];
            view_info.subresourceRange.levelCount = 1;
            view_info.subresourceRange.layerCount = 1;
            view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            view_info.components.r = VK_COMPONENT_SWIZZLE_R;
            view_info.components.g = VK_COMPONENT_SWIZZLE_G;
            view_info.components.b = VK_COMPONENT_SWIZZLE_B;
            view_info.components.a = VK_COMPONENT_SWIZZLE_A;

            VkImageView imageView;
            VK_CHECK(vkCreateImageView(device, &view_info, nullptr, &imageView));
            swapchainImageViews.push_back(imageView);

            //backbufferTextures[backbufferIndex] = new VulkanTexture(this, swapChainImages[i]);
            //backbufferTextures[backbufferIndex]->SetName(fmt::format("Back Buffer {}", i));
            SetObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)swapchainImages[i], fmt::format("Back Buffer {}", i));
        }

        return true;
    }

    VkResult VulkanGraphicsDevice::AcquireNextImage(uint32_t* imageIndex)
    {
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

        VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, acquireSemaphore, VK_NULL_HANDLE, imageIndex);

        if (result != VK_SUCCESS)
        {
            recycledSemaphores.push_back(acquireSemaphore);
            return result;
        }

        // If we have outstanding fences for this swapchain image, wait for them to complete first.
        // After begin frame returns, it is safe to reuse or delete resources which
        // were used previously.
        //
        // We wait for fences which completes N frames earlier, so we do not stall,
        // waiting for all GPU work to complete before this returns.
        // Normally, this doesn't really block at all,
        // since we're waiting for old frames to have been completed, but just in case.
        if (frame[*imageIndex].fence != VK_NULL_HANDLE)
        {
            VK_CHECK(vkWaitForFences(device, 1, &frame[*imageIndex].fence, true, UINT64_MAX));
            VK_CHECK(vkResetFences(device, 1, &frame[*imageIndex].fence));
        }

        if (frame[*imageIndex].primaryCommandPool != VK_NULL_HANDLE)
        {
            vkResetCommandPool(device, frame[*imageIndex].primaryCommandPool, 0);
        }

        // Recycle the old semaphore back into the semaphore manager.
        VkSemaphore old_semaphore = frame[*imageIndex].swapchainAcquireSemaphore;

        if (old_semaphore != VK_NULL_HANDLE)
        {
            recycledSemaphores.push_back(old_semaphore);
        }

        frame[*imageIndex].swapchainAcquireSemaphore = acquireSemaphore;

        return VK_SUCCESS;
    }

    VkResult VulkanGraphicsDevice::PresentImage(uint32_t imageIndex)
    {
        VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &frame[imageIndex].swapchainReleaseSemaphore;
        return vkQueuePresentKHR(graphicsQueue, &presentInfo);
    }

    bool VulkanGraphicsDevice::Initialize(WindowHandle windowHandle, uint32_t width, uint32_t height, bool isFullscreen)
    {
        //backbufferSize.x = width;
        //backbufferSize.y = height;
        UpdateSwapchain();

        // Create frame data.
        for (size_t i = 0, count = frame.size(); i < count; i++)
        {
            VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            VK_CHECK(vkCreateFence(device, &fenceInfo, nullptr, &frame[i].fence));

            VkCommandPoolCreateInfo commandPoolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
            commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            commandPoolInfo.queueFamilyIndex = queueFamilies.graphicsQueueFamilyIndex;
            VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frame[i].primaryCommandPool));

            VkCommandBufferAllocateInfo cmdAllocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
            cmdAllocateInfo.commandPool = frame[i].primaryCommandPool;
            cmdAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdAllocateInfo.commandBufferCount = 1;
            VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocateInfo, &frame[i].primaryCommandBuffer));
        }

        return true;
    }

    bool VulkanGraphicsDevice::InitSurface(GPUPlatformHandle windowHandle)
    {
        VkResult result = VK_SUCCESS;

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
        VkWin32SurfaceCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
        createInfo.hinstance = windowHandle.hinstance;
        createInfo.hwnd = windowHandle.hwnd;
        result = vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
#elif defined(VK_USE_PLATFORM_XCB_KHR)
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
#else
#	pragma error Platform not supported
#endif
        if (result != VK_SUCCESS)
        {
            VK_LOG_ERROR(result, "Vulkan: Failed to create surface");
            return false;
        }

        return true;
    }

    bool VulkanGraphicsDevice::InitPhysicalDevice(GPUPowerPreference powerPreference)
    {
        // Enumerating and creating devices:
        uint32_t physicalDevicesCount = 0;
        VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, nullptr));

        if (physicalDevicesCount == 0) {
            LOGE("failed to find GPUs with Vulkan support!");
            assert(0);
        }

        vector<VkPhysicalDevice> physicalDevices(physicalDevicesCount);
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

            switch (physical_device_props.deviceType)
            {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                score += 100u;
                if (powerPreference == GPUPowerPreference::HighPerformance)
                    score += 1000u;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                score += 90u;
                if (powerPreference == GPUPowerPreference::LowPower)
                    score += 1000u;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                score += 80u;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                score += 70u;
                break;
            default:
                score += 10U;
                break;
            }

            if (score > bestDeviceScore) {
                bestDeviceIndex = i;
                bestDeviceScore = score;
            }
        }

        if (bestDeviceIndex == VK_QUEUE_FAMILY_IGNORED) {
            LOGE("Vulkan: Cannot find suitable physical device.");
            return false;
        }

        adapter = new VulkanGPUAdapter(physicalDevices[bestDeviceIndex]);
        queueFamilies = QueryQueueFamilies(instance, adapter->GetHandle(), surface);
        physicalDeviceExts = QueryPhysicalDeviceExtensions(instanceExts, adapter->GetHandle());
        return true;
    }

    bool VulkanGraphicsDevice::InitLogicalDevice()
    {
        VkResult result = VK_SUCCESS;

        /* Setup device queue's. */
        vector<VkQueueFamilyProperties> queue_families = adapter->GetQueueFamilyProperties();

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
        vector<const char*> enabledExtensions;
        const bool deviceApiVersion11 = adapter->GetProperties().apiVersion >= VK_API_VERSION_1_1;

        if (surface != VK_NULL_HANDLE) {
            enabledExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }

        if (physicalDeviceExts.get_memory_requirements2 && physicalDeviceExts.dedicated_allocation)
        {
            enabledExtensions.push_back("VK_KHR_get_memory_requirements2");
            enabledExtensions.push_back("VK_KHR_dedicated_allocation");
        }

        if (!deviceApiVersion11)
        {
            if (physicalDeviceExts.maintenance_1)
                enabledExtensions.push_back("VK_KHR_maintenance1");

            if (physicalDeviceExts.maintenance_2)
                enabledExtensions.push_back("VK_KHR_maintenance2");

            if (physicalDeviceExts.maintenance_3)
                enabledExtensions.push_back("VK_KHR_maintenance3");
        }

        if (physicalDeviceExts.image_format_list)
        {
            enabledExtensions.push_back(VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME);
        }

        if (physicalDeviceExts.sampler_mirror_clamp_to_edge)
        {
            enabledExtensions.push_back(VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME);
        }

        if (physicalDeviceExts.depth_clip_enable)
        {
            enabledExtensions.push_back(VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME);
        }

        /*if (vk.physical_device_features.buffer_device_address)
        {
            enabledExtensions[enabled_device_ext_count++] = VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME;
        }*/

#ifdef _WIN32
        if (instanceExts.get_surface_capabilities2 && physicalDeviceExts.win32_full_screen_exclusive)
        {
            enabledExtensions.push_back(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME);
        }
#endif

        VkPhysicalDeviceFeatures2 features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        VkPhysicalDeviceMultiviewFeatures multiview_features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES };
        void** ppNext = &features.pNext;

        if (physicalDeviceExts.multiview)
        {
            if (!deviceApiVersion11) {
                enabledExtensions.push_back("VK_KHR_multiview");
            }

            *ppNext = &multiview_features;
            ppNext = &multiview_features.pNext;
        }

        vkGetPhysicalDeviceFeatures2(adapter->GetHandle(), &features);

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
        createInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
        createInfo.ppEnabledExtensionNames = enabledExtensions.data();

        result = vkCreateDevice(adapter->GetHandle(), &createInfo, nullptr, &device);
        if (result != VK_SUCCESS)
        {
            VK_LOG_ERROR(result, "Failed to create device");
            return false;
        }

        vkGetDeviceQueue(device, queueFamilies.graphicsQueueFamilyIndex, 0, &graphicsQueue);
        vkGetDeviceQueue(device, queueFamilies.computeQueueFamily, compute_queue_index, &computeQueue);
        vkGetDeviceQueue(device, queueFamilies.copyQueueFamily, copy_queue_index, &copyQueue);

        LOGI("Created VkDevice using '{}' adapter with API version: {}.{}.{}",
            adapter->GetProperties().deviceName,
            VK_VERSION_MAJOR(adapter->GetProperties().apiVersion),
            VK_VERSION_MINOR(adapter->GetProperties().apiVersion),
            VK_VERSION_PATCH(adapter->GetProperties().apiVersion));
        for (uint32 i = 0; i < createInfo.enabledExtensionCount; ++i)
        {
            LOGI("Device extension '{}'", createInfo.ppEnabledExtensionNames[i]);
        }

        // Create vma allocator.
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = adapter->GetHandle();
        allocatorInfo.device = device;
        allocatorInfo.instance = instance;

        if (physicalDeviceExts.get_memory_requirements2 && physicalDeviceExts.dedicated_allocation)
        {
            allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
        }

        result = vmaCreateAllocator(&allocatorInfo, &allocator);
        if (result != VK_SUCCESS)
        {
            VK_LOG_ERROR(result, "Cannot create allocator");
            return false;
        }

        return true;
    }

    void VulkanGraphicsDevice::SetObjectName(VkObjectType type, uint64_t handle, const String& name)
    {
        if (!instanceExts.debugUtils)
            return;

        VkDebugUtilsObjectNameInfoEXT info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        info.objectType = type;
        info.objectHandle = handle;
        info.pObjectName = name.c_str();
        VK_CHECK(vkSetDebugUtilsObjectNameEXT(device, &info));
    }

    void VulkanGraphicsDevice::WaitForGPU()
    {
        VK_CHECK(vkDeviceWaitIdle(device));
    }

    bool VulkanGraphicsDevice::BeginFrameImpl()
    {
        ALIMER_ASSERT_MSG(!frameActive, "Frame is still active, please call EndFrame first");

        /*VkResult result = AcquireNextImage(&backbufferIndex);

        // Handle outdated error in acquire.
        if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            UpdateSwapchain();
            result = AcquireNextImage(&backbufferIndex);
        }

        if (result != VK_SUCCESS)
        {
            WaitForGPU();
            return false;
        }

        // Begin primary frame command buffer. We will only submit this once before it's recycled 
        VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_CHECK(vkBeginCommandBuffer(frame[backbufferIndex].primaryCommandBuffer, &beginInfo));*/

        // Now the frame is active again.
        frameActive = true;

        return true;
    }

    void VulkanGraphicsDevice::EndFrameImpl()
    {
        ALIMER_ASSERT_MSG(frameActive, "Frame is not active, please call BeginFrame first.");

        /*VkResult result = VK_SUCCESS;
        VkCommandBuffer commandBuffer = frame[backbufferIndex].primaryCommandBuffer;

        //TextureBarrier(commandBuffer, swapchainImages[backbufferIndex], swapChainImageLayouts[backbufferIndex], VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        //swapChainImageLayouts[backbufferIndex] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // Complete the command buffer.
        VK_CHECK(vkEndCommandBuffer(commandBuffer));

        // Submit it to the queue with a release semaphore.
        if (frame[backbufferIndex].swapchainReleaseSemaphore == VK_NULL_HANDLE)
        {
            VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
            VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frame[backbufferIndex].swapchainReleaseSemaphore));
        }

        VkPipelineStageFlags waitStage{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &frame[backbufferIndex].swapchainAcquireSemaphore;
        submitInfo.pWaitDstStageMask = &waitStage;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &frame[backbufferIndex].swapchainReleaseSemaphore;

        // Submit command buffer to graphics queue.
        VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, frame[backbufferIndex].fence));

        result = PresentImage(backbufferIndex);

        // Handle Outdated error in present.
        if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            UpdateSwapchain();
        }
        else if (result != VK_SUCCESS)
        {
            LOGE("Failed to present swapchain image.");
        }*/

        // Frame is not active anymore
        frameActive = false;
    }

    void VulkanGraphicsDevice::Present(GPUSwapChain* swapChain, bool verticalSync)
    {

    }

    void VulkanGraphicsDevice::SetVerticalSync(bool value)
    {
        /*if (verticalSync == value)
            return;

        verticalSync = value;
        WaitForGPU();
        UpdateSwapchain();*/
    }

    void VulkanGraphicsDevice::TeardownPerFrame(PerFrame& frame)
    {
        Purge(frame);

        if (frame.fence != VK_NULL_HANDLE)
        {
            vkDestroyFence(device, frame.fence, nullptr);
            frame.fence = VK_NULL_HANDLE;
        }

        if (frame.primaryCommandBuffer != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers(device, frame.primaryCommandPool, 1, &frame.primaryCommandBuffer);
            frame.primaryCommandBuffer = VK_NULL_HANDLE;
        }

        if (frame.primaryCommandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(device, frame.primaryCommandPool, nullptr);
            frame.primaryCommandPool = VK_NULL_HANDLE;
        }

        if (frame.swapchainAcquireSemaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(device, frame.swapchainAcquireSemaphore, nullptr);
            frame.swapchainAcquireSemaphore = VK_NULL_HANDLE;
        }

        if (frame.swapchainReleaseSemaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(device, frame.swapchainReleaseSemaphore, nullptr);
            frame.swapchainReleaseSemaphore = VK_NULL_HANDLE;
        }

    }

    void VulkanGraphicsDevice::Purge(PerFrame& frame)
    {
        while (frame.deferredReleases.size())
        {
            const ResourceRelease* ref = &frame.deferredReleases.front();

            switch (ref->type)
            {
            case VK_OBJECT_TYPE_BUFFER:
                if (ref->memory)
                    vmaDestroyBuffer(allocator, (VkBuffer)ref->handle, ref->memory);
                else
                    vkDestroyBuffer(device, (VkBuffer)ref->handle, nullptr);
                break;

            case VK_OBJECT_TYPE_IMAGE:
                if (ref->memory)
                    vmaDestroyImage(allocator, (VkImage)ref->handle, ref->memory);
                else
                    vkDestroyImage(device, (VkImage)ref->handle, nullptr);
                break;

            case VK_OBJECT_TYPE_DEVICE_MEMORY:
                vkFreeMemory(device, (VkDeviceMemory)ref->handle, nullptr);
                break;

            case VK_OBJECT_TYPE_IMAGE_VIEW:
                vkDestroyImageView(device, (VkImageView)ref->handle, nullptr);
                break;

            case VK_OBJECT_TYPE_SAMPLER:
                vkDestroySampler(device, (VkSampler)ref->handle, nullptr);
                break;

            case VK_OBJECT_TYPE_RENDER_PASS:
                vkDestroyRenderPass(device, (VkRenderPass)ref->handle, nullptr);
                break;

            case VK_OBJECT_TYPE_FRAMEBUFFER:
                vkDestroyFramebuffer(device, (VkFramebuffer)ref->handle, nullptr);
                break;

            case VK_OBJECT_TYPE_PIPELINE:
                vkDestroyPipeline(device, (VkPipeline)ref->handle, nullptr);
                break;
            default:
                break;
            }

            frame.deferredReleases.pop();
        }
    }

    /* Resource creation methods */
    GPUSwapChain* VulkanGraphicsDevice::CreateSwapChainCore(const GPUSwapChainDescriptor& descriptor)
    {
        return nullptr;
    }

#if TOOD_VK
    TextureHandle VulkanGraphicsDevice::AllocTextureHandle()
    {
        std::lock_guard<std::mutex> LockGuard(handle_mutex);

        if (textures.isFull()) {
            LOGE("Not enough free texture slots.");
            return kInvalidTexture;
        }
        const int id = textures.Alloc();
        ALIMER_ASSERT(id >= 0);

        VulkanTexture& texture = textures[id];
        texture.handle = nullptr;
        texture.memory = nullptr;
        return { (uint32_t)id };
    }

    TextureHandle VulkanGraphicsDevice::CreateTexture(const GPUTextureDescriptor* desc, const void* data)
    {
        TextureHandle handle = kInvalidTexture;

        /*if (desc->externalHandle != nullptr)
        {
            handle = AllocTextureHandle();
            textures[handle.id].handle = (VkImage)desc->externalHandle;
            textures[handle.id].memory = VK_NULL_HANDLE;
        }
        else*/
        {
            VkImageCreateInfo createInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
            createInfo.flags = 0u;
            createInfo.imageType = VK_IMAGE_TYPE_2D;
            createInfo.format = VK_FORMAT_R8G8B8A8_UNORM; // GetVkFormat(desc->format);
            createInfo.extent.width = desc->width;
            createInfo.extent.height = desc->height;
            createInfo.extent.depth = desc->depth;
            createInfo.mipLevels = desc->mipLevels;
            createInfo.arrayLayers = desc->arrayLayers;
            createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT; // vgpu_vkGetImageUsage(desc->usage, desc->format);
            createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
            createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            VmaAllocationCreateInfo allocCreateInfo = {};
            allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            VkImage image;
            VmaAllocation allocation;
            VkResult result = vmaCreateImage(
                allocator,
                &createInfo, &allocCreateInfo,
                &image, &allocation,
                nullptr);

            if (result != VK_SUCCESS)
            {
                return kInvalidTexture;
            }

            handle = AllocTextureHandle();
            textures[handle.id].handle = image;
            textures[handle.id].memory = allocation;
        }

        return handle;
    }

    void VulkanGraphicsDevice::Destroy(TextureHandle handle)
    {
        if (!handle.isValid())
            return;

        VulkanTexture& texture = textures[handle.id];
        //SafeRelease(texture.handle);

        std::lock_guard<std::mutex> LockGuard(handle_mutex);
        textures.Dealloc(handle.id);
    }

    void VulkanGraphicsDevice::SetName(TextureHandle handle, const char* name)
    {
        if (!handle.isValid())
            return;

        SetObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)textures[handle.id].handle, name);
    }

#endif // TOOD_VK

    BufferHandle VulkanGraphicsDevice::AllocBufferHandle()
    {
        std::lock_guard<std::mutex> LockGuard(handle_mutex);

        if (buffers.isFull()) {
            LOGE("Not enough free buffer slots.");
            return kInvalidBuffer;
        }
        const int id = buffers.Alloc();
        ALIMER_ASSERT(id >= 0);

        VulkanBuffer& buffer = buffers[id];
        buffer.handle = nullptr;
        buffer.memory = nullptr;
        return { (uint32_t)id };
    }

    BufferHandle VulkanGraphicsDevice::CreateBuffer(BufferUsage usage, uint32_t size, uint32_t stride, const void* data)
    {
        BufferHandle handle = AllocBufferHandle();
        //buffers[handle.id].handle = d3dBuffer;
        return handle;
    }

    void VulkanGraphicsDevice::Destroy(BufferHandle handle)
    {
        if (!handle.isValid())
            return;

        VulkanBuffer& buffer = buffers[handle.id];
        //SafeRelease(texture.handle);

        std::lock_guard<std::mutex> LockGuard(handle_mutex);
        buffers.Dealloc(handle.id);
    }

    void VulkanGraphicsDevice::SetName(BufferHandle handle, const char* name)
    {
        if (!handle.isValid())
            return;

        SetObjectName(VK_OBJECT_TYPE_BUFFER, (uint64_t)buffers[handle.id].handle, name);
    }

    /* Commands */
    void VulkanGraphicsDevice::PushDebugGroup(const String& name, CommandList commandList)
    {
    }

    void VulkanGraphicsDevice::PopDebugGroup(CommandList commandList)
    {
    }

    void VulkanGraphicsDevice::InsertDebugMarker(const String& name, CommandList commandList)
    {
    }

    void VulkanGraphicsDevice::BeginRenderPass(CommandList commandList, uint32_t numColorAttachments, const RenderPassColorAttachment* colorAttachments, const RenderPassDepthStencilAttachment* depthStencil)
    {
        VkCommandBuffer commandBuffer = frame[backbufferIndex].primaryCommandBuffer;

        uint32_t clearValueCount = 0;
        VkClearValue clearValues[kMaxColorAttachments + 1];

        for (uint32_t i = 0; i < numColorAttachments; i++)
        {
            //TextureBarrier(commandBuffer, swapchainImages[backbufferIndex], swapChainImageLayouts[backbufferIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            //swapChainImageLayouts[backbufferIndex] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            clearValues[clearValueCount].color.float32[0] = colorAttachments[i].clearColor.r;
            clearValues[clearValueCount].color.float32[1] = colorAttachments[i].clearColor.g;
            clearValues[clearValueCount].color.float32[2] = colorAttachments[i].clearColor.b;
            clearValues[clearValueCount].color.float32[3] = colorAttachments[i].clearColor.a;
            clearValueCount++;
        }

        VkRenderPassBeginInfo beginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        beginInfo.renderPass = GetRenderPass(numColorAttachments, colorAttachments, depthStencil);
        beginInfo.framebuffer = GetFramebuffer(beginInfo.renderPass, numColorAttachments, colorAttachments, depthStencil);
        beginInfo.renderArea.offset.x = 0;
        beginInfo.renderArea.offset.y = 0;
        //beginInfo.renderArea.extent.width = backbufferSize.x;
        //beginInfo.renderArea.extent.height = backbufferSize.y;
        beginInfo.clearValueCount = clearValueCount;
        beginInfo.pClearValues = clearValues;
        vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanGraphicsDevice::EndRenderPass(CommandList commandList)
    {
        /* TODO: Resolve */
        VkCommandBuffer commandBuffer = frame[backbufferIndex].primaryCommandBuffer;
        vkCmdEndRenderPass(commandBuffer);
    }

    void VulkanGraphicsDevice::TextureBarrier(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        VkImageMemoryBarrier imageMemoryBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        imageMemoryBarrier.oldLayout = oldLayout;
        imageMemoryBarrier.newLayout = newLayout;
        imageMemoryBarrier.image = image;
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
        imageMemoryBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
        imageMemoryBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        // Source layouts (old)
        // Source access mask controls actions that have to be finished on the old layout
        // before it will be transitioned to the new layout
        switch (oldLayout)
        {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            // Image layout is undefined (or does not matter)
            // Only valid as initial layout
            // No flags required, listed only for completeness
            imageMemoryBarrier.srcAccessMask = 0;
            break;

        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            // Image is preinitialized
            // Only valid as initial layout for linear images, preserves memory contents
            // Make sure host writes have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image is a color attachment
            // Make sure any writes to the color buffer have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image is a depth/stencil attachment
            // Make sure any writes to the depth/stencil buffer have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Image is a transfer source
            // Make sure any reads from the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Image is a transfer destination
            // Make sure any writes to the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Image is read by a shader
            // Make sure any shader reads from the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            // Other source layouts aren't handled (yet)
            break;
        }

        // Target layouts (new)
        // Destination access mask controls the dependency for the new image layout
        switch (newLayout)
        {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Image will be used as a transfer destination
            // Make sure any writes to the image have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Image will be used as a transfer source
            // Make sure any reads from the image have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image will be used as a color attachment
            // Make sure any writes to the color buffer have been finished
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image layout will be used as a depth/stencil attachment
            // Make sure any writes to depth/stencil buffer have been finished
            imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Image will be read in a shader (sampler, input attachment)
            // Make sure any writes to the image have been finished
            if (imageMemoryBarrier.srcAccessMask == 0)
            {
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
            }
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            // Other source layouts aren't handled (yet)
            break;
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
    }

    VkRenderPass VulkanGraphicsDevice::GetRenderPass(uint32_t numColorAttachments, const RenderPassColorAttachment* colorAttachments, const RenderPassDepthStencilAttachment* depthStencil)
    {
        Hasher h;

        VkFormat formats[kMaxColorAttachments];

        for (uint32_t i = 0; i < numColorAttachments; i++)
        {
            //ALIMER_ASSERT(colorAttachments[i] != nullptr);
            formats[i] = VK_FORMAT_B8G8R8A8_UNORM;
        }

        h.data(formats, numColorAttachments * sizeof(VkFormat));
        h.u32(numColorAttachments);
        //h.u32(depth_stencil);

        auto hash = h.get();

        auto it = renderPasses.find(hash);
        if (it == renderPasses.end())
        {
            uint32_t attachmentCount = 0;
            VkAttachmentDescription attachments[kMaxColorAttachments + 1];
            VkAttachmentReference references[kMaxColorAttachments + 1];

            for (uint32_t i = 0; i < numColorAttachments; i++)
            {
                attachments[i] = {};
                attachments[i].format = VK_FORMAT_B8G8R8A8_UNORM; // texture.format;
                attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
                attachments[i].loadOp = VulkanAttachmentLoadOp(colorAttachments[i].loadAction);
                attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachments[i].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                attachments[i].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                references[i] = { i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
                attachmentCount++;
            }

            VkSubpassDescription subpass = {};
            subpass.flags = 0u;
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.inputAttachmentCount = 0u;
            subpass.pInputAttachments = nullptr;
            subpass.colorAttachmentCount = numColorAttachments;
            subpass.pColorAttachments = references;
            subpass.pResolveAttachments = nullptr;
            subpass.pDepthStencilAttachment = (depthStencil != nullptr) ? &references[attachmentCount - 1] : nullptr;

            VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
            createInfo.attachmentCount = attachmentCount;
            createInfo.pAttachments = attachments;
            createInfo.subpassCount = 1;
            createInfo.pSubpasses = &subpass;
            createInfo.dependencyCount = 0;
            createInfo.pDependencies = nullptr;

            VkRenderPass handle;
            VK_CHECK(vkCreateRenderPass(device, &createInfo, nullptr, &handle));
            renderPasses[hash] = handle;
            return handle;
        }

        return it->second;
    }

    VkFramebuffer VulkanGraphicsDevice::GetFramebuffer(VkRenderPass renderPass, uint32_t numColorAttachments, const RenderPassColorAttachment* colorAttachments, const RenderPassDepthStencilAttachment* depthStencil)
    {
        Hasher h;
        h.u64((uint64)renderPass);

        uint32 width = 0; // backbufferSize.x; // UINT32_MAX;
        uint32 height = 0; //  = backbufferSize.y; //UINT32_MAX;
        uint32 attachmentCount = 0;
        VkImageView attachments[kMaxColorAttachments + 1];

        for (uint32_t i = 0; i < numColorAttachments; i++)
        {
            //ALIMER_ASSERT(colorAttachments[i] != nullptr);
            attachments[attachmentCount] = swapchainImageViews[backbufferIndex];
            h.u64((uint64_t)attachments[attachmentCount]);
            attachmentCount++;
        }

        auto hash = h.get();
        auto it = framebuffers.find(hash);
        if (it == framebuffers.end())
        {
            VkFramebufferCreateInfo createInfo { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
            createInfo.renderPass = renderPass;
            createInfo.attachmentCount = attachmentCount;
            createInfo.pAttachments = attachments;
            createInfo.width = width;
            createInfo.height = height;
            createInfo.layers = 1;

            VkFramebuffer handle;
            if (vkCreateFramebuffer(device, &createInfo, NULL, &handle) != VK_SUCCESS)
            {
                LOGE("Vulkan: Failed to create framebuffer.");
                return VK_NULL_HANDLE;
            }

            framebuffers[hash] = handle;
            return handle;
        }

        return it->second;
    }

    void VulkanGraphicsDevice::ClearRenderPassCache()
    {
        for (auto it : renderPasses) {
            vkDestroyRenderPass(device, it.second, nullptr);
        }
        renderPasses.clear();
    }

    void VulkanGraphicsDevice::ClearFramebufferCache()
    {
        for (auto it : framebuffers) {
            vkDestroyFramebuffer(device, it.second, nullptr);
        }
        framebuffers.clear();
    }
}
