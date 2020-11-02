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

#include "AlimerConfig.h"
#include "Graphics/Graphics.h"
#include "Platform/Window.h"
#include "VulkanBackend.h"
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

namespace alimer
{
    namespace
    {
#if defined(_DEBUG)
        VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
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
                    LOGE("Validation Layer '%s' not found", layer);
                    return false;
                }
            }

            return true;
        }

        std::vector<const char*> GetOptimalValidationLayers(const std::vector<VkLayerProperties>& supportedInstanceLayers)
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
    }

    struct InstanceFeatures
    {
        bool debugUtils                   = false;
        bool headless                     = false;
        bool getPhysicalDeviceProperties2 = false;
        bool getSurfaceCapabilities2      = false;
    };

    struct GraphicsImpl
    {
        InstanceFeatures instanceFeatues;
        VkInstance       instance{VK_NULL_HANDLE};
#if defined(_DEBUG)
        VkDebugUtilsMessengerEXT debugUtilsMessenger{VK_NULL_HANDLE};
#endif
        VkSurfaceKHR surface{VK_NULL_HANDLE};
    };

    Graphics::Graphics(Window& window, const GraphicsSettings& settings) :
        window{window},
        colorFormat(settings.colorFormat),
        depthStencilFormat(settings.depthStencilFormat),
        enableDebugLayer(settings.enableDebugLayer),
        verticalSync(settings.verticalSync)
    {
        VkResult result = volkInitialize();
        if (result)
        {
            VK_THROW(result, "Failed to initialize volk.");
        }

        apiData = new GraphicsImpl();

        // Create instance first.
        {
            const bool               headless = false;
            std::vector<const char*> enabledInstanceLayers;
            std::vector<const char*> enabledInstanceExtensions;

#if defined(_DEBUG)
            if (settings.enableDebugLayer)
            {
                uint32_t instanceLayerCount;
                VK_CHECK(vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr));

                std::vector<VkLayerProperties> supportedInstanceLayers(instanceLayerCount);
                VK_CHECK(vkEnumerateInstanceLayerProperties(&instanceLayerCount, supportedInstanceLayers.data()));

                std::vector<const char*> optimalValidationLayers = GetOptimalValidationLayers(supportedInstanceLayers);
                enabledInstanceLayers.insert(enabledInstanceLayers.end(), optimalValidationLayers.begin(), optimalValidationLayers.end());
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
                    apiData->instanceFeatues.debugUtils = true;
                }
                else if (strcmp(availableExtension.extensionName, VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME) == 0)
                {
                    apiData->instanceFeatues.headless = true;
                }
                else if (strcmp(availableExtension.extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0)
                {
                    // VK_KHR_get_physical_device_properties2 is a prerequisite of VK_KHR_performance_query
                    // which will be used for stats gathering where available.
                    apiData->instanceFeatues.getPhysicalDeviceProperties2 = true;
                    enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
                }
                else if (strcmp(availableExtension.extensionName, VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME) == 0)
                {
                    apiData->instanceFeatues.getSurfaceCapabilities2 = true;
                }
            }

            if (headless)
            {
                if (apiData->instanceFeatues.headless)
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

                if (apiData->instanceFeatues.getSurfaceCapabilities2)
                {
                    enabledInstanceExtensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
                }
            }

#if defined(_DEBUG)
            if (settings.enableDebugLayer && apiData->instanceFeatues.debugUtils)
            {
                enabledInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
#endif

            VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
            appInfo.pApplicationName   = settings.applicationName.c_str();
            appInfo.applicationVersion = 0;
            appInfo.pEngineName        = "Alimer Engine";
            appInfo.engineVersion      = VK_MAKE_VERSION(ALIMER_VERSION_MAJOR, ALIMER_VERSION_MINOR, ALIMER_VERSION_PATCH);
            appInfo.apiVersion         = volkGetInstanceVersion();

            VkInstanceCreateInfo instanceCreateInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
            instanceCreateInfo.pApplicationInfo        = &appInfo;
            instanceCreateInfo.enabledLayerCount       = static_cast<uint32_t>(enabledInstanceLayers.size());
            instanceCreateInfo.ppEnabledLayerNames     = enabledInstanceLayers.data();
            instanceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(enabledInstanceExtensions.size());
            instanceCreateInfo.ppEnabledExtensionNames = enabledInstanceExtensions.data();

#if defined(_DEBUG)
            VkDebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
            if (settings.enableDebugLayer && apiData->instanceFeatues.debugUtils)
            {
                debugUtilsCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
                debugUtilsCreateInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
                debugUtilsCreateInfo.pfnUserCallback = DebugUtilsMessengerCallback;

                instanceCreateInfo.pNext = &debugUtilsCreateInfo;
            }
#endif

            // Create the Vulkan instance
            result = vkCreateInstance(&instanceCreateInfo, nullptr, &apiData->instance);

            if (result != VK_SUCCESS)
            {
                VK_THROW(result, "Could not create Vulkan instance");
            }

            volkLoadInstance(apiData->instance);

#if defined(_DEBUG)
            if (settings.enableDebugLayer && apiData->instanceFeatues.debugUtils)
            {
                result = vkCreateDebugUtilsMessengerEXT(apiData->instance, &debugUtilsCreateInfo, nullptr, &apiData->debugUtilsMessenger);
                if (result != VK_SUCCESS)
                {
                    VK_THROW(result, "Could not create debug utils messenger");
                }
            }
#endif

            LOGI("Created VkInstance with version: {}.{}.{}", VK_VERSION_MAJOR(appInfo.apiVersion), VK_VERSION_MINOR(appInfo.apiVersion), VK_VERSION_PATCH(appInfo.apiVersion));
            if (instanceCreateInfo.enabledLayerCount)
            {
                for (uint32_t i = 0; i < instanceCreateInfo.enabledLayerCount; ++i)
                {
                    LOGI("Instance layer '{}'", instanceCreateInfo.ppEnabledLayerNames[i]);
                }
            }

            for (uint32_t i = 0; i < instanceCreateInfo.enabledExtensionCount; ++i)
            {
                LOGI("Instance extension '{}'", instanceCreateInfo.ppEnabledExtensionNames[i]);
            }
        }

        {
#ifdef VK_USE_PLATFORM_ANDROID_KHR
#elif VK_USE_PLATFORM_WIN32_KHR
            VkWin32SurfaceCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
            createInfo.hinstance = GetModuleHandle(nullptr);
            createInfo.hwnd = window.GetHandle();

            VkResult result = vkCreateWin32SurfaceKHR(apiData->instance, &createInfo, nullptr, &apiData->surface);
#elif VK_USE_PLATFORM_MACOS_MVK
#endif

            if (result != VK_SUCCESS)
            {
                VK_THROW(result, "Could not create surface");
            }
        }
    }

    void Graphics::Shutdown()
    {
        if (apiData->surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(apiData->instance, apiData->surface, nullptr);
        }

#if defined(_DEBUG)
        if (apiData->debugUtilsMessenger != VK_NULL_HANDLE)
        {
            vkDestroyDebugUtilsMessengerEXT(apiData->instance, apiData->debugUtilsMessenger, nullptr);
        }
#endif

        if (apiData->instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(apiData->instance, nullptr);
        }

        delete apiData;
        apiData = nullptr;
    }
}
