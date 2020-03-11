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

#include "Graphics/GraphicsDevice.h"
#include "vulkan_backend.h"
#include <EASTL/vector.h>

using namespace eastl;

namespace alimer
{
    bool hasLayers(const vector<const char*>& required, const vector<VkLayerProperties>& available)
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

    vector<const char*> getOptimalValidationLayers(const vector<VkLayerProperties>& supported_instance_layers)
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
            {"VK_LAYER_LUNARG_core_validation"} };

        for (auto& validation_layers : validationLayerPriorityList)
        {
            if (hasLayers(validation_layers, supported_instance_layers))
            {
                return validation_layers;
            }

            ALIMER_LOGW("Couldn't enable validation layers (see log for error) - falling back");
        }

        // Else return nothing
        return {};
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT                  messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        auto* context = static_cast<GraphicsDevice*>(pUserData);

        switch (messageSeverity)
        {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            if (messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
            {
                ALIMER_LOGE("[Vulkan]: Validation Error: %s", pCallbackData->pMessage);
                context->notify_validation_error(pCallbackData->pMessage);
            }
            else
                ALIMER_LOGE("[Vulkan]: Other Error: %s", pCallbackData->pMessage);
            break;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            if (messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
                ALIMER_LOGW("[Vulkan]: Validation Warning: %s", pCallbackData->pMessage);
            else
                ALIMER_LOGW("[Vulkan]: Other Warning: %s", pCallbackData->pMessage);
            break;

        default:
            return VK_FALSE;
        }

        bool log_object_names = false;
        for (uint32_t i = 0; i < pCallbackData->objectCount; i++)
        {
            auto* name = pCallbackData->pObjects[i].pObjectName;
            if (name)
            {
                log_object_names = true;
                break;
            }
        }

        if (log_object_names)
        {
            for (uint32_t i = 0; i < pCallbackData->objectCount; i++)
            {
                auto* name = pCallbackData->pObjects[i].pObjectName;
                ALIMER_LOGI("  Object #%u: %s", i, name ? name : "N/A");
            }
        }

        return VK_FALSE;
    }

    bool GraphicsDevice::backend_create()
    {
        VkResult result = volkInitialize();
        if (result != VK_SUCCESS)
        {
            ALIMER_LOGERROR("Failed to initialize volk.");
            return false;
        }

        // Setup application info.
        const bool enableValidationLayers = any(flags & GraphicsDeviceFlags::DebugRuntime) || any(flags & GraphicsDeviceFlags::GPUBasedValidation);

        bool api_version_12 = false;
        bool api_version_11 = false;
        VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
        appInfo.pApplicationName = applicationName.c_str();
        appInfo.applicationVersion = 0;
        appInfo.pEngineName = "Alimer";
        appInfo.engineVersion = 0;
        if (volkGetInstanceVersion() >= VK_API_VERSION_1_2) {
            appInfo.apiVersion = VK_API_VERSION_1_2;
            api_version_12 = true;
        }
        else if (volkGetInstanceVersion() >= VK_API_VERSION_1_1) {
            appInfo.apiVersion = VK_API_VERSION_1_1;
            api_version_11 = true;
        }
        else {
            appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 55);
        }

        // Create instance first.
        {
            uint32_t extensionCount;
            VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));

            vector<VkExtensionProperties> queriedExtensions(extensionCount);
            VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, queriedExtensions.data()));

            const auto hasExtension = [&](const char* name) -> bool {
                auto itr = find_if(begin(queriedExtensions), end(queriedExtensions), [name](const VkExtensionProperties& e) -> bool {
                    return strcmp(e.extensionName, name) == 0;
                    });
                return itr != end(queriedExtensions);
            };

            vector<const char*> instanceExtensions;
            // Try to enable headless surface extension if it exists
            if (headless)
            {
                if (!hasExtension(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME))
                {
                    ALIMER_LOGW("%s is not available, disabling swapchain creation", VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
                }
                else
                {
                    ALIMER_LOGI("%s is available, enabling it", VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
                    instanceExtensions.push_back(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
                }
            }
            else
            {
                instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
                // Enable surface extensions depending on os
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
                instanceExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
                instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(_DIRECT2DISPLAY)
                instanceExtensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
                instanceExtensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
                instanceExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_IOS_MVK)
                instanceExtensions.push_back(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
                // TODO: Support VK_EXT_metal_surface
                instanceExtensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#endif

                if (hasExtension(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME))
                {
                    instanceExtensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
                    features.surfaceCapabilities2 = true;
                }
            }

            if (hasExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
            {
                features.physicalDeviceProperties2 = true;
                instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
            }

            if (features.physicalDeviceProperties2
                && hasExtension(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME)
                && hasExtension(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME))
            {
                instanceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
                instanceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
                features.external = true;
            }

            if (hasExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
            {
                instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                features.debugUtils = true;
            }

            vector<const char*> instanceLayers;

            if (enableValidationLayers)
            {
                uint32_t layerCount;
                VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

                vector<VkLayerProperties> queriedLayers(layerCount);
                VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, queriedLayers.data()));

                instanceLayers = getOptimalValidationLayers(queriedLayers);
            }

            VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
            createInfo.pApplicationInfo = &appInfo;
            createInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size());
            createInfo.ppEnabledLayerNames = instanceLayers.data();
            createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
            createInfo.ppEnabledExtensionNames = instanceExtensions.data();

            // Create the Vulkan instance
            VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

            if (result != VK_SUCCESS)
            {
                VK_THROW(result, "Could not create Vulkan instance");
            }

            volkLoadInstance(instance);

            if (enableValidationLayers && features.debugUtils)
            {
                VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
                debugMessengerCreateInfo.messageSeverity =
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
                debugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
                debugMessengerCreateInfo.pfnUserCallback = vulkanDebugCallback;
                debugMessengerCreateInfo.pUserData = this;
                VK_CHECK(
                    vkCreateDebugUtilsMessengerEXT(instance, &debugMessengerCreateInfo, nullptr, &debug_messenger)
                    );
            }
        }

        return true;
    }

    void GraphicsDevice::backend_destroy()
    {
        if (instance == VK_NULL_HANDLE) {
            return;
        }

        /*mainContext.reset();

        SafeDelete(graphicsQueue);
        SafeDelete(computeQueue);
        SafeDelete(copyQueue);

        // Allocator
        D3D12MA::Stats stats;
        allocator->CalculateStats(&stats);

        if (stats.Total.UsedBytes > 0) {
            ALIMER_LOGERRORF("Total device memory leaked: %llu bytes.", stats.Total.UsedBytes);
        }

        SafeRelease(allocator);*/

        if (device != VK_NULL_HANDLE) {
            vkDestroyDevice(device, nullptr);
        }

        if (debug_messenger != VK_NULL_HANDLE) {
            vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
            debug_messenger = VK_NULL_HANDLE;
        }

        vkDestroyInstance(instance, nullptr);
        instance = VK_NULL_HANDLE;
    }

    void GraphicsDevice::wait_idle()
    {
        VK_CHECK(vkDeviceWaitIdle(device));
    }

    bool GraphicsDevice::begin_frame()
    {
        return true;
    }

    void GraphicsDevice::end_frame()
    {

    }
}
