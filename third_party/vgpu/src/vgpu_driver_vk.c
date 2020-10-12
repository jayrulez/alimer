//
// Copyright (c) 2019-2020 Amer Koleci.
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
#include <inttypes.h>

#if _WIN32
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#else
#   include <dlfcn.h>
#endif

#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

/* Needed by vma */
PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;

// Functions that don't require an instance
#define VGPU_FOREACH_ANONYMOUS(X)\
    X(vkCreateInstance)\
    X(vkEnumerateInstanceExtensionProperties)\
    X(vkEnumerateInstanceLayerProperties)\
    X(vkEnumerateInstanceVersion)

// Functions that require an instance but don't require a device
#define VGPU_FOREACH_INSTANCE(X)\
    X(vkDestroyInstance)\
    X(vkEnumerateDeviceExtensionProperties)\
    X(vkCreateDebugUtilsMessengerEXT)\
    X(vkDestroyDebugUtilsMessengerEXT)\
    X(vkEnumeratePhysicalDevices)\
    X(vkGetPhysicalDeviceProperties)\
    X(vkGetPhysicalDeviceFeatures)\
    X(vkGetPhysicalDeviceMemoryProperties)\
    X(vkGetPhysicalDeviceQueueFamilyProperties)\
    X(vkCreateDevice)\
    X(vkDestroyDevice)\
    X(vkGetDeviceQueue)\
    X(vkDestroySurfaceKHR)\
    X(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)\
    X(vkGetPhysicalDeviceSurfaceFormatsKHR)\
    X(vkGetPhysicalDeviceSurfacePresentModesKHR)\
    X(vkGetPhysicalDeviceSurfaceSupportKHR)\
    X(vkGetPhysicalDeviceProperties2)

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
#define VGPU_FOREACH_INSTANCE_SURFACE(X)\
    X(vkCreateWin32SurfaceKHR)
#endif

// Functions that require a device
#define VGPU_FOREACH_DEVICE(X)\
    X(vkSetDebugUtilsObjectNameEXT)\
    X(vkQueueSubmit)\
    X(vkDeviceWaitIdle)\
    X(vkCreateCommandPool)\
    X(vkDestroyCommandPool)\
    X(vkAllocateCommandBuffers)\
    X(vkFreeCommandBuffers)\
    X(vkBeginCommandBuffer)\
    X(vkEndCommandBuffer)\
    X(vkCreateFence)\
    X(vkDestroyFence)\
    X(vkWaitForFences)\
    X(vkResetFences)\
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

#define VGPU_DECLARE(fn) static PFN_##fn fn;
#define VGPU_LOAD_ANONYMOUS(fn) fn = (PFN_##fn) vkGetInstanceProcAddr(NULL, #fn);
#define VGPU_LOAD_INSTANCE(fn) fn = (PFN_##fn) vkGetInstanceProcAddr(vk.instance, #fn);
#define VGPU_LOAD_DEVICE(fn) fn = (PFN_##fn) vkGetDeviceProcAddr(vk.device, #fn);

// Declare function pointers
VGPU_FOREACH_ANONYMOUS(VGPU_DECLARE);
VGPU_FOREACH_INSTANCE(VGPU_DECLARE);
VGPU_FOREACH_INSTANCE_SURFACE(VGPU_DECLARE);
VGPU_FOREACH_DEVICE(VGPU_DECLARE);

#define VOIDP_TO_U64(x) (((union { uint64_t u; void* p; }) { .p = x }).u)
#define VGPU_VK_LOG_ERROR(result, message) vgpu_log(VGPU_LOG_LEVEL_ERROR, "%s - Vulkan error: %s", _vgpu_vk_get_error_string(result));
#define VGPU_VK_CHECK(f) do { VkResult r = (f); VGPU_CHECK(r >= 0, _vgpu_vk_get_error_string(r)); } while (0)

typedef struct vk_queue_family_indices {
    uint32_t graphics;
    uint32_t compute;
    uint32_t copy;
} vk_queue_family_indices;

typedef struct vk_physical_device_extensions {
    bool depth_clip_enable;
    bool maintenance_1;
    bool maintenance_2;
    bool maintenance_3;
    bool get_memory_requirements2;
    bool dedicated_allocation;
    bool bind_memory2;
    bool memory_budget;
    bool image_format_list;
    bool sampler_mirror_clamp_to_edge;
    bool win32_full_screen_exclusive;
    bool raytracing;
    bool buffer_device_address;
    bool deferred_host_operations;
    bool descriptor_indexing;
    bool pipeline_library;
    bool multiview;
} vk_physical_device_extensions;

typedef struct {
    VkBuffer handle;
} vk_buffer;

typedef struct {
    VkImage handle;
} vk_texture;

typedef struct {
    VkBuffer handle;
} vk_shader;

typedef struct {
    VkPipeline handle;
} vk_pipeline;

/* Constants */
enum {
    _VGPU_VK_INVALID_INDEX = ~0u,
    _VGPU_VK_MAX_PHYSICAL_DEVICES = 16,
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

    vgpu_config config;
    vgpu_caps caps;

    bool debug_utils;
    bool headless_extension;
    bool get_physical_device_properties2;
    bool get_surface_capabilities2;

    VkInstance instance;
    VkDebugUtilsMessengerEXT messenger;

    VkSurfaceKHR surface;

    VkPhysicalDevice physical_device;
    vk_queue_family_indices queue_families;

    VkDevice device;
    VkQueue graphics_queue;
    VkQueue compute_queue;
    VkQueue copy_queue;

    VmaAllocator memory_allocator;
} vk;

static VkBool32 _vgpu_vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT flags, const VkDebugUtilsMessengerCallbackDataEXT* data, void* context);
static const char* _vgpu_vk_get_error_string(VkResult result);
static vk_queue_family_indices _vgpu_vk_query_queue_families(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
static vk_physical_device_extensions _vgpu_vk_query_physical_device_extensions(VkPhysicalDevice physical_device);
static bool _vgpu_vk_is_device_suitable(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

static VkSurfaceKHR _vgpu_vk_create_surface(void* window_handle)
{
    VkResult result = VK_SUCCESS;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
    const VkWin32SurfaceCreateInfoKHR surface_info = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .hinstance = GetModuleHandle(NULL),
        .hwnd = (HWND)window_handle
    };

    result = vkCreateWin32SurfaceKHR(vk.instance, &surface_info, NULL, &surface);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
#elif defined(VK_USE_PLATFORM_XCB_KHR)
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
#else
#	pragma error Platform not supported
#endif
    if (result != VK_SUCCESS)
    {
        VGPU_VK_LOG_ERROR(result, "Vulkan: Failed to create surface");
        return VK_NULL_HANDLE;
    }

    return surface;
}

static bool vk_init(const char* app_name, const vgpu_config* config)
{
    VkResult result = VK_SUCCESS;
    memcpy(&vk.config, config, sizeof(vk.config));

    /* Instance */
    {
        const char* enabled_layer_names[32];
        uint32_t enabled_layer_count = 0;
        const char* enabled_extension_names[32];
        uint32_t enabled_extension_count = 0;

        if (config->debug)
        {

            VkLayerProperties* supported_layers = NULL;
            uint32_t supported_layer_count;
            result = vkEnumerateInstanceLayerProperties(&supported_layer_count, NULL);
            if (result != VK_SUCCESS) {
                vgpu_log(VGPU_LOG_LEVEL_ERROR, "Failed to query extension count.");
                return false;
            }

            supported_layers = VGPU_ALLOCA(VkLayerProperties, supported_layer_count);
            result = vkEnumerateInstanceLayerProperties(&supported_layer_count, supported_layers);
            if (result != VK_SUCCESS) {
                vgpu_log(VGPU_LOG_LEVEL_ERROR, "Failed to retrieve extensions.");
                return false;
            }

            // Search VK_LAYER_KHRONOS_validation first.
            bool found_layer = false;
            for (uint32_t i = 0; i < supported_layer_count; ++i)
            {
                if (strcmp(supported_layers[i].layerName, "VK_LAYER_KHRONOS_validation") == 0)
                {
                    enabled_layer_names[enabled_layer_count++] = "VK_LAYER_KHRONOS_validation";
                    found_layer = true;
                    break;
                }
            }

            if (!found_layer)
            {
                // Search VK_LAYER_LUNARG_standard_validation otherwise.
                for (uint32_t i = 0; i < supported_layer_count; ++i)
                {
                    if (strcmp(supported_layers[i].layerName, "VK_LAYER_LUNARG_standard_validation") == 0)
                    {
                        enabled_layer_names[enabled_layer_count++] = "VK_LAYER_LUNARG_standard_validation";
                        found_layer = true;
                        break;
                    }
                }
            }
        }

        VkExtensionProperties* supported_instance_extensions = NULL;
        uint32_t supported_instance_extension_count;
        result = vkEnumerateInstanceExtensionProperties(NULL, &supported_instance_extension_count, NULL);
        if (result != VK_SUCCESS) {
            vgpu_log(VGPU_LOG_LEVEL_ERROR, "Failed to query extension count.");
            return false;
        }

        supported_instance_extensions = VGPU_ALLOCA(VkExtensionProperties, supported_instance_extension_count);
        result = vkEnumerateInstanceExtensionProperties(NULL, &supported_instance_extension_count, supported_instance_extensions);
        if (result != VK_SUCCESS) {
            vgpu_log(VGPU_LOG_LEVEL_ERROR, "Failed to retrieve extensions.");
            return false;
        }

        for (uint32_t i = 0; i < supported_instance_extension_count; ++i)
        {
            if (strcmp(supported_instance_extensions[i].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
            {
                vk.debug_utils = true;
                if (config->debug) {
                    enabled_extension_names[enabled_extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
                }
            }
            else if (strcmp(supported_instance_extensions[i].extensionName, VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME) == 0)
            {
                vk.headless_extension = true;
            }
            else if (strcmp(supported_instance_extensions[i].extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0)
            {
                // VK_KHR_get_physical_device_properties2 is a prerequisite of VK_KHR_performance_query
                // which will be used for stats gathering where available.
                vk.get_physical_device_properties2 = true;
                enabled_extension_names[enabled_extension_count++] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
            }
            else if (strcmp(supported_instance_extensions[i].extensionName, VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME) == 0)
            {
                vk.get_surface_capabilities2 = true;
            }
        }

        const bool headless = false;
        if (headless)
        {
            if (vk.headless_extension)
            {
                enabled_extension_names[enabled_extension_count++] = VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME;
            }
            else
            {
                vgpu_log(VGPU_LOG_LEVEL_WARN, "'%s' is not available, disabling swapchain creation", VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
            }
        }
        else
        {
            enabled_extension_names[enabled_extension_count++] = VK_KHR_SURFACE_EXTENSION_NAME;

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
            enabled_extension_names[enabled_extension_count++] = VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
            enabled_extension_names[enabled_extension_count++] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
            enabled_extension_names[enabled_extension_count++] = VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
            enabled_extension_names[enabled_extension_count++] = VK_KHR_XCB_SURFACE_EXTENSION_NAME;
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)
            enabled_extension_names[enabled_extension_count++] = VK_KHR_DISPLAY_EXTENSION_NAME;
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
            enabled_extension_names[enabled_extension_count++] = VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
#else
#	pragma error Platform not supported
#endif

            if (vk.get_surface_capabilities2)
            {
                enabled_extension_names[enabled_extension_count++] = VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME;
            }
        }

        VkInstanceCreateInfo instance_info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &(VkApplicationInfo) {
                .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .pApplicationName = app_name,
                .pEngineName = "vgpu",
                .apiVersion = VK_API_VERSION_1_1,
            },
            .enabledLayerCount = enabled_layer_count,
            .ppEnabledLayerNames = enabled_layer_names,
            .enabledExtensionCount = enabled_extension_count,
            .ppEnabledExtensionNames = enabled_extension_names
        };

        const VkDebugUtilsMessengerCreateInfoEXT messenger_info = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = _vgpu_vk_debug_callback
        };

        if (config->debug) {
            instance_info.pNext = &messenger_info;
        }

        if (vkCreateInstance(&instance_info, NULL, &vk.instance) != VK_SUCCESS) {
            return false;
        }

        VGPU_FOREACH_INSTANCE(VGPU_LOAD_INSTANCE);
        VGPU_FOREACH_INSTANCE_SURFACE(VGPU_LOAD_INSTANCE);
        vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(vk.instance, "vkGetDeviceProcAddr");

        if (config->debug) {
            if (vkCreateDebugUtilsMessengerEXT(vk.instance, &messenger_info, NULL, &vk.messenger)) {
                vgpu_shutdown();
                return false;
            }
        }

        vgpu_log(VGPU_LOG_LEVEL_INFO, "Created VkInstance with version: %u.%u.%u",
            VK_VERSION_MAJOR(instance_info.pApplicationInfo->apiVersion),
            VK_VERSION_MINOR(instance_info.pApplicationInfo->apiVersion),
            VK_VERSION_PATCH(instance_info.pApplicationInfo->apiVersion));

        if (instance_info.enabledLayerCount > 0)
        {
            for (uint32_t i = 0; i < instance_info.enabledLayerCount; ++i)
                vgpu_log(VGPU_LOG_LEVEL_INFO, "Instance layer '%s'", instance_info.ppEnabledLayerNames[i]);
        }

        for (uint32_t i = 0; i < instance_info.enabledExtensionCount; ++i)
        {
            vgpu_log(VGPU_LOG_LEVEL_INFO, "Instance extension '%s'", instance_info.ppEnabledExtensionNames[i]);
        }
    }

    /* Surface*/
    vk.surface = _vgpu_vk_create_surface(config->swapchain_info.window_handle);

    // Device
    {
        uint32_t physical_device_count = _VGPU_VK_MAX_PHYSICAL_DEVICES;
        VkPhysicalDevice physical_devices[_VGPU_VK_MAX_PHYSICAL_DEVICES];
        if (vkEnumeratePhysicalDevices(vk.instance, &physical_device_count, physical_devices) < 0) {
            vgpu_shutdown();
            return false;
        }

        // Pick a suitable physical device based on user's preference.
        uint32_t best_device_score = 0U;
        uint32_t best_device_index = _VGPU_VK_INVALID_INDEX;
        for (uint32_t i = 0; i < physical_device_count; ++i) {
            if (!_vgpu_vk_is_device_suitable(physical_devices[i], vk.surface)) {
                continue;
            }

            VkPhysicalDeviceProperties gpu_props;
            vkGetPhysicalDeviceProperties(physical_devices[i], &gpu_props);
            uint32_t score = 0U;

            if (gpu_props.apiVersion >= VK_API_VERSION_1_2) {
                score += 10000u;
            }

            switch (gpu_props.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                score += 100U;
                if (config->device_preference == VGPU_DEVICE_PREFERENCE_HIGH_PERFORMANCE) { score += 1000U; }
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                score += 90U;
                if (config->device_preference == VGPU_DEVICE_PREFERENCE_LOW_POWER) { score += 1000U; }
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                score += 80U;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                score += 70U;
                break;
            default: score += 10U;
            }
            if (score > best_device_score) {
                best_device_index = i;
                best_device_score = score;
            }
        }
        if (best_device_index == _VGPU_VK_INVALID_INDEX) {
            vgpu_log(VGPU_LOG_LEVEL_ERROR, "Failed to find a suitable physical device.");
            vgpu_shutdown();
            return false;
        }

        vk.physical_device = physical_devices[best_device_index];
        vk.queue_families = _vgpu_vk_query_queue_families(vk.physical_device, vk.surface);

        /* Setup device queue's. */
        uint32_t queue_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(vk.physical_device, &queue_count, NULL);

        VkQueueFamilyProperties* queue_families = VGPU_ALLOCA(VkQueueFamilyProperties, queue_count);
        vkGetPhysicalDeviceQueueFamilyProperties(vk.physical_device, &queue_count, queue_families);

        uint32_t universal_queue_index = 1;
        uint32_t compute_queue_index = 0;
        uint32_t copy_queue_index = 0;

        if (vk.queue_families.compute == VK_QUEUE_FAMILY_IGNORED)
        {
            vk.queue_families.compute = vk.queue_families.graphics;
            compute_queue_index = VGPU_MIN(queue_families[vk.queue_families.graphics].queueCount - 1, universal_queue_index);
            universal_queue_index++;
        }

        if (vk.queue_families.copy == VK_QUEUE_FAMILY_IGNORED)
        {
            vk.queue_families.copy = vk.queue_families.graphics;
            copy_queue_index = VGPU_MIN(queue_families[vk.queue_families.graphics].queueCount - 1, universal_queue_index);
            universal_queue_index++;
        }
        else if (vk.queue_families.copy == vk.queue_families.compute)
        {
            copy_queue_index = VGPU_MIN(queue_families[vk.queue_families.compute].queueCount - 1, 1u);
        }

        static const float graphics_queue_prio = 0.5f;
        static const float compute_queue_prio = 1.0f;
        static const float transfer_queue_prio = 1.0f;
        float prio[3] = { graphics_queue_prio, compute_queue_prio, transfer_queue_prio };

        uint32_t queue_info_count = 0;
        VkDeviceQueueCreateInfo queue_info[3] = { 0 };
        queue_info[queue_info_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[queue_info_count].queueFamilyIndex = vk.queue_families.graphics;
        queue_info[queue_info_count].queueCount = VGPU_MIN(universal_queue_index, queue_families[vk.queue_families.graphics].queueCount);
        queue_info[queue_info_count].pQueuePriorities = prio;
        queue_info_count++;

        /* Dedicated compute queue */
        if (vk.queue_families.compute != vk.queue_families.graphics)
        {
            queue_info[queue_info_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_info[queue_info_count].queueFamilyIndex = vk.queue_families.compute;
            queue_info[queue_info_count].queueCount = VGPU_MIN(vk.queue_families.copy == vk.queue_families.compute ? 2u : 1u,
                queue_families[vk.queue_families.compute].queueCount);
            queue_info[queue_info_count].pQueuePriorities = prio + 1;
            queue_info_count++;
        }

        /* Dedicated copy queue */
        if (vk.queue_families.copy != vk.queue_families.graphics
            && vk.queue_families.copy != vk.queue_families.compute)
        {
            queue_info[queue_info_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_info[queue_info_count].queueFamilyIndex = vk.queue_families.copy;
            queue_info[queue_info_count].queueCount = 1;
            queue_info[queue_info_count].pQueuePriorities = prio + 2;
            queue_info_count++;
        }


        VkDeviceCreateInfo device_info = {
          .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
          .pNext = &(VkPhysicalDeviceFeatures2) {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &(VkPhysicalDeviceMultiviewFeatures) {
              .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES,
              .multiview = VK_TRUE
            },
            .features = {
              .fullDrawIndexUint32 = VK_TRUE,
              .multiDrawIndirect = VK_TRUE,
              .shaderSampledImageArrayDynamicIndexing = VK_TRUE
            }
          },
          .queueCreateInfoCount = queue_info_count,
          .pQueueCreateInfos = queue_info
        };

        if (vkCreateDevice(vk.physical_device, &device_info, NULL, &vk.device) != VK_SUCCESS) {
            vgpu_shutdown();
            return false;
        }

        vkGetDeviceQueue(vk.device, vk.queue_families.graphics, 0, &vk.graphics_queue);
        vkGetDeviceQueue(vk.device, vk.queue_families.compute, compute_queue_index, &vk.compute_queue);
        vkGetDeviceQueue(vk.device, vk.queue_families.copy, copy_queue_index, &vk.copy_queue);

        VGPU_FOREACH_DEVICE(VGPU_LOAD_DEVICE);
    }

    // Allocator
    {
        const VmaAllocatorCreateInfo allocator_info = {
            .physicalDevice = vk.physical_device,
            .device = vk.device,
            .pVulkanFunctions = NULL,
            .instance = vk.instance
        };

        result = vmaCreateAllocator(&allocator_info, &vk.memory_allocator);

        if (result != VK_SUCCESS)
        {
            vgpu_log(VGPU_LOG_LEVEL_ERROR, "Cannot create allocator");
            vgpu_shutdown();
            return false;
        }
    }


    /* Log some info */
    vgpu_log(VGPU_LOG_LEVEL_INFO, "VGPU driver: Vulkan");
    return true;
}

static void vk_shutdown(void)
{
    if (vk.device) {
        vkDeviceWaitIdle(vk.device);
    }

    if (vk.memory_allocator != VK_NULL_HANDLE)
    {
        VmaStats stats;
        vmaCalculateStats(vk.memory_allocator, &stats);

        vgpu_log(VGPU_LOG_LEVEL_INFO, "Total device memory leaked: % "PRIu64" bytes.", stats.total.usedBytes);
        vmaDestroyAllocator(vk.memory_allocator);
    }

    if (vk.device) vkDestroyDevice(vk.device, NULL);

    if (vk.surface) {
        vkDestroySurfaceKHR(vk.instance, vk.surface, NULL);
        vk.surface = VK_NULL_HANDLE;
    }

    if (vk.messenger) vkDestroyDebugUtilsMessengerEXT(vk.instance, vk.messenger, NULL);
    if (vk.instance) vkDestroyInstance(vk.instance, NULL);

#if defined(_WIN32)
    FreeLibrary(vk.library);
#else
    dlclose(vk.library);
#endif

    memset(&vk, 0, sizeof(vk));
}

static bool vk_frame_begin(void)
{
    return true;
}

static void vk_frame_finish(void)
{
}

static void vk_query_caps(vgpu_caps* caps) {
    *caps = vk.caps;
}

/* Buffer */
static vgpu_buffer vk_buffer_create(const vgpu_buffer_info* info)
{
    vk_buffer* buffer = VGPU_ALLOC(vk_buffer);
    return (vgpu_buffer)buffer;
}

static void vk_buffer_destroy(vgpu_buffer handle)
{
}

/* Shader */
static vgpu_shader vk_shader_create(const vgpu_shader_info* info)
{
    vk_shader* shader = VGPU_ALLOC(vk_shader);
    return (vgpu_shader)shader;
}

static void vk_shader_destroy(vgpu_shader handle)
{

}

/* Texture */
static vgpu_texture vk_texture_create(const vgpu_texture_info* info) {
    return NULL;
}

static void vk_texture_destroy(vgpu_texture handle) {
}

static uint64_t vk_texture_get_native_handle(vgpu_texture handle) {
    vk_texture* texture = (vk_texture*)handle;
    return VOIDP_TO_U64(texture->handle);
}

/* Pipeline */
static vgpu_pipeline vk_pipeline_create(const vgpu_pipeline_info* info)
{
    vk_pipeline* pipeline = VGPU_ALLOC(vk_pipeline);
    return (vgpu_pipeline)pipeline;
}

static void vk_pipeline_destroy(vgpu_pipeline handle)
{
}

/* Commands */
static void vk_push_debug_group(const char* name)
{
}

static void vk_pop_debug_group(void)
{
}

static void vk_insert_debug_marker(const char* name)
{
}

static void vk_begin_render_pass(const vgpu_render_pass_info* info)
{

}

static void vk_end_render_pass(void)
{
}

static void vk_bind_pipeline(vgpu_pipeline handle)
{
    //vk_pipeline* pipeline = (vk_pipeline*)handle;
}

static void vk_draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex)
{
}

static VkBool32 _vgpu_vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT flags,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* context)
{
    // Log debug messge
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        vgpu_log(VGPU_LOG_LEVEL_WARN, "Vulkan: %s", data->pMessage);
    }
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        vgpu_log(VGPU_LOG_LEVEL_ERROR, "Vulkan: %s", data->pMessage);
    }
    return VK_FALSE;
}

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

static vk_queue_family_indices _vgpu_vk_query_queue_families(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    uint32_t queue_count = 0u;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_count, NULL);

    VkQueueFamilyProperties* queue_families = VGPU_ALLOCA(VkQueueFamilyProperties, queue_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_count, queue_families);

    vk_queue_family_indices result;
    result.graphics = VK_QUEUE_FAMILY_IGNORED;
    result.compute = VK_QUEUE_FAMILY_IGNORED;
    result.copy = VK_QUEUE_FAMILY_IGNORED;

    for (uint32_t i = 0; i < queue_count; i++)
    {
        VkBool32 present_support = VK_TRUE;
        if (surface != VK_NULL_HANDLE) {
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
        }

        static const VkQueueFlags required = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT;
        if (present_support && ((queue_families[i].queueFlags & required) == required))
        {
            result.graphics = i;
            break;
        }
    }

    /* Dedicated compute queue. */
    for (uint32_t i = 0; i < queue_count; i++)
    {
        static const VkQueueFlags required = VK_QUEUE_COMPUTE_BIT;
        if (i != result.graphics &&
            (queue_families[i].queueFlags & required) == required)
        {
            result.compute = i;
            break;
        }
    }

    /* Dedicated transfer/copy queue. */
    for (uint32_t i = 0; i < queue_count; i++)
    {
        static const VkQueueFlags required = VK_QUEUE_TRANSFER_BIT;
        if (i != result.graphics &&
            i != result.compute &&
            (queue_families[i].queueFlags & required) == required)
        {
            result.copy = i;
            break;
        }
    }

    if (result.copy == VK_QUEUE_FAMILY_IGNORED)
    {
        for (uint32_t i = 0; i < queue_count; i++)
        {
            static const VkQueueFlags required = VK_QUEUE_TRANSFER_BIT;
            if (i != result.graphics &&
                (queue_families[i].queueFlags & required) == required)
            {
                result.copy = i;
                break;
            }
        }
    }

    return result;
}

static vk_physical_device_extensions _vgpu_vk_query_physical_device_extensions(VkPhysicalDevice physical_device) {
    uint32_t count = 0;
    VGPU_VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, NULL, &count, NULL));

    VkExtensionProperties* extensions = VGPU_ALLOCA(VkExtensionProperties, count);
    VGPU_VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, NULL, &count, extensions));

    vk_physical_device_extensions result;
    memset(&result, 0, sizeof(result));

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
    VkPhysicalDeviceProperties2 gpu_props2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2
    };
    vkGetPhysicalDeviceProperties2(physical_device, &gpu_props2);

    /* We run on vulkan 1.1 or higher. */
    if (gpu_props2.properties.apiVersion >= VK_API_VERSION_1_1)
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

static bool _vgpu_vk_is_device_suitable(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    vk_queue_family_indices indices = _vgpu_vk_query_queue_families(physical_device, surface);

    if (indices.graphics == VK_QUEUE_FAMILY_IGNORED)
        return false;

    vk_physical_device_extensions features = _vgpu_vk_query_physical_device_extensions(physical_device);

    /* We required maintenance_1 to support viewport flipping to match DX style. */
    if (!features.maintenance_1) {
        return false;
    }

    return true;
}

/* Driver */
static bool vk_is_supported(void)
{
    if (vk.available_initialized) {
        return vk.available;
    }

    vk.available_initialized = true;

#if defined(_WIN32)
    vk.library = LoadLibraryA("vulkan-1.dll");
    if (!vk.library)
        return false;

    // note: function pointer is cast through void function pointer to silence cast-function-type warning on gcc8
    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)(void(*)(void))GetProcAddress(vk.library, "vkGetInstanceProcAddr");
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
        return VK_ERROR_INITIALIZATION_FAILED;

    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(vk.library, "vkGetInstanceProcAddr");
#endif

    VGPU_FOREACH_ANONYMOUS(VGPU_LOAD_ANONYMOUS);

    VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &(VkApplicationInfo) {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .apiVersion = VK_API_VERSION_1_1,
        }
    };

    VkInstance temp_instance;
    if (vkCreateInstance(&instance_info, NULL, &temp_instance) != VK_SUCCESS) {
        return false;
    }

    vkDestroyInstance = (PFN_vkDestroyInstance)vkGetInstanceProcAddr(temp_instance, "vkDestroyInstance");
    vkDestroyInstance(temp_instance, NULL);

    vk.available = true;
    return true;
};

static vgpu_renderer* vk_create_renderer(void)
{
    static vgpu_renderer renderer = { 0 };
    ASSIGN_DRIVER(vk);
    return &renderer;
}

vgpu_driver vulkan_driver = {
    VGPU_BACKEND_TYPE_VULKAN,
    vk_is_supported,
    vk_create_renderer
};

#endif /* defined(VGPU_DRIVER_VULKAN)  */
