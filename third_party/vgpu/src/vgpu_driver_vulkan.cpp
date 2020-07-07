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
#include <string.h>
#include <stdlib.h>

#define VK_NO_PROTOTYPES

#if defined(_WIN32)
#   define VK_SURFACE_EXT               "VK_KHR_win32_surface"
#   define VK_CREATE_SURFACE_FN         vkCreateWin32SurfaceKHR
#   define VK_CREATE_SURFACE_FN_TYPE    PFN_vkCreateWin32SurfaceKHR
#   define VK_USE_PLATFORM_WIN32_KHR
#   ifndef NOMINMAX
#       define NOMINMAX
#endif
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#elif defined(__ANDROID__)
#   include <dlfcn.h>
#   define VK_SURFACE_EXT               "VK_KHR_android_surface"
#   define VK_CREATE_SURFACE_FN         vkCreateAndroidSurfaceKHR
#   define VK_CREATE_SURFACE_FN_TYPE    PFN_vkCreateAndroidSurfaceKHR
#   define VK_USE_PLATFORM_ANDROID_KHR
#else
#   include <dlfcn.h>
#   include <X11/Xlib-xcb.h>
#   define VK_SURFACE_EXT               "VK_KHR_xcb_surface"
#   define VK_CREATE_SURFACE_FN         vkCreateXcbSurfaceKHR
#   define VK_CREATE_SURFACE_FN_TYPE    PFN_vkCreateXcbSurfaceKHR
#   define VK_USE_PLATFORM_XCB_KHR
#endif

#include <vulkan/vulkan.h>

/* Needed by vk_mem_alloc */
PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = nullptr;

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

// Functions that don't require an instance
#define VGPU_FOREACH_ANONYMOUS(X)\
X(vkCreateInstance)\
X(vkEnumerateInstanceExtensionProperties)\
X(vkEnumerateInstanceLayerProperties)\
X(vkEnumerateInstanceVersion)

// Functions that require an instance but don't require a device
#define VGPU_FOREACH_INSTANCE(X)\
X(vkCreateDevice)\
X(vkDestroyInstance)\
X(vkEnumerateDeviceExtensionProperties)\
X(vkCreateDebugUtilsMessengerEXT)\
X(vkDestroyDebugUtilsMessengerEXT)\
X(vkCreateDebugReportCallbackEXT)\
X(vkDestroyDebugReportCallbackEXT)\
X(vkEnumeratePhysicalDevices)\
X(vkGetPhysicalDeviceProperties)\
X(vkGetPhysicalDeviceFeatures)\
X(vkGetPhysicalDeviceMemoryProperties)\
X(vkGetPhysicalDeviceQueueFamilyProperties)\
X(vkGetPhysicalDeviceFormatProperties)\
X(vkDestroyDevice)\
X(vkGetDeviceQueue)\
X(vkDestroySurfaceKHR)\
X(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)\
X(vkGetPhysicalDeviceSurfaceFormatsKHR)\
X(vkGetPhysicalDeviceSurfacePresentModesKHR)\
X(vkGetPhysicalDeviceSurfaceSupportKHR)\
X(vkGetPhysicalDeviceFeatures2)

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
#define VGPU_FOREACH_INSTANCE_SURFACE(X)\
X(vkCreateAndroidSurfaceKHR)
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
#define VGPU_FOREACH_INSTANCE_SURFACE(X)\
X(vkCreateWin32SurfaceKHR)\
X(vkGetPhysicalDeviceWin32PresentationSupportKHR)
#elif defined(VK_USE_PLATFORM_METAL_EXT)
#define VGPU_FOREACH_INSTANCE_SURFACE(X)\
X(vkCreateMetalSurfaceEXT)
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
#define VGPU_FOREACH_INSTANCE_SURFACE(X)\
X(vkCreateMacOSSurfaceMVK)
#elif defined(VK_USE_PLATFORM_XCB_KHR)
#define VGPU_FOREACH_INSTANCE_SURFACE(X)\
X(vkCreateXcbSurfaceKHR)\
X(vkGetPhysicalDeviceXcbPresentationSupportKHR)
#endif

// Functions that require a device
#define GPU_FOREACH_DEVICE(X)\
X(vkSetDebugUtilsObjectNameEXT)\
X(vkQueueSubmit)\
X(vkDeviceWaitIdle)\
X(vkCreateCommandPool)\
X(vkDestroyCommandPool)\
X(vkResetCommandPool)\
X(vkAllocateCommandBuffers)\
X(vkFreeCommandBuffers)\
X(vkBeginCommandBuffer)\
X(vkEndCommandBuffer)\
X(vkCreateFence)\
X(vkDestroyFence)\
X(vkWaitForFences)\
X(vkResetFences)\
X(vkCreateSemaphore)\
X(vkDestroySemaphore)\
X(vkCmdPipelineBarrier)\
X(vkCreateBuffer)\
X(vkDestroyBuffer)\
X(vkGetBufferMemoryRequirements)\
X(vkBindBufferMemory)\
X(vkCmdCopyBuffer)\
X(vkCreateImage)\
X(vkDestroyImage)\
X(vkGetImageMemoryRequirements)\
X(vkBindImageMemory)\
X(vkCmdCopyBufferToImage)\
X(vkAllocateMemory)\
X(vkFreeMemory)\
X(vkMapMemory)\
X(vkUnmapMemory)\
X(vkCreateSampler)\
X(vkDestroySampler)\
X(vkCreateRenderPass)\
X(vkDestroyRenderPass)\
X(vkCmdBeginRenderPass)\
X(vkCmdEndRenderPass)\
X(vkCreateImageView)\
X(vkDestroyImageView)\
X(vkCreateFramebuffer)\
X(vkDestroyFramebuffer)\
X(vkCreateShaderModule)\
X(vkDestroyShaderModule)\
X(vkCreateGraphicsPipelines)\
X(vkDestroyPipeline)\
X(vkCmdBindPipeline)\
X(vkCmdBindVertexBuffers)\
X(vkCmdBindIndexBuffer)\
X(vkCmdDraw)\
X(vkCmdDrawIndexed)\
X(vkCmdDrawIndirect)\
X(vkCmdDrawIndexedIndirect)\
X(vkCmdDispatch)

#define VGPU_VK_DECLARE(fn) static PFN_##fn fn;
#define GPU_LOAD_ANONYMOUS(fn) fn = (PFN_##fn) vkGetInstanceProcAddr(NULL, #fn);
#define GPU_LOAD_INSTANCE(fn) fn = (PFN_##fn) vkGetInstanceProcAddr(vk.instance, #fn);
#define GPU_LOAD_DEVICE(fn) fn = (PFN_##fn) vkGetDeviceProcAddr(vk.device, #fn);

/* Declare function pointers */
VGPU_FOREACH_ANONYMOUS(VGPU_VK_DECLARE);
VGPU_FOREACH_INSTANCE(VGPU_VK_DECLARE);
VGPU_FOREACH_INSTANCE_SURFACE(VGPU_VK_DECLARE);
GPU_FOREACH_DEVICE(VGPU_VK_DECLARE);

#define VGPU_VK_THROW(str) if (vk.debug) { vgpu_log_error("%", str); }
#define VGPU_VK_CHECK(c, str) if (!(c)) { VGPU_VK_THROW(str); }
#define VK_CHECK(res) do { VkResult r = (res); VGPU_VK_CHECK(r >= 0, _vgpu_vk_get_error_string(r)); } while (0)

struct vk_texture {
    VkImage handle;
};

/* Global data */
static struct {
    bool available_initialized;
    bool available;

#if defined(_WIN32)
    HMODULE library;
#else
    void* library;
#endif

    bool debug;

    struct {
        bool debug_utils;
        bool headless;
        bool surface_capabilities2;
        bool get_physical_device_properties2;
        bool external_memory_capabilities;
        bool external_semaphore_capabilities;
        bool win32_full_screen_exclusive;
    } instance_exts;

    VkInstance instance;
    VkDebugUtilsMessengerEXT messenger;
} vk;

/* Helper */
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

static uint32_t _vgpu_vulkan_get_instance_version(void)
{
#if defined(VK_VERSION_1_1)
    uint32_t apiVersion = 0;
    if (vkEnumerateInstanceVersion && vkEnumerateInstanceVersion(&apiVersion) == VK_SUCCESS)
        return apiVersion;
#endif

    return VK_API_VERSION_1_0;
}

static VkBool32 _vgpu_vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT flags, const VkDebugUtilsMessengerCallbackDataEXT* data, void* context) {
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        vgpu_log(VGPU_LOG_LEVEL_ERROR, "%s", data->pMessage);
    }
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        vgpu_log(VGPU_LOG_LEVEL_WARN, "%s", data->pMessage);
    }
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        vgpu_log(VGPU_LOG_LEVEL_INFO, "%s", data->pMessage);
    }

    return VK_FALSE;
}

static bool vulkan_init(const vgpu_init_info* info) {
    vk.debug = info->debug;

    // Create instance and optional debug messenger.
    {
        uint32_t instance_extension_count = 0;
        VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, NULL));

        VkExtensionProperties* available_instance_extensions = VGPU_ALLOCA(VkExtensionProperties, instance_extension_count);
        VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, available_instance_extensions));

        uint32_t enabled_instance_exts_count = 0;
        uint32_t enabled_instance_layers_count = 0;
        const char* enabled_instance_exts[16];
        const char* enabled_instance_layers[6];

        for (uint32_t i = 0; i < instance_extension_count; ++i)
        {
            if (strcmp(available_instance_extensions[i].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
                vk.instance_exts.debug_utils = true;
                if (info->debug) {
                    enabled_instance_exts[enabled_instance_exts_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
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
            else if (strcmp(available_instance_extensions[i].extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0)
            {
                vk.instance_exts.get_physical_device_properties2 = true;
                enabled_instance_exts[enabled_instance_exts_count++] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
            }
            else if (strcmp(available_instance_extensions[i].extensionName, VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME) == 0)
            {
                vk.instance_exts.external_memory_capabilities = true;
                enabled_instance_exts[enabled_instance_exts_count++] = VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME;
            }
            else if (strcmp(available_instance_extensions[i].extensionName, VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME) == 0)
            {
                vk.instance_exts.external_semaphore_capabilities = true;
                enabled_instance_exts[enabled_instance_exts_count++] = VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME;
            }
        }

        VkApplicationInfo app_info{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
        app_info.apiVersion = _vgpu_vulkan_get_instance_version();

        VkInstanceCreateInfo instance_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        VkDebugUtilsMessengerCreateInfoEXT debug_utils_create_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };

        if (info->debug)
        {
            //if (instanceExts.debugUtils)
            {
                debug_utils_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
                debug_utils_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
                debug_utils_create_info.pfnUserCallback = _vgpu_vk_debug_callback;
                debug_utils_create_info.pUserData = nullptr;

                instance_info.pNext = &debug_utils_create_info;
            }
        }

        instance_info.pApplicationInfo = &app_info;
        instance_info.enabledLayerCount = enabled_instance_layers_count;
        instance_info.ppEnabledLayerNames = enabled_instance_layers;
        instance_info.enabledExtensionCount = enabled_instance_exts_count;
        instance_info.ppEnabledExtensionNames = enabled_instance_exts;

        // Create the Vulkan instance.
        VkResult result = vkCreateInstance(&instance_info, nullptr, &vk.instance);

        if (result != VK_SUCCESS)
        {
            //VK_LOG_ERROR(result, "Could not create Vulkan instance");
            vgpu_shutdown();
            return false;
        }

        VGPU_FOREACH_INSTANCE(GPU_LOAD_INSTANCE);
        VGPU_FOREACH_INSTANCE_SURFACE(GPU_LOAD_INSTANCE);
    }

    return true;
}

static void vulkan_shutdown(void) {
    if (vk.messenger) vkDestroyDebugUtilsMessengerEXT(vk.instance, vk.messenger, NULL);
    if (vk.instance) vkDestroyInstance(vk.instance, NULL);
    memset(&vk, 0, sizeof(vk));
}

static void vulkan_begin_frame(void) {
}

static void vulkan_end_frame(void) {
}

static vgpu_texture vulkan_texture_create(const vgpu_texture_info* info) {
    vk_texture* texture = VGPU_ALLOC(vk_texture);
    if (info->external_handle) {
        texture->handle = (VkImage)info->external_handle;
    }
    else {

    }

    return (vgpu_texture)texture;
}

static void vulkan_texture_destroy(vgpu_texture handle) {
    vk_texture* texture = (vk_texture*)handle;
    //vk_release_resource(texture->handle);
    VGPU_FREE(texture);
}

/* Driver */
static bool vulkan_is_supported(void) {
    if (vk.available_initialized) {
        return vk.available;
    }

    vk.available_initialized = true;

#if defined(_WIN32)
    vk.library = LoadLibraryA("vulkan-1.dll");
    if (!vk.library) {
        return false;
    }

    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(vk.library, "vkGetInstanceProcAddr");
#elif defined(__APPLE__)
    vk.library = dlopen("libvulkan.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!vk.library)
        vk.library = dlopen("libvulkan.1.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!vk.library)
        vk.library = dlopen("libMoltenVK.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!vk.library)
        return false;

    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(vk.library, "vkGetInstanceProcAddr");
#else
    vk.library = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!vk.library)
        vk.library = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    if (!vk.library)
        return false;

    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(vk.library, "vkGetInstanceProcAddr");
#endif

    VGPU_FOREACH_ANONYMOUS(GPU_LOAD_ANONYMOUS);

    // We reuire vulkan 1.1.0 or higher API
    VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app_info.apiVersion = _vgpu_vulkan_get_instance_version();

    VkInstanceCreateInfo instance_info = {};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;

    VkInstance instance;
    VkResult result = vkCreateInstance(&instance_info, nullptr, &instance);
    if (result != VK_SUCCESS) {
        return false;
    }

    vkDestroyInstance = (PFN_vkDestroyInstance)vkGetInstanceProcAddr(instance, "vkDestroyInstance");
    vkDestroyInstance(instance, nullptr);
    vk.available = true;
    return true;
};

static vgpu_renderer* vulkan_init_renderer(void) {
    static vgpu_renderer renderer = { nullptr };
    renderer.init = vulkan_init;
    renderer.shutdown = vulkan_shutdown;
    renderer.begin_frame = vulkan_begin_frame;
    renderer.end_frame = vulkan_end_frame;

    renderer.create_texture = vulkan_texture_create;
    renderer.texture_destroy = vulkan_texture_destroy;

    return &renderer;
}

vgpu_driver vulkan_driver = {
    VGPU_BACKEND_TYPE_VULKAN,
    vulkan_is_supported,
    vulkan_init_renderer
};

#endif /* defined(VGPU_DRIVER_VULKAN) */
