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

#include "graphics_vulkan.h"
#include "volk.h"
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include <vector>
#include <map>
#include <algorithm>

namespace alimer
{
    namespace graphics
    {
        namespace vulkan
        {
            static const char* GetErrorString(VkResult result) {
                switch (result) {
                case VK_ERROR_OUT_OF_HOST_MEMORY: return "Out of CPU memory";
                case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "Out of GPU memory";
                case VK_ERROR_MEMORY_MAP_FAILED: return "Could not map memory";
                case VK_ERROR_DEVICE_LOST: return "Lost connection to GPU";
                case VK_ERROR_TOO_MANY_OBJECTS: return "Too many objects";
                case VK_ERROR_FORMAT_NOT_SUPPORTED: return "Unsupported format";
                default: return NULL;
                }
            }

            VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
                const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                void* user_data)
            {
                // Log debug messge
                if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
                {
                    LogWarn("%u - %s: %s", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
                }
                else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
                {
                    LogError("%u - %s: %s", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
                }
                return VK_FALSE;
            }

            struct PhysicalDeviceExtensions
            {
                bool swapchain;
                bool depthClipEnable;
                bool maintenance1;
                bool maintenance2;
                bool maintenance3;
                bool getMemoryRequirements2;
                bool dedicatedAllocation;
                bool bindMemory2;
                bool memoryBudget;
                bool imageFormatList;
                bool debugMarker;
                bool raytracing;
                bool buffer_device_address;
                bool deferred_host_operations;
                bool descriptor_indexing;
                bool pipeline_library;
                bool externalSemaphore;
                bool externalMemory;
                struct {
                    bool fullScreenExclusive;
                    bool externalSemaphore;
                    bool externalMemory;
                } win32;
                struct {
                    bool externalSemaphore;
                    bool externalMemory;
                } fd;
            };

            struct QueueFamilyIndices
            {
                uint32_t graphicsFamily = VK_QUEUE_FAMILY_IGNORED;
                uint32_t computeFamily = VK_QUEUE_FAMILY_IGNORED;
                uint32_t transferFamily = VK_QUEUE_FAMILY_IGNORED;
                uint32_t timestampValidBits;

                bool IsComplete()
                {
                    return (graphicsFamily != VK_QUEUE_FAMILY_IGNORED);
                }
            };

#define GPU_VK_THROW(str) LogError("%s", str);
#define GPU_VK_CHECK(c, str) if (!(c)) { GPU_VK_THROW(str); }
#define VK_CHECK(res) do { VkResult r = (res); GPU_VK_CHECK(r >= 0, GetErrorString(r)); } while (0)

            struct Frame
            {
                uint32_t index;
                VkCommandPool commandPool;
                VkFence fence;
                VkSemaphore imageAcquiredSemaphore;
                VkSemaphore renderCompleteSemaphore;
            };

            struct Context
            {
                enum { MAX_COUNT = 16 };

                VkSurfaceKHR surface;
                VkSurfaceFormatKHR surfaceFormat;
                VkSwapchainKHR handle;
                uint32_t frameIndex;
                uint32_t imageCount;
                uint32_t semaphoreIndex;
                Frame* frames;
            };

            static struct {
                bool availableInitialized = false;
                bool available = false;

                /// VK_KHR_get_physical_device_properties2
                bool physicalDeviceProperties2 = false;
                /// VK_KHR_external_memory_capabilities
                bool externalMemoryCapabilities = false;
                /// VK_KHR_external_semaphore_capabilities
                bool externalSemaphoreCapabilities = false;
                /// VK_EXT_debug_utils
                bool debugUtils = false;
                /// VK_EXT_headless_surface
                bool headless = false;
                /// VK_KHR_surface
                bool surface = false;
                /// VK_KHR_get_surface_capabilities2
                bool surfaceCapabilities2 = false;

                VkInstance instance;
                VkDebugUtilsMessengerEXT debugUtilsMessenger = VK_NULL_HANDLE;

                VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
                VkPhysicalDeviceProperties physicalDeviceProperties;
                std::vector<VkQueueFamilyProperties> queueFamilyProperties;
                PhysicalDeviceExtensions physicalDeviceExtensions;
                QueueFamilyIndices queueFamilyIndices;
                bool supportsExternal;

                VkDevice device = VK_NULL_HANDLE;
                VolkDeviceTable deviceTable;
                VkQueue graphicsQueue = VK_NULL_HANDLE;
                VkQueue computeQueue = VK_NULL_HANDLE;
                VkQueue copyQueue = VK_NULL_HANDLE;

                VmaAllocator memoryAllocator = VK_NULL_HANDLE;

                Pool<Context, Context::MAX_COUNT> contexts;
            } state;

            static bool VulkanIsSupported(void)
            {
                if (state.availableInitialized) {
                    return state.available;
                }

                state.availableInitialized = true;
                VkResult result = volkInitialize();
                if (result != VK_SUCCESS)
                {
                    return false;
                }

                // Create the Vulkan instance
                VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
                appInfo.pApplicationName = "Alimer";
                appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
                appInfo.pEngineName = "Alimer";
                appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
                appInfo.apiVersion = VK_API_VERSION_1_1;

                VkInstanceCreateInfo instanceInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
                instanceInfo.pApplicationInfo = &appInfo;

                VkInstance instance;
                result = vkCreateInstance(&instanceInfo, nullptr, &instance);
                if (result != VK_SUCCESS)
                {
                    return false;
                }

                volkLoadInstance(instance);
                vkDestroyInstance(instance, nullptr);
                state.available = true;
                return state.available;
            }

            static PhysicalDeviceExtensions CheckDeviceExtensionSupport(VkPhysicalDevice device)
            {
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
                        exts.depthClipEnable = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_MAINTENANCE1_EXTENSION_NAME) == 0) {
                        exts.maintenance1 = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_MAINTENANCE2_EXTENSION_NAME) == 0) {
                        exts.maintenance2 = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_MAINTENANCE3_EXTENSION_NAME) == 0) {
                        exts.maintenance3 = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME) == 0) {
                        exts.getMemoryRequirements2 = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME) == 0) {
                        exts.dedicatedAllocation = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_BIND_MEMORY_2_EXTENSION_NAME) == 0) {
                        exts.bindMemory2 = true;
                    }
                    else if (strcmp(extension.extensionName, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME) == 0) {
                        exts.memoryBudget = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME) == 0) {
                        exts.imageFormatList = true;
                    }
                    else if (strcmp(extension.extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0) {
                        exts.debugMarker = true;
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
                    else if (strcmp(extension.extensionName, VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME) == 0) {
                        exts.externalSemaphore = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME) == 0) {
                        exts.externalMemory = true;
                    }
                    else if (strcmp(extension.extensionName, "VK_EXT_full_screen_exclusive") == 0) {
                        exts.win32.fullScreenExclusive = true;
                    }
                    else if (strcmp(extension.extensionName, "VK_KHR_external_semaphore_win32") == 0) {
                        exts.win32.externalSemaphore = true;
                    }
                    else if (strcmp(extension.extensionName, "VK_KHR_external_memory_win32") == 0) {
                        exts.win32.externalMemory = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME) == 0) {
                        exts.fd.externalSemaphore = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME) == 0) {
                        exts.fd.externalMemory = true;
                    }
                }

                return exts;
            }

            static bool GetPhysicalDevicePresentationSupport(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex) {
#if defined(_WIN32) || defined(_WIN64)
                return vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, queueFamilyIndex);
#elif defined(__ANDROID__)
                return true; // All Android queues surfaces support present.
#else
                // TODO:
                return true;
#endif
            }

            static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
            {
                uint32_t queueFamilyCount = 0;
                vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

                std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
                vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

                QueueFamilyIndices indices = {};
                for (uint32_t i = 0; i < queueFamilyCount; i++)
                {
                    VkBool32 presentSupported = surface == VK_NULL_HANDLE;
                    if (surface != VK_NULL_HANDLE)
                    {
                        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupported);
                    }
                    else
                    {
                        presentSupported = GetPhysicalDevicePresentationSupport(physicalDevice, i);
                    }

                    static const VkQueueFlags required = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT;
                    if (presentSupported && ((queueFamilyProperties[i].queueFlags & required) == required))
                    {
                        indices.graphicsFamily = i;

                        // This assumes timestamp valid bits is the same for all queue types.
                        indices.timestampValidBits = queueFamilyProperties[i].timestampValidBits;
                        break;
                    }
                }

                for (uint32_t i = 0; i < queueFamilyCount; i++)
                {
                    static const VkQueueFlags required = VK_QUEUE_COMPUTE_BIT;
                    if (i != indices.graphicsFamily
                        && (queueFamilyProperties[i].queueFlags & required) == required)
                    {
                        indices.computeFamily = i;
                        break;
                    }
                }

                for (uint32_t i = 0; i < queueFamilyCount; i++)
                {
                    static const VkQueueFlags required = VK_QUEUE_TRANSFER_BIT;
                    if (i != indices.graphicsFamily
                        && i != indices.computeFamily
                        && (queueFamilyProperties[i].queueFlags & required) == required)
                    {
                        indices.transferFamily = i;
                        break;
                    }
                }

                /* Find dedicated transfer family. */
                if (indices.transferFamily == VK_QUEUE_FAMILY_IGNORED)
                {
                    for (uint32_t i = 0; i < queueFamilyCount; i++)
                    {
                        static const VkQueueFlags required = VK_QUEUE_TRANSFER_BIT;
                        if (i != indices.graphicsFamily && (queueFamilyProperties[i].queueFlags & required) == required)
                        {
                            indices.transferFamily = i;
                            break;
                        }
                    }
                }


                return indices;
            }

            static int32_t RatePhysicalDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
            {
                VkPhysicalDeviceProperties deviceProperties;
                vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

                VkPhysicalDeviceFeatures deviceFeatures;
                vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

                PhysicalDeviceExtensions exts = CheckDeviceExtensionSupport(physicalDevice);
                if (!exts.swapchain || !exts.maintenance1)
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

            static bool VulkanInit(const Configuration& config)
            {
                // Enumerate global supported extensions.
                uint32_t instanceExtensionCount;
                VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr));

                std::vector<VkExtensionProperties> availableInstanceExtensions(instanceExtensionCount);
                VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, availableInstanceExtensions.data()));

                for (uint32_t i = 0; i < instanceExtensionCount; ++i)
                {
                    if (strcmp(availableInstanceExtensions[i].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
                        state.debugUtils = true;
                    }
                    else if (strcmp(availableInstanceExtensions[i].extensionName, VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME) == 0)
                    {
                        state.headless = true;
                    }
                    else if (strcmp(availableInstanceExtensions[i].extensionName, VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME) == 0)
                    {
                        state.surfaceCapabilities2 = true;
                    }
                    else if (strcmp(availableInstanceExtensions[i].extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0)
                    {
                        state.physicalDeviceProperties2 = true;
                    }
                    else if (strcmp(availableInstanceExtensions[i].extensionName, VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME) == 0)
                    {
                        state.externalMemoryCapabilities = true;
                    }
                    else if (strcmp(availableInstanceExtensions[i].extensionName, VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME) == 0)
                    {
                        state.externalSemaphoreCapabilities = true;
                    }
                }

                std::vector<const char*> enabledInstanceExtensions;

                if (state.physicalDeviceProperties2)
                {
                    enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
                }

                if (state.physicalDeviceProperties2 &&
                    state.externalMemoryCapabilities &&
                    state.externalSemaphoreCapabilities)
                {
                    enabledInstanceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
                    enabledInstanceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
                }

                if (config.debug && state.debugUtils)
                {
                    enabledInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                }

                // Try to enable headless surface extension if it exists.
                const bool headless = false;
                if (headless)
                {
                    if (!state.headless)
                    {
                        LogWarn("%s is not available, disabling swapchain creation", VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
                    }
                    else
                    {
                        LogInfo("%s is available, enabling it", VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
                        enabledInstanceExtensions.push_back(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
                    }
                }
                else
                {
                    enabledInstanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
                    // Enable surface extensions depending on os
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
                    enabledInstanceExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
                    enabledInstanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
                    enabledInstanceExtensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
                    enabledInstanceExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_IOS_MVK)
                    enabledInstanceExtensions.push_back(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
                // TODO: Support VK_EXT_metal_surface
                    enabledInstanceExtensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#endif

                    if (state.surfaceCapabilities2)
                    {
                        enabledInstanceExtensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
                    }
                }

                uint32_t enabledLayersCount = 0;
                const char* enabledLayers[8] = {};

                if (config.debug)
                {
                    uint32_t layerCount;
                    VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

                    VkLayerProperties queriedLayers[32] = {};
                    VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, queriedLayers));

                    bool foundValidationLayer = false;
                    for (uint32_t i = 0; i < layerCount; i++)
                    {
                        if (strcmp(queriedLayers[i].layerName, "VK_LAYER_KHRONOS_validation") == 0)
                        {
                            enabledLayers[enabledLayersCount++] = "VK_LAYER_KHRONOS_validation";
                            foundValidationLayer = true;
                            break;
                        }
                    }

                    if (!foundValidationLayer) {
                        for (uint32_t i = 0; i < layerCount; i++)
                        {
                            if (strcmp(queriedLayers[i].layerName, "VK_LAYER_LUNARG_standard_validation") == 0)
                            {
                                enabledLayers[enabledLayersCount++] = "VK_LAYER_LUNARG_standard_validation";
                                foundValidationLayer = true;
                                break;
                            }
                        }
                    }
                }

                // Create the Vulkan instance
                VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
                appInfo.pApplicationName = "Alimer";
                appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
                appInfo.pEngineName = "Alimer";
                appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
                appInfo.apiVersion = VK_API_VERSION_1_1;

                VkInstanceCreateInfo instanceInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
                instanceInfo.pApplicationInfo = &appInfo;
                instanceInfo.enabledLayerCount = enabledLayersCount;
                instanceInfo.ppEnabledLayerNames = enabledLayers;
                instanceInfo.enabledExtensionCount = static_cast<uint32_t>(enabledInstanceExtensions.size());
                instanceInfo.ppEnabledExtensionNames = enabledInstanceExtensions.data();

                VkDebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
                if (config.debug && state.debugUtils)
                {
                    debugUtilsCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
                    debugUtilsCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
                    debugUtilsCreateInfo.pfnUserCallback = DebugUtilsMessengerCallback;

                    instanceInfo.pNext = &debugUtilsCreateInfo;
                }

                VkResult result = vkCreateInstance(&instanceInfo, nullptr, &state.instance);
                if (result != VK_SUCCESS)
                {
                    return false;
                }

                volkLoadInstance(state.instance);

                LogInfo("Created VkInstance with version: %u.%u.%u", VK_VERSION_MAJOR(appInfo.apiVersion), VK_VERSION_MINOR(appInfo.apiVersion), VK_VERSION_PATCH(appInfo.apiVersion));
                if (enabledLayersCount > 0) {
                    for (uint32_t i = 0; i < enabledLayersCount; i++)
                    {
                        LogInfo("Instance layer '%s'", enabledLayers[i]);
                    }
                }

                for (auto* ext_name : enabledInstanceExtensions)
                {
                    LogInfo("Instance extension '%s'", ext_name);
                }

                if (config.debug && state.debugUtils)
                {
                    result = vkCreateDebugUtilsMessengerEXT(state.instance, &debugUtilsCreateInfo, nullptr, &state.debugUtilsMessenger);
                    if (result != VK_SUCCESS)
                    {
                        LogError("Could not create debug utils messenger");
                    }
                }

                uint32_t deviceCount;
                if (vkEnumeratePhysicalDevices(state.instance, &deviceCount, nullptr) != VK_SUCCESS ||
                    deviceCount == 0)
                {
                    return false;
                }

                std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
                if (vkEnumeratePhysicalDevices(state.instance, &deviceCount, physicalDevices.data()) != VK_SUCCESS) {
                    return false;
                }

                std::multimap<int32_t, VkPhysicalDevice> physicalDeviceCandidates;
                for (VkPhysicalDevice physicalDevice : physicalDevices)
                {
                    int32_t score = RatePhysicalDevice(physicalDevice, VK_NULL_HANDLE);
                    physicalDeviceCandidates.insert(std::make_pair(score, physicalDevice));
                }

                // Check if the best candidate is suitable at all
                if (physicalDeviceCandidates.rbegin()->first <= 0)
                {
                    LogError("[Vulkan]: Failed to find a suitable GPU!");
                    return false;
                }

                state.physicalDevice = physicalDeviceCandidates.rbegin()->second;
                vkGetPhysicalDeviceProperties(state.physicalDevice, &state.physicalDeviceProperties);
                // Store the properties of each queuefamily
                uint32_t queueFamilyCount = 0;
                vkGetPhysicalDeviceQueueFamilyProperties(state.physicalDevice, &queueFamilyCount, nullptr);
                state.queueFamilyProperties.resize(queueFamilyCount);
                vkGetPhysicalDeviceQueueFamilyProperties(state.physicalDevice, &queueFamilyCount, state.queueFamilyProperties.data());

                state.physicalDeviceExtensions = CheckDeviceExtensionSupport(state.physicalDevice);
                state.queueFamilyIndices = FindQueueFamilies(state.physicalDevice, VK_NULL_HANDLE);

                // Setup queues first.
                uint32_t universal_queue_index = 1;
                const uint32_t graphicsQueueIndex = 0;
                uint32_t computeQueueIndex = 0;
                uint32_t copyQueueIndex = 0;

                if (state.queueFamilyIndices.computeFamily == VK_QUEUE_FAMILY_IGNORED)
                {
                    state.queueFamilyIndices.computeFamily = state.queueFamilyIndices.graphicsFamily;
                    computeQueueIndex = std::min(state.queueFamilyProperties[state.queueFamilyIndices.graphicsFamily].queueCount - 1, universal_queue_index);
                    universal_queue_index++;
                }

                if (state.queueFamilyIndices.transferFamily == VK_QUEUE_FAMILY_IGNORED)
                {
                    state.queueFamilyIndices.transferFamily = state.queueFamilyIndices.graphicsFamily;
                    copyQueueIndex = std::min(state.queueFamilyProperties[state.queueFamilyIndices.graphicsFamily].queueCount - 1, universal_queue_index);
                    universal_queue_index++;
                }
                else if (state.queueFamilyIndices.transferFamily == state.queueFamilyIndices.computeFamily)
                {
                    copyQueueIndex = std::min(state.queueFamilyProperties[state.queueFamilyIndices.computeFamily].queueCount - 1, 1u);
                }

                static const float graphics_queue_prio = 0.5f;
                static const float compute_queue_prio = 1.0f;
                static const float transfer_queue_prio = 1.0f;
                float prio[3] = { graphics_queue_prio, compute_queue_prio, transfer_queue_prio };

                uint32_t queueCreateInfoCount = 0;
                VkDeviceQueueCreateInfo queueCreateInfo[3] = {};

                queueCreateInfo[queueCreateInfoCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo[queueCreateInfoCount].queueFamilyIndex = state.queueFamilyIndices.graphicsFamily;
                queueCreateInfo[queueCreateInfoCount].queueCount = std::min(universal_queue_index, state.queueFamilyProperties[state.queueFamilyIndices.graphicsFamily].queueCount);
                queueCreateInfo[queueCreateInfoCount].pQueuePriorities = prio;
                queueCreateInfoCount++;

                if (state.queueFamilyIndices.computeFamily != state.queueFamilyIndices.graphicsFamily)
                {
                    queueCreateInfo[queueCreateInfoCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                    queueCreateInfo[queueCreateInfoCount].queueFamilyIndex = state.queueFamilyIndices.computeFamily;
                    queueCreateInfo[queueCreateInfoCount].queueCount = std::min(state.queueFamilyIndices.transferFamily == state.queueFamilyIndices.computeFamily ? 2u : 1u,
                        state.queueFamilyProperties[state.queueFamilyIndices.computeFamily].queueCount);
                    queueCreateInfo[queueCreateInfoCount].pQueuePriorities = prio + 1;
                    queueCreateInfoCount++;
                }

                if (state.queueFamilyIndices.transferFamily != state.queueFamilyIndices.graphicsFamily
                    && state.queueFamilyIndices.transferFamily != state.queueFamilyIndices.computeFamily)
                {
                    queueCreateInfo[queueCreateInfoCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                    queueCreateInfo[queueCreateInfoCount].queueFamilyIndex = state.queueFamilyIndices.transferFamily;
                    queueCreateInfo[queueCreateInfoCount].queueCount = 1;
                    queueCreateInfo[queueCreateInfoCount].pQueuePriorities = prio + 2;
                    queueCreateInfoCount++;
                }

                std::vector<const char*> enabledExtensions;

                if (!headless)
                    enabledExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

                if (state.physicalDeviceExtensions.getMemoryRequirements2)
                {
                    enabledExtensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
                }

                if (state.physicalDeviceExtensions.getMemoryRequirements2
                    && state.physicalDeviceExtensions.dedicatedAllocation)
                {
                    enabledExtensions.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
                }

                if (state.physicalDeviceExtensions.imageFormatList)
                {
                    enabledExtensions.push_back(VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME);
                }

                if (state.physicalDeviceExtensions.debugMarker)
                {
                    enabledExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
                }

#ifdef _WIN32
                if (state.surfaceCapabilities2 &&
                    state.physicalDeviceExtensions.win32.fullScreenExclusive)
                {
                    enabledExtensions.push_back(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME);
                }
#endif

                if (state.externalMemoryCapabilities &&
                    state.externalSemaphoreCapabilities &&
                    state.physicalDeviceExtensions.getMemoryRequirements2 &&
                    state.physicalDeviceExtensions.dedicatedAllocation &&
                    state.physicalDeviceExtensions.externalSemaphore &&
                    state.physicalDeviceExtensions.externalSemaphore &&
#ifdef _WIN32
                    state.physicalDeviceExtensions.win32.externalMemory &&
                    state.physicalDeviceExtensions.win32.externalSemaphore
#else
                    state.physicalDeviceExtensions.fd.externalMemory &&
                    state.physicalDeviceExtensions.fd.externalSemaphore
#endif
                    )
                {
                    state.supportsExternal = true;
                    enabledExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
                    enabledExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
#ifdef _WIN32
                    enabledExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME);
                    enabledExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
#else
                    enabledExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
                    enabledExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
#endif
                }
                else
                {
                    state.supportsExternal = false;
                }

                if (state.physicalDeviceExtensions.maintenance1)
                {
                    enabledExtensions.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
                }

                if (state.physicalDeviceExtensions.maintenance2)
                {
                    enabledExtensions.push_back(VK_KHR_MAINTENANCE2_EXTENSION_NAME);
                }

                if (state.physicalDeviceExtensions.maintenance3)
                {
                    enabledExtensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
                }

                if (state.physicalDeviceExtensions.bindMemory2)
                {
                    enabledExtensions.push_back(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME);
                }

                if (state.physicalDeviceExtensions.memoryBudget)
                {
                    enabledExtensions.push_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
                }

                VkPhysicalDeviceFeatures2 features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
                vkGetPhysicalDeviceFeatures2(state.physicalDevice, &features);

                // Enable device features we might care about.
                {
                    VkPhysicalDeviceFeatures enabledFeatures = {};
                    if (features.features.textureCompressionETC2)
                        enabledFeatures.textureCompressionETC2 = VK_TRUE;
                    if (features.features.textureCompressionBC)
                        enabledFeatures.textureCompressionBC = VK_TRUE;
                    if (features.features.textureCompressionASTC_LDR)
                        enabledFeatures.textureCompressionASTC_LDR = VK_TRUE;
                    if (features.features.fullDrawIndexUint32)
                        enabledFeatures.fullDrawIndexUint32 = VK_TRUE;
                    if (features.features.multiDrawIndirect)
                        enabledFeatures.multiDrawIndirect = VK_TRUE;
                    if (features.features.imageCubeArray)
                        enabledFeatures.imageCubeArray = VK_TRUE;
                    if (features.features.fillModeNonSolid)
                        enabledFeatures.fillModeNonSolid = VK_TRUE;
                    if (features.features.independentBlend)
                        enabledFeatures.independentBlend = VK_TRUE;
                    if (features.features.sampleRateShading)
                        enabledFeatures.sampleRateShading = VK_TRUE;
                    if (features.features.fragmentStoresAndAtomics)
                        enabledFeatures.fragmentStoresAndAtomics = VK_TRUE;
                    if (features.features.shaderStorageImageExtendedFormats)
                        enabledFeatures.shaderStorageImageExtendedFormats = VK_TRUE;
                    if (features.features.shaderStorageImageMultisample)
                        enabledFeatures.shaderStorageImageMultisample = VK_TRUE;
                    if (features.features.largePoints)
                        enabledFeatures.largePoints = VK_TRUE;
                    if (features.features.shaderInt16)
                        enabledFeatures.shaderInt16 = VK_TRUE;
                    if (features.features.shaderInt64)
                        enabledFeatures.shaderInt64 = VK_TRUE;

                    if (features.features.shaderSampledImageArrayDynamicIndexing)
                        enabledFeatures.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
                    if (features.features.shaderUniformBufferArrayDynamicIndexing)
                        enabledFeatures.shaderUniformBufferArrayDynamicIndexing = VK_TRUE;
                    if (features.features.shaderStorageBufferArrayDynamicIndexing)
                        enabledFeatures.shaderStorageBufferArrayDynamicIndexing = VK_TRUE;
                    if (features.features.shaderStorageImageArrayDynamicIndexing)
                        enabledFeatures.shaderStorageImageArrayDynamicIndexing = VK_TRUE;

                    features.features = enabledFeatures;
                }

                VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
                createInfo.pNext = &features;
                createInfo.queueCreateInfoCount = queueFamilyCount;
                createInfo.pQueueCreateInfos = queueCreateInfo;
                createInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
                createInfo.ppEnabledExtensionNames = enabledExtensions.data();

                if (vkCreateDevice(state.physicalDevice, &createInfo, nullptr, &state.device) != VK_SUCCESS)
                {
                    return false;
                }

                LogInfo("Created VkDevice with adapter '%s' API version: %u.%u.%u",
                    state.physicalDeviceProperties.deviceName,
                    VK_VERSION_MAJOR(state.physicalDeviceProperties.apiVersion),
                    VK_VERSION_MINOR(state.physicalDeviceProperties.apiVersion),
                    VK_VERSION_PATCH(state.physicalDeviceProperties.apiVersion));
                for (auto* ext_name : enabledExtensions)
                {
                    LogInfo("Device extension '%s'", ext_name);
                }

                volkLoadDeviceTable(&state.deviceTable, state.device);
                state.deviceTable.vkGetDeviceQueue(state.device, state.queueFamilyIndices.graphicsFamily, graphicsQueueIndex, &state.graphicsQueue);
                state.deviceTable.vkGetDeviceQueue(state.device, state.queueFamilyIndices.computeFamily, computeQueueIndex, &state.computeQueue);
                state.deviceTable.vkGetDeviceQueue(state.device, state.queueFamilyIndices.transferFamily, copyQueueIndex, &state.copyQueue);

                // Create memory allocator.
                {
                    VmaAllocatorCreateInfo createInfo{};
                    createInfo.physicalDevice = state.physicalDevice;
                    createInfo.device = state.device;
                    createInfo.instance = state.instance;

                    if (state.physicalDeviceExtensions.getMemoryRequirements2 &&
                        state.physicalDeviceExtensions.dedicatedAllocation)
                    {
                        createInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
                    }

                    if (state.physicalDeviceExtensions.bindMemory2)
                    {
                        createInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;
                    }

                    if (state.physicalDeviceExtensions.memoryBudget)
                    {
                        createInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
                    }

                    VkResult result = vmaCreateAllocator(&createInfo, &state.memoryAllocator);
                    if (result != VK_SUCCESS)
                    {
                        //VK_THROW(result, "Cannot create allocator");
                        return false;
                    }
                }

                return true;
            }

            static void VulkanShutdown()
            {
                if (state.instance == VK_NULL_HANDLE)
                    return;

                vkDeviceWaitIdle(state.device);

                if (state.memoryAllocator != VK_NULL_HANDLE)
                {
                    VmaStats stats;
                    vmaCalculateStats(state.memoryAllocator, &stats);

                    if (stats.total.usedBytes > 0) {
                        LogInfo("Total device memory leaked: %llx bytes.", stats.total.usedBytes);
                    }

                    vmaDestroyAllocator(state.memoryAllocator);
                }

                vkDestroyDevice(state.device, NULL);

                if (state.debugUtilsMessenger != VK_NULL_HANDLE) {
                    vkDestroyDebugUtilsMessengerEXT(state.instance, state.debugUtilsMessenger, nullptr);
                }

                vkDestroyInstance(state.instance, nullptr);
                state.instance = VK_NULL_HANDLE;
                memset(&state, 0, sizeof(state));
            }

            void VulkanSetObjectName(VkObjectType objectType, uint64_t objectHandle, const char* objectName)
            {
                if (!state.debugUtils)
                {
                    return;
                }

                VkDebugUtilsObjectNameInfoEXT info;
                info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
                info.pNext = nullptr;
                info.objectType = objectType;
                info.objectHandle = objectHandle;
                info.pObjectName = objectName;
                vkSetDebugUtilsObjectNameEXT(state.device, &info);
            }

            static ContextHandle VulkanCreateContext(const ContextInfo& info) {
                if (state.contexts.isFull()) {
                    LogError("Not enough free context slots.");
                    return kInvalidContext;
                }

                VkResult result = VK_SUCCESS;

                const int id = state.contexts.alloc();
                Context& context = state.contexts[id];
                context.surfaceFormat.format = VK_FORMAT_UNDEFINED;
                context.surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                ContextHandle contextHandle = { (uint32_t)id };

#if defined(VK_USE_PLATFORM_WIN32_KHR)
                VkWin32SurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
                createInfo.hinstance = GetModuleHandle(NULL);
                createInfo.hwnd = reinterpret_cast<HWND>(info.handle);
                result = vkCreateWin32SurfaceKHR(state.instance, &createInfo, nullptr, &context.surface);
#endif

                if (result != VK_SUCCESS)
                {
                    LogError("Failed to create surface for SwapChain");
                    state.contexts.dealloc(contextHandle.value);
                    return kInvalidContext;
                }

                VkBool32 surfaceSupported = false;
                result = vkGetPhysicalDeviceSurfaceSupportKHR(state.physicalDevice,
                    state.queueFamilyIndices.graphicsFamily,
                    context.surface,
                    &surfaceSupported);

                if (result != VK_SUCCESS || !surfaceSupported)
                {
                    vkDestroySurfaceKHR(state.instance, context.surface, nullptr);
                    state.contexts.dealloc(contextHandle.value);
                    return kInvalidContext;
                }

                if (!ResizeContext(contextHandle, info.width, info.height)) {
                    state.contexts.dealloc(contextHandle.value);
                    return kInvalidContext;
                }

                return contextHandle;
            }

            static void VulkanDestroyContext(ContextHandle handle)
            {
                Context& context = state.contexts[handle.value];

                if (context.handle != VK_NULL_HANDLE)
                {
                    state.deviceTable.vkDestroySwapchainKHR(state.device, context.handle, nullptr);
                    context.handle = VK_NULL_HANDLE;
                }

                // Destroy surface
                if (context.surface != VK_NULL_HANDLE)
                {
                    vkDestroySurfaceKHR(state.instance, context.surface, nullptr);
                    context.surface = VK_NULL_HANDLE;
                }

                state.contexts.dealloc(handle.value);
            }

            static VkPresentModeKHR ChooseSwapPresentMode(
                const std::vector<VkPresentModeKHR>& availablePresentModes, bool vsyncEnabled)
            {
                // Try to match the correct present mode to the vsync state.
                std::vector<VkPresentModeKHR> desiredModes;
                if (vsyncEnabled) desiredModes = { VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_FIFO_RELAXED_KHR };
                else desiredModes = { VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_MAILBOX_KHR };

                // Iterate over all available present mdoes and match to one of the desired ones.
                for (const auto& availablePresentMode : availablePresentModes)
                {
                    for (auto mode : desiredModes)
                    {
                        if (availablePresentMode == mode)
                            return availablePresentMode;
                    }
                }

                // If no match was found, return the first present mode or default to FIFO.
                if (availablePresentModes.size() > 0) return availablePresentModes[0];
                else return VK_PRESENT_MODE_FIFO_KHR;
            }

            static bool VulkanResizeContext(ContextHandle handle, uint32_t width, uint32_t height) {
                Context& context = state.contexts[handle.value];

                VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo = {
                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
                    nullptr,
                    context.surface
                };

                uint32_t format_count;
                std::vector<VkSurfaceFormatKHR> formats;

                if (state.surfaceCapabilities2)
                {
                    if (vkGetPhysicalDeviceSurfaceFormats2KHR(state.physicalDevice, &surfaceInfo, &format_count, nullptr) != VK_SUCCESS)
                        return false;

                    std::vector<VkSurfaceFormat2KHR> formats2(format_count);

                    for (VkSurfaceFormat2KHR& format2 : formats2)
                    {
                        format2 = {};
                        format2.sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
                    }

                    if (vkGetPhysicalDeviceSurfaceFormats2KHR(state.physicalDevice, &surfaceInfo, &format_count, formats2.data()) != VK_SUCCESS)
                        return false;

                    formats.reserve(format_count);
                    for (auto& f : formats2)
                    {
                        formats.push_back(f.surfaceFormat);
                    }
                }
                else
                {
                    if (vkGetPhysicalDeviceSurfaceFormatsKHR(state.physicalDevice, context.surface, &format_count, nullptr) != VK_SUCCESS)
                        return false;
                    formats.resize(format_count);
                    if (vkGetPhysicalDeviceSurfaceFormatsKHR(state.physicalDevice, context.surface, &format_count, formats.data()) != VK_SUCCESS)
                        return false;
                }

                const bool srgb = false;
                if (format_count == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
                {
                    context.surfaceFormat = formats[0];
                    context.surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
                }
                else
                {
                    if (format_count == 0)
                    {
                        LogError("Vulkan: Surface has no formats.");
                        return false;
                    }

                    bool found = false;
                    for (uint32_t i = 0; i < format_count; i++)
                    {
                        if (srgb)
                        {
                            if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB ||
                                formats[i].format == VK_FORMAT_B8G8R8A8_SRGB ||
                                formats[i].format == VK_FORMAT_A8B8G8R8_SRGB_PACK32)
                            {
                                context.surfaceFormat = formats[i];
                                found = true;
                            }
                        }
                        else
                        {
                            if (formats[i].format == VK_FORMAT_R8G8B8A8_UNORM ||
                                formats[i].format == VK_FORMAT_B8G8R8A8_UNORM ||
                                formats[i].format == VK_FORMAT_A8B8G8R8_UNORM_PACK32)
                            {
                                context.surfaceFormat = formats[i];
                                found = true;
                            }
                        }
                    }

                    if (!found)
                        context.surfaceFormat = formats[0];
                }

                VkSurfaceCapabilitiesKHR capabilities;
                if (state.surfaceCapabilities2)
                {
                    VkSurfaceCapabilities2KHR surfaceCapabilities2 = { VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR };

                    // TODO: Add fullscreen exclusive.

                    if (vkGetPhysicalDeviceSurfaceCapabilities2KHR(state.physicalDevice, &surfaceInfo, &surfaceCapabilities2) != VK_SUCCESS)
                        return false;

                    capabilities = surfaceCapabilities2.surfaceCapabilities;
                }
                else
                {
                    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state.physicalDevice, context.surface, &capabilities) != VK_SUCCESS)
                        return false;
                }

                if (capabilities.maxImageExtent.width == 0
                    && capabilities.maxImageExtent.height == 0)
                {
                    return false;
                }

                /* Choose present mode. */
                uint32_t num_present_modes;
                std::vector<VkPresentModeKHR> presentModes;
                if (vkGetPhysicalDeviceSurfacePresentModesKHR(state.physicalDevice, context.surface, &num_present_modes, nullptr) != VK_SUCCESS)
                    return false;
                presentModes.resize(num_present_modes);
                if (vkGetPhysicalDeviceSurfacePresentModesKHR(state.physicalDevice, context.surface, &num_present_modes, presentModes.data()) != VK_SUCCESS)
                    return false;

                const bool tripleBuffer = false;
                uint32_t minImageCount = (tripleBuffer) ? 3 : capabilities.minImageCount + 1;
                if (capabilities.maxImageCount > 0 &&
                    minImageCount > capabilities.maxImageCount)
                {
                    minImageCount = capabilities.maxImageCount;
                }

                // Choose swapchain extent (Size)
                VkExtent2D newExtent = {};
                if (capabilities.currentExtent.width != UINT32_MAX ||
                    capabilities.currentExtent.height != UINT32_MAX ||
                    width == 0 || height == 0)
                {
                    newExtent = capabilities.currentExtent;
                }
                else
                {
                    newExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, width));
                    newExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, height));
                }
                newExtent.width = std::max(newExtent.width, 1u);
                newExtent.height = std::max(newExtent.height, 1u);

                // Enable transfer source and destination on swap chain images if supported
                VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                if (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
                    imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
                }
                if (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
                    imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                }

                VkSurfaceTransformFlagBitsKHR preTransform;
                if ((capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) != 0) {
                    // We prefer a non-rotated transform
                    preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
                }
                else {
                    preTransform = capabilities.currentTransform;
                }

                VkCompositeAlphaFlagBitsKHR compositeMode = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
                if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
                    compositeMode = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
                if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
                    compositeMode = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
                if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
                    compositeMode = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
                if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
                    compositeMode = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;

                VkPresentModeKHR presentMode = ChooseSwapPresentMode(presentModes, true);

                VkSwapchainKHR oldSwapchain = context.handle;

                VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
                createInfo.surface = context.surface;
                createInfo.minImageCount = minImageCount;
                createInfo.imageFormat = context.surfaceFormat.format;
                createInfo.imageColorSpace = context.surfaceFormat.colorSpace;
                createInfo.imageExtent = newExtent;
                createInfo.imageArrayLayers = 1;
                createInfo.imageUsage = imageUsage;
                createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                createInfo.preTransform = preTransform;
                createInfo.compositeAlpha = compositeMode;
                createInfo.presentMode = presentMode;
                createInfo.clipped = VK_TRUE;
                createInfo.oldSwapchain = oldSwapchain;

                VkResult result = state.deviceTable.vkCreateSwapchainKHR(state.device, &createInfo, nullptr, &context.handle);
                if (result != VK_SUCCESS)
                {
                    //VK_THROW(result, "Cannot create Swapchain");
                    return false;
                }

                LogDebug("[Vulkan]: Created SwapChain");

                if (oldSwapchain != VK_NULL_HANDLE)
                {
                    state.deviceTable.vkDestroySwapchainKHR(state.device, oldSwapchain, nullptr);
                }

                // Get SwapChain images
                if (state.deviceTable.vkGetSwapchainImagesKHR(state.device, context.handle, &context.imageCount, nullptr) != VK_SUCCESS) {
                    return false;
                }

                VkImage swapChainImages[16] = {};
                result = state.deviceTable.vkGetSwapchainImagesKHR(state.device, context.handle, &context.imageCount, swapChainImages);
                if (result != VK_SUCCESS)
                {
                    //VK_THROW(result, "[Vulkan]: Failed to retrive SwapChain-Images");
                    return false;
                }

                context.frameIndex = 0;
                context.semaphoreIndex = 0;
                context.frames = (Frame*)malloc(sizeof(Frame) * context.imageCount);
                for (uint32_t i = 0; i < context.imageCount; i++)
                {
                    context.frames[i].index = i;

                    VkCommandPoolCreateInfo commandPoolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
                    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                    commandPoolInfo.queueFamilyIndex = state.queueFamilyIndices.graphicsFamily;
                    if (state.deviceTable.vkCreateCommandPool(state.device, &commandPoolInfo, nullptr, &context.frames[i].commandPool) != VK_SUCCESS) {
                        return false;
                    }

                    VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
                    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
                    if (vkCreateFence(state.device, &fenceInfo, NULL, &context.frames[i].fence)) {
                        return false;
                    }
                }

                return true;
            }

            static bool VulkanBeginFrame(ContextHandle handle) {
                return true;
            }

            static void VulkanEndFrame(ContextHandle handle) {

            }

            Renderer* CreateRenderer() {
                static Renderer renderer = { nullptr };
                renderer.IsSupported = VulkanIsSupported;
                renderer.Init = VulkanInit;
                renderer.Shutdown = VulkanShutdown;
                renderer.CreateContext = VulkanCreateContext;
                renderer.DestroyContext = VulkanDestroyContext;
                renderer.ResizeContext = VulkanResizeContext;
                renderer.BeginFrame = VulkanBeginFrame;
                renderer.EndFrame = VulkanEndFrame;
                return &renderer;
            }
        }
    }
}
