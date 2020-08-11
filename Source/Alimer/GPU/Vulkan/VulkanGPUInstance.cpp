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
#include "VulkanGPUInstance.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
#   define GLFW_INCLUDE_NONE
#   include <GLFW/glfw3.h>
#endif

using namespace eastl;

namespace
{
    bool ValidateLayers(const vector<const char*>& required, const  vector<VkLayerProperties>& available)
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
            {"VK_LAYER_LUNARG_core_validation"}
        };

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

    bool QueryPresentationSupport(VkInstance instance, VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex)
    {
#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
        return glfwGetPhysicalDevicePresentationSupport(instance, physicalDevice, queueFamilyIndex);
#else
        return true;
#endif
    }
}

bool VulkanGPUInstance::IsAvailable()
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

VulkanGPUInstance::VulkanGPUInstance(const eastl::string& applicationName)
{
    ALIMER_VERIFY(IsAvailable());

    vector<const char*> enabledExtensions;
    vector<const char*> enabledLayers;

    {
        uint32_t instanceExtensionCount;
        VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr));

        vector<VkExtensionProperties> availableInstanceExtensions(instanceExtensionCount);
        VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, availableInstanceExtensions.data()));

        for (auto& availableExtension : availableInstanceExtensions)
        {
            if (strcmp(availableExtension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
                extensions.debugUtils = true;
#if defined(GPU_DEBUG) || defined(VULKAN_VALIDATION_LAYERS)
                enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
            }
            else if (strcmp(availableExtension.extensionName, VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME) == 0)
            {
                extensions.headless = true;
            }
            else if (strcmp(availableExtension.extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0)
            {
                // VK_KHR_get_physical_device_properties2 is a prerequisite of VK_KHR_performance_query
                // which will be used for stats gathering where available.
                extensions.get_physical_device_properties2 = true;
                enabledExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
            }
            else if (strcmp(availableExtension.extensionName, VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME) == 0)
            {
                extensions.get_surface_capabilities2 = true;
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
    }

    const bool headless = false;
    if (!headless)
    {
        enabledExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

        if (extensions.get_surface_capabilities2)
        {
            enabledExtensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
        }
    }

    VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.pApplicationName = applicationName.c_str();
    appInfo.applicationVersion = 0;
    appInfo.pEngineName = "Alimer";
    appInfo.engineVersion = VK_MAKE_VERSION(ALIMER_VERSION_MAJOR, ALIMER_VERSION_MINOR, ALIMER_VERSION_PATCH);
    appInfo.apiVersion = volkGetInstanceVersion();

    VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };

#if defined(GPU_DEBUG) || defined(VULKAN_VALIDATION_LAYERS)
    VkDebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };

    if (extensions.debugUtils)
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
    VkResult result = vkCreateInstance(&createInfo, nullptr, &handle);

    if (result != VK_SUCCESS)
    {
        VK_LOG_ERROR(result, "Could not create Vulkan instance");
    }

    volkLoadInstance(handle);

#if defined(GPU_DEBUG) || defined(VULKAN_VALIDATION_LAYERS)
    if (extensions.debugUtils)
    {
        result = vkCreateDebugUtilsMessengerEXT(handle, &debugUtilsCreateInfo, nullptr, &debugUtilsMessenger);
        if (result != VK_SUCCESS)
        {
            VK_LOG_ERROR(result, "Could not create debug utils messenger");
        }
    }
#endif
}

VulkanGPUInstance::~VulkanGPUInstance()
{
#if defined(GPU_DEBUG) || defined(VULKAN_VALIDATION_LAYERS)
    if (debugUtilsMessenger != VK_NULL_HANDLE)
    {
        vkDestroyDebugUtilsMessengerEXT(handle, debugUtilsMessenger, nullptr);
    }
#endif

    if (handle != VK_NULL_HANDLE)
    {
        vkDestroyInstance(handle, nullptr);
    }

    handle = VK_NULL_HANDLE;
}
