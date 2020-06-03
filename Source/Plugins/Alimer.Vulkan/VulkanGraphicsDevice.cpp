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

#include "VulkanGraphicsDevice.h"
#include "Core/Vector.h"

namespace alimer
{
    namespace
    {
#if defined(VULKAN_DEBUG) || defined(VULKAN_VALIDATION_LAYERS)
        VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
            const VkDebugUtilsMessengerCallbackDataEXT * callback_data,
            void* user_data)
        {
            // Log debug messge
            if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            {
                LOG_WARN("%d - %s: %s", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
            }
            else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            {
                LOG_ERROR("%d - %s: %s", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
            }
            return VK_FALSE;
        }
#endif
    }

    VulkanGraphicsDevice::VulkanGraphicsDevice(bool validation)
        : GraphicsDevice()
    {
        VkResult result = volkInitialize();
        if (result)
        {
            LOG_VK_ERROR_MSG(result, "Failed to initialize volk.");
        }

        // Create instance first
        {
            uint32_t instanceExtensionCount;
            VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr));

            Vector<VkExtensionProperties> availableInstanceExtensions(instanceExtensionCount);
            VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, availableInstanceExtensions.Data()));

            for (uint32_t i = 0; i < instanceExtensionCount; ++i)
            {
                if (strcmp(availableInstanceExtensions[i].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
                    features.debugUtils = true;
                }
                else if (strcmp(availableInstanceExtensions[i].extensionName, VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME) == 0)
                {
                    features.headless = true;
                }
                else if (strcmp(availableInstanceExtensions[i].extensionName, VK_KHR_SURFACE_EXTENSION_NAME) == 0)
                {
                    features.surface = true;
                }
                else if (strcmp(availableInstanceExtensions[i].extensionName, VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME) == 0)
                {
                    features.surfaceCapabilities2 = true;
                }
                else if (strcmp(availableInstanceExtensions[i].extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0)
                {
                    features.physicalDeviceProperties2 = true;
                }
                else if (strcmp(availableInstanceExtensions[i].extensionName, VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME) == 0)
                {
                    features.externalMemoryCapabilities = true;
                }
                else if (strcmp(availableInstanceExtensions[i].extensionName, VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME) == 0)
                {
                    features.externalSemaphoreCapabilities = true;
                }
            }

            // Vectors for extensions and layers to enable.
            Vector<const char*> enabledInstanceExtensions;
            Vector<const char*> enabledInstanceLayers;

            if (features.physicalDeviceProperties2)
            {
                enabledInstanceExtensions.Push(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
            }

            if (features.physicalDeviceProperties2 &&
                features.externalMemoryCapabilities &&
                features.externalSemaphoreCapabilities)
            {
                enabledInstanceExtensions.Push(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
                enabledInstanceExtensions.Push(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
            }

            const bool headless = false;
            if (headless)
            {
                if (!features.headless)
                {
                    LOG_WARN("%s is not available, disabling swapchain creation", VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
                }
                else
                {
                    enabledInstanceExtensions.Push(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
                }
            }
            else
            {
                enabledInstanceExtensions.Push(VK_KHR_SURFACE_EXTENSION_NAME);
                // Enable surface extensions depending on os
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
                enabledInstanceExtensions.Push(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
                enabledInstanceExtensions.Push(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
                enabledInstanceExtensions.Push(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
                enabledInstanceExtensions.Push(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_IOS_MVK)
                enabledInstanceExtensions.Push(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
                // TODO: Support VK_EXT_metal_surface
                enabledInstanceExtensions.Push(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#endif

                if (features.surfaceCapabilities2)
                {
                    enabledInstanceExtensions.Push(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
                }
            }

#if defined(VULKAN_DEBUG) || defined(VULKAN_VALIDATION_LAYERS)
            if (features.debugUtils)
            {
                enabledInstanceExtensions.Push(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }

            if (validation)
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
                        enabledInstanceLayers.Push("VK_LAYER_KHRONOS_validation");
                        foundValidationLayer = true;
                        break;
                    }
                }

                if (!foundValidationLayer) {
                    for (uint32_t i = 0; i < layerCount; i++)
                    {
                        if (strcmp(queriedLayers[i].layerName, "VK_LAYER_LUNARG_standard_validation") == 0)
                        {
                            enabledInstanceLayers.Push("VK_LAYER_LUNARG_standard_validation");
                            foundValidationLayer = true;
                            break;
                        }
                    }
                }
            }
#endif
            uint32_t apiVersion = volkGetInstanceVersion();
            if (apiVersion < VK_API_VERSION_1_1) {
                apiVersion = VK_API_VERSION_1_1;
            }

            // Create the Vulkan instance
            VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
            appInfo.pApplicationName = "Alimer";
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = "Alimer";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = apiVersion;

            VkInstanceCreateInfo instanceInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
            instanceInfo.pApplicationInfo = &appInfo;
            instanceInfo.enabledLayerCount = enabledInstanceLayers.Size();
            instanceInfo.ppEnabledLayerNames = enabledInstanceLayers.Data();
            instanceInfo.enabledExtensionCount = enabledInstanceExtensions.Size();
            instanceInfo.ppEnabledExtensionNames = enabledInstanceExtensions.Data();

#if defined(VULKAN_DEBUG) || defined(VULKAN_VALIDATION_LAYERS)
            VkDebugUtilsMessengerCreateInfoEXT debug_utils_create_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
            if (features.debugUtils)
            {
                debug_utils_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
                debug_utils_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
                debug_utils_create_info.pfnUserCallback = DebugUtilsMessengerCallback;

                instanceInfo.pNext = &debug_utils_create_info;
            }
#endif

            // Create the Vulkan instance
            VkResult result = vkCreateInstance(&instanceInfo, nullptr, &instance);

            if (result != VK_SUCCESS)
            {
                LOG_VK_ERROR_MSG(result, "Could not create Vulkan instance");
            }

            volkLoadInstance(instance);

#if defined(VULKAN_DEBUG) || defined(VULKAN_VALIDATION_LAYERS)
            if (features.debugUtils)
            {
                result = vkCreateDebugUtilsMessengerEXT(instance, &debug_utils_create_info, nullptr, &debugUtilsMessenger);
                if (result != VK_SUCCESS)
                {
                    LOG_VK_ERROR_MSG(result, "Could not create debug utils messenger");
                }
            }
#endif
        }
    }

    VulkanGraphicsDevice::~VulkanGraphicsDevice()
    {

#if defined(VULKAN_DEBUG) || defined(VULKAN_VALIDATION_LAYERS)
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

    void VulkanGraphicsDevice::InitCapabilities()
    {
        caps.backendType = BackendType::Vulkan;
    }

    GraphicsContext* VulkanGraphicsDevice::CreateContext(const GraphicsContextDescription& desc)
    {
        return nullptr;
    }

    Texture* VulkanGraphicsDevice::CreateTexture(const TextureDescription& desc, const void* initialData)
    {
        return nullptr;
    }

    GraphicsDevice* VulkanGraphicsDeviceFactory::CreateDevice(bool validation)
    {
        return new VulkanGraphicsDevice(validation);
    }
}

