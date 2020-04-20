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

#if TODO
#include "VulkanGraphicsProvider.h"
#include "VulkanGraphicsAdapter.h"
#include "core/Assert.h"
#include "core/Log.h"
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
                //LOGW("{} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
            }
            else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            {
                //LOGE("{} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
            }
            return VK_FALSE;
        }

        static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT /*type*/,
            uint64_t /*object*/, size_t /*location*/, int32_t /*message_code*/,
            const char* layer_prefix, const char* message, void* /*user_data*/)
        {
            if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
            {
                //LOGE("{}: {}", layer_prefix, message);
            }
            else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
            {
                //LOGW("{}: {}", layer_prefix, message);
            }
            else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
            {
                //LOGW("{}: {}", layer_prefix, message);
            }
            else
            {
                //LOGI("{}: {}", layer_prefix, message);
            }
            return VK_FALSE;
        }

        static bool HasLayers(const vector<const char*>& required, const vector<VkLayerProperties>& available)
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

        vector<const char*> GetOptimalValidationLayers(const vector<VkLayerProperties>& supported_instance_layers)
        {
            vector<vector<const char*>> validationLayerPriorityList =
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
    }

    bool VulkanGraphicsProvider::IsAvailable()
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

    VulkanGraphicsProvider::VulkanGraphicsProvider(const std::string& applicationName, GraphicsProviderFlags flags)
    {
        ALIMER_ASSERT(IsAvailable());

        features.apiVersion = volkGetInstanceVersion();

        VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
        appInfo.pApplicationName = applicationName.c_str();
        appInfo.applicationVersion = 0;
        appInfo.pEngineName = "Alimer";
        appInfo.engineVersion = 0;
        if (features.apiVersion >= VK_API_VERSION_1_2) {
            appInfo.apiVersion = VK_API_VERSION_1_2;
        }
        else if (features.apiVersion >= VK_API_VERSION_1_1) {
            appInfo.apiVersion = VK_API_VERSION_1_1;
        }
        else {
            appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 57);
        }
        const bool headless = any(flags & GraphicsProviderFlags::Headless);

        // Enumerate supported extensions and setup instance extensions.
        std::vector<const char*> enabledExtensions;
        {
            uint32_t instanceExtensionCount;
            VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr));

            vector<VkExtensionProperties> availableInstanceExtensions(instanceExtensionCount);
            VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, availableInstanceExtensions.data()));

            for (auto& available_extension : availableInstanceExtensions)
            {
                if (strcmp(available_extension.extensionName, "VK_KHR_get_physical_device_properties2") == 0)
                {
                    features.physicalDeviceProperties2 = true;
                    enabledExtensions.push_back("VK_KHR_get_physical_device_properties2");
                }
                else if (strcmp(available_extension.extensionName, "VK_KHR_external_memory_capabilities") == 0)
                {
                    features.externalMemoryCapabilities = true;
                }
                else if (strcmp(available_extension.extensionName, "VK_KHR_external_semaphore_capabilities") == 0)
                {
                    features.externalSemaphoreCapabilities = true;
                }
                else if (strcmp(available_extension.extensionName, "VK_EXT_debug_utils") == 0)
                {
                    features.debugUtils = true;
                }
                else if (strcmp(available_extension.extensionName, "VK_EXT_headless_surface") == 0)
                {
                    features.headless = true;
                }
                else if (strcmp(available_extension.extensionName, "VK_KHR_surface"))
                {
                    features.surface = true;
                }
                else if (strcmp(available_extension.extensionName, "VK_KHR_get_surface_capabilities2"))
                {
                    features.surfaceCapabilities2 = true;
                }
            }

            if (features.physicalDeviceProperties2 &&
                features.externalMemoryCapabilities &&
                features.externalSemaphoreCapabilities)
            {
                enabledExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
                enabledExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
            }

            if (features.debugUtils)
            {
                enabledExtensions.push_back("VK_EXT_debug_utils");
            }
            else
            {
                enabledExtensions.push_back("VK_EXT_debug_report");
            }

            // Try to enable headless surface extension if it exists
            if (headless)
            {
                if (!features.headless)
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
                enabledExtensions.push_back("VK_KHR_surface");
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

                if (features.surfaceCapabilities2)
                {
                    enabledExtensions.push_back("VK_KHR_get_surface_capabilities2");
                }
            }
        }

        const bool validation =
            any(flags & GraphicsProviderFlags::Validation) |
            any(flags & GraphicsProviderFlags::GPUBasedValidation);

        vector<const char*> enabledLayers;

        if (validation)
        {
            uint32_t layerCount;
            VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

            vector<VkLayerProperties> queriedLayers(layerCount);
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
        if (features.debugUtils)
        {
            debugUtilsCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
            debugUtilsCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
            debugUtilsCreateInfo.pfnUserCallback = DebugUtilsMessengerCallback;

            instanceInfo.pNext = &debugUtilsCreateInfo;
        }
        else
        {
            debugReportCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
            debugReportCreateInfo.pfnCallback = DebugReportCallback;

            instanceInfo.pNext = &debugReportCreateInfo;
        }

        // Create the Vulkan instance
        VkResult result = vkCreateInstance(&instanceInfo, nullptr, &instance);

        if (result != VK_SUCCESS)
        {
            VK_THROW(result, "Could not create Vulkan instance");
        }

        volkLoadInstance(instance);

        if (features.debugUtils)
        {
            result = vkCreateDebugUtilsMessengerEXT(instance, &debugUtilsCreateInfo, nullptr, &debugUtilsMessenger);
            if (result != VK_SUCCESS)
            {
                //ALIMER_LOGE(result, "Could not create debug utils messenger");
            }
        }
        else
        {
            result = vkCreateDebugReportCallbackEXT(instance, &debugReportCreateInfo, nullptr, &debugReportCallback);
            if (result != VK_SUCCESS)
            {
                //ALIMER_LOGE(result, "Could not create debug report callback");
            }
        }
    }

    VulkanGraphicsProvider::~VulkanGraphicsProvider()
    {
        if (instance == VK_NULL_HANDLE)
            return;

        if (debugUtilsMessenger != VK_NULL_HANDLE) {
            vkDestroyDebugUtilsMessengerEXT(instance, debugUtilsMessenger, nullptr);
        }

        if (debugReportCallback != VK_NULL_HANDLE)
        {
            vkDestroyDebugReportCallbackEXT(instance, debugReportCallback, nullptr);
        }

        vkDestroyInstance(instance, nullptr);
        instance = VK_NULL_HANDLE;
    }

    vector<unique_ptr<GraphicsAdapter>> VulkanGraphicsProvider::EnumerateGraphicsAdapters()
    {
        vector<unique_ptr<GraphicsAdapter>> adapters;

        uint32_t deviceCount;
        if (vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr) != VK_SUCCESS ||
            deviceCount == 0)
        {
            return adapters;
        }

        vector<VkPhysicalDevice> physicalDevices(deviceCount);
        if (vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data()) != VK_SUCCESS) {
            return adapters;
        }

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

            adapters.push_back(make_unique<VulkanGraphicsAdapter>(this, physicalDevices[i]));
        }

        return adapters;
    }
}

#endif // TODO
