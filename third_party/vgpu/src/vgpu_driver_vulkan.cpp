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

#if defined(VGPU_DRIVER_VULKAN)

#include "vgpu_driver.h"

#include <volk.h>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

static const char* _vgpu_vk_get_error_string(VkResult result) {
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

#define VGPU_VK_THROW(str) if (vk.debug) { vgpu::logError("%", str); }
#define VGPU_VK_CHECK(c, str) if (!(c)) { VGPU_VK_THROW(str); }
#define VK_CHECK(res) do { VkResult r = (res); VGPU_VK_CHECK(r >= 0, _vgpu_vk_get_error_string(r)); } while (0)

namespace vgpu
{
    struct BufferVk {
        enum { MAX_COUNT = 4096 };

        VkBuffer handle;
        VmaAllocation allocation;
    };

    struct TextureVk {
        enum { MAX_COUNT = 4096 };

        VkImage handle;
        VmaAllocation allocation;
    };

    /* Global data */
    static struct {
        bool available_initialized;
        bool available;

        bool debug;

        struct {
            bool debug_utils;
            bool headless;
            bool surface_capabilities2;
            bool win32_full_screen_exclusive;
        } instance_exts;

        Caps caps;

        VkInstance instance;
        VkDebugUtilsMessengerEXT messenger;
        VkSurfaceKHR surface;

        Pool<BufferVk, BufferVk::MAX_COUNT> buffers;
        Pool<TextureVk, TextureVk::MAX_COUNT> textures;
    } vk;

    static VkBool32 _vgpu_vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT flags, const VkDebugUtilsMessengerCallbackDataEXT* data, void* context) {
        if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            vgpu::log(LogLevel::Error, "%s", data->pMessage);
        }
        else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            vgpu::log(LogLevel::Warn, "%s", data->pMessage);
        }
        else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
            vgpu::log(LogLevel::Info, "%s", data->pMessage);
        }

        return VK_FALSE;
    }

    static bool vulkan_init(InitFlags flags, const PresentationParameters& presentationParameters)
    {
        if (any(flags & InitFlags::DebugRutime) || any(flags & InitFlags::GPUBasedValidation))
        {
            vk.debug = true;
        }

        vk.buffers.init();
        vk.textures.init();

        VkResult result = VK_SUCCESS;

        // Create instance and optional debug messenger.
        {
            uint32_t instance_extension_count = 0;
            VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr));

            std::vector<VkExtensionProperties> available_instance_extensions(instance_extension_count);
            VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, available_instance_extensions.data()));

            std::vector<const char*> enabled_instance_exts;
            std::vector<const char*> enabledLayers;

            for (uint32_t i = 0; i < instance_extension_count; ++i)
            {
                if (strcmp(available_instance_extensions[i].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
                    vk.instance_exts.debug_utils = true;
                    if (vk.debug) {
                        enabled_instance_exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                    }
                }
                else if (strcmp(available_instance_extensions[i].extensionName, VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME) == 0)
                {
                    vk.instance_exts.headless = true;
                }
                else if (strcmp(available_instance_extensions[i].extensionName, VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME) == 0)
                {
                    vk.instance_exts.surface_capabilities2 = true;
                }
            }

            if (vk.debug)
            {
                uint32_t layerCount;
                VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

                std::vector<VkLayerProperties> supportedLayers(layerCount);
                VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, supportedLayers.data()));

                bool foundLayer = false;
                for (const VkLayerProperties& layer : supportedLayers)
                {
                    if (strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
                        enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
                        foundLayer = true;
                        break;
                    }
                }

                if (!foundLayer) {
                    for (const VkLayerProperties& layer : supportedLayers)
                    {
                        if (strcmp(layer.layerName, "VK_LAYER_LUNARG_standard_validation") == 0) {
                            enabledLayers.push_back("VK_LAYER_LUNARG_standard_validation");
                            foundLayer = true;
                            break;
                        }
                    }
                }
            }

            VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
            appInfo.pApplicationName = "Alimer Engine Application";
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = "Alimer Engine";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = volkGetInstanceVersion();

            VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
            VkDebugUtilsMessengerCreateInfoEXT debug_utils_create_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };

            if (vk.debug && vk.instance_exts.debug_utils)
            {
                debug_utils_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
                debug_utils_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
                debug_utils_create_info.pfnUserCallback = _vgpu_vk_debug_callback;
                debug_utils_create_info.pUserData = nullptr;

                createInfo.pNext = &debug_utils_create_info;
            }

            createInfo.pApplicationInfo = &appInfo;
            createInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
            createInfo.ppEnabledLayerNames = enabledLayers.data();
            createInfo.enabledExtensionCount = static_cast<uint32_t>(enabled_instance_exts.size());
            createInfo.ppEnabledExtensionNames = enabled_instance_exts.data();

            // Create the Vulkan instance.
            result = vkCreateInstance(&createInfo, nullptr, &vk.instance);

            if (result != VK_SUCCESS)
            {
                //VK_LOG_ERROR(result, "Could not create Vulkan instance");
                vgpu::shutdown();
                return false;
            }

            volkLoadInstance(vk.instance);
        }

        // Create surface if not running headless
        if (presentationParameters.windowHandle)
        {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
            VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
            surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            surfaceCreateInfo.hinstance = GetModuleHandle(NULL);
            surfaceCreateInfo.hwnd = (HWND)presentationParameters.windowHandle;
            result = vkCreateWin32SurfaceKHR(vk.instance, &surfaceCreateInfo, nullptr, &vk.surface);
#else
#endif

            if (result != VK_SUCCESS)
            {
                vgpu::shutdown();
                return false;
            }
        }

        return true;
    }

    static void vulkan_shutdown(void) {
        if (vk.messenger) vkDestroyDebugUtilsMessengerEXT(vk.instance, vk.messenger, nullptr);
        if (vk.instance) vkDestroyInstance(vk.instance, nullptr);
        memset(&vk, 0, sizeof(vk));
    }

    static void vulkan_beginFrame(void) {

    }

    static void vulkan_endFrame(void) {

    }

    static const Caps* vulkan_queryCaps(void) {
        return &vk.caps;
    }

    static bool vulkan_isSupported(void) {
        if (vk.available_initialized) {
            return vk.available;
        }

        vk.available_initialized = true;

        VkResult result = volkInitialize();
        if (result != VK_SUCCESS) {
            return false;
        }

        // We reuire vulkan 1.1.0 or higher API
        if (volkGetInstanceVersion() < VK_API_VERSION_1_1) {
            return false;
        }

        VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        appInfo.apiVersion = volkGetInstanceVersion();

        VkInstanceCreateInfo instance_info = {};
        instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_info.pApplicationInfo = &appInfo;

        VkInstance instance;
        result = vkCreateInstance(&instance_info, nullptr, &instance);
        if (result != VK_SUCCESS) {
            return false;
        }

        vkDestroyInstance = (PFN_vkDestroyInstance)vkGetInstanceProcAddr(instance, "vkDestroyInstance");
        vkDestroyInstance(instance, nullptr);
        vk.available = true;
        return true;
    };

    static Renderer* vulkan_initRenderer(void) {
        static Renderer renderer = { nullptr };
        renderer.init = vulkan_init;
        renderer.shutdown = vulkan_shutdown;
        renderer.beginFrame = vulkan_beginFrame;
        renderer.endFrame = vulkan_endFrame;
        renderer.queryCaps = vulkan_queryCaps;

        //renderer.create_texture = vulkan_texture_create;
        //renderer.texture_destroy = vulkan_texture_destroy;

        return &renderer;
    }

    Driver vulkan_driver = {
        BackendType::Vulkan,
        vulkan_isSupported,
        vulkan_initRenderer
    };
}

#endif /* defined(VGPU_DRIVER_VULKAN) */
