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
    X(vkResetCommandPool)\
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
    X(vkCmdDispatch)\
    X(vkAcquireNextImageKHR)\
    X(vkCreateSwapchainKHR)\
    X(vkDestroySwapchainKHR)\
    X(vkGetSwapchainImagesKHR)\
    X(vkQueuePresentKHR)

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

/* Constants */
enum {
    _VGPU_VK_INVALID_INDEX = ~0u,
    _VGPU_VK_MAX_PHYSICAL_DEVICES = 16,
};

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
} vgpu_vk_buffer;

typedef struct {
    VkImage handle;
    VmaAllocation allocation;
    VkImageLayout layout;
    VkImageAspectFlagBits aspect;
} vgpu_vk_texture;

typedef struct {
    VkBuffer handle;
} vgpu_vk_shader;

typedef struct {
    VkPipeline handle;
} vgpu_vk_pipeline;

typedef struct {
    VkSurfaceKHR surface;
    VkSwapchainKHR handle;
    VkFormat format;
    vgpu_extent3D size;
} vgpu_vk_swapchain;

typedef struct {
    VkObjectType type;
    void* handle;
    VmaAllocation allocation;
} vgpu_vk_ref;

typedef struct {
    vgpu_vk_ref* data;
    size_t capacity;
    size_t length;
} vgpu_vk_freelist;

typedef struct {
    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
    VkFence fence;
    vgpu_vk_freelist freelist;
} vgpu_vk_frame;

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

    VkPhysicalDevice physical_device;
    VkPhysicalDeviceProperties physical_device_properties;
    vk_queue_family_indices queue_families;
    vk_physical_device_extensions extensions;

    VkDevice        device;
    VkQueue         graphics_queue;
    VkQueue         compute_queue;
    VkQueue         copy_queue;

    VmaAllocator    memory_allocator;

    vgpu_vk_swapchain swapchain;

    uint64_t        frame_count;
    uint32_t        max_inflight_frames;
    uint32_t        frame_index;
    vgpu_vk_frame   frames[VGPU_NUM_INFLIGHT_FRAMES];
} vk;

static VkBool32 _vgpu_vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT flags, const VkDebugUtilsMessengerCallbackDataEXT* data, void* context);
static const char* _vgpu_vk_get_error_string(VkResult result);
static vk_queue_family_indices _vgpu_vk_query_queue_families(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
static vk_physical_device_extensions _vgpu_vk_query_physical_device_extensions(VkPhysicalDevice physical_device);
static bool _vgpu_vk_is_device_suitable(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
static void _vgpu_vk_destroy_free_list(vgpu_vk_frame* frame);
static bool _vgpu_vk_init_swapchain(vgpu_vk_swapchain* swapchain);
static void _vgpu_vk_shutdown_swapchain(vgpu_vk_swapchain* swapchain);
static void _vgpu_vk_set_name(void* handle, VkObjectType type, const char* name);
static void _vgpu_vk_defer_destroy(VkObjectType type, void* handle, VmaAllocation allocation);

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

static bool vk_init(const char* app_name, const vgpu_config* config) {
    vgpu_log(VGPU_LOG_LEVEL_INFO, "VGPU driver: Vulkan");

    VkResult result = VK_SUCCESS;
    memcpy(&vk.config, config, sizeof(vk.config));
    const bool headless = false;

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
    VkSurfaceKHR surface = _vgpu_vk_create_surface(config->swapchain_info.window_handle);

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
            if (!_vgpu_vk_is_device_suitable(physical_devices[i], surface)) {
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
                if (config->device_preference == VGPU_ADAPTER_TYPE_DISCRETE_GPU) {
                    score += 1000u;
                }
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                score += 90U;
                if (config->device_preference == VGPU_ADAPTER_TYPE_INTEGRATED_GPU) {
                    score += 1000u;
                }
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                score += 80u;
                if (config->device_preference == VGPU_ADAPTER_TYPE_VIRTUAL_GPU) {
                    score += 1000u;
                }
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                score += 70u;
                if (config->device_preference == VGPU_ADAPTER_TYPE_CPU) {
                    score += 1000u;
                }
                break;
            default:
                score += 10u;
                break;
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
        vkGetPhysicalDeviceProperties(vk.physical_device, &vk.physical_device_properties);
        vk.queue_families = _vgpu_vk_query_queue_families(vk.physical_device, surface);
        vk.extensions = _vgpu_vk_query_physical_device_extensions(vk.physical_device);

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

        /* Extensions now */
        const bool device_api_version11 = vk.physical_device_properties.apiVersion >= VK_API_VERSION_1_1;
        const char* enabled_extension_names[32];
        uint32_t enabled_extension_count = 0;

        if (!headless) {
            enabled_extension_names[enabled_extension_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        }

        if (vk.extensions.get_memory_requirements2 && vk.extensions.dedicated_allocation) {
            enabled_extension_names[enabled_extension_count++] = "VK_KHR_get_memory_requirements2";
            enabled_extension_names[enabled_extension_count++] = "VK_KHR_dedicated_allocation";
        }

        if (!device_api_version11)
        {
            if (vk.extensions.maintenance_1)
                enabled_extension_names[enabled_extension_count++] = "VK_KHR_maintenance1";

            if (vk.extensions.maintenance_2)
                enabled_extension_names[enabled_extension_count++] = "VK_KHR_maintenance2";

            if (vk.extensions.maintenance_3)
                enabled_extension_names[enabled_extension_count++] = "VK_KHR_maintenance3";
        }

        if (vk.extensions.image_format_list)
        {
            enabled_extension_names[enabled_extension_count++] = VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME;
        }

        //if (vk.extensions.sampler_mirror_clamp_to_edge) {
        //    enabled_extension_names[enabled_extension_count++] = VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME;
        //}

        if (vk.extensions.depth_clip_enable)
        {
            enabled_extension_names[enabled_extension_count++] = VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME;
        }

        VkPhysicalDeviceFeatures2 features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2
        };

        VkPhysicalDeviceMultiviewFeatures multiview_features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES,
            .multiview = VK_TRUE
        };
        void** ppNext = &features.pNext;

        if (vk.extensions.multiview)
        {
            if (!device_api_version11) {
                enabled_extension_names[enabled_extension_count++] = "VK_KHR_multiview";
            }

            *ppNext = &multiview_features;
            ppNext = &multiview_features.pNext;
        }

        // Enable device features we might care about.
        {
            VkPhysicalDeviceFeatures enabled_features;
            memset(&enabled_features, 0, sizeof(enabled_features));

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

        VkDeviceCreateInfo device_info = {
          .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
          .queueCreateInfoCount = queue_info_count,
          .pQueueCreateInfos = queue_info,
          .enabledExtensionCount = enabled_extension_count,
          .ppEnabledExtensionNames = enabled_extension_names
        };

        if (vk.get_physical_device_properties2)
            device_info.pNext = &features;
        else
            device_info.pEnabledFeatures = &features.features;

        if (vkCreateDevice(vk.physical_device, &device_info, NULL, &vk.device) != VK_SUCCESS) {
            vgpu_shutdown();
            return false;
        }

        vkGetDeviceQueue(vk.device, vk.queue_families.graphics, 0, &vk.graphics_queue);
        vkGetDeviceQueue(vk.device, vk.queue_families.compute, compute_queue_index, &vk.compute_queue);
        vkGetDeviceQueue(vk.device, vk.queue_families.copy, copy_queue_index, &vk.copy_queue);

        VGPU_FOREACH_DEVICE(VGPU_LOAD_DEVICE);

        vgpu_log(VGPU_LOG_LEVEL_INFO, "Created VkDevice using '%s' adapter with API version: %u.%u.%u",
            vk.physical_device_properties.deviceName,
            VK_VERSION_MAJOR(vk.physical_device_properties.apiVersion),
            VK_VERSION_MINOR(vk.physical_device_properties.apiVersion),
            VK_VERSION_PATCH(vk.physical_device_properties.apiVersion));
        for (uint32_t i = 0; i < device_info.enabledExtensionCount; ++i)
        {
            vgpu_log(VGPU_LOG_LEVEL_INFO, "Device extension '%s'", device_info.ppEnabledExtensionNames[i]);
        }
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

    if (!headless) {
        vk.swapchain.surface = surface;
        _vgpu_vk_init_swapchain(&vk.swapchain);
    }

    /* Init caps now */
    {
        vk.caps.backend = VGPU_BACKEND_TYPE_VULKAN;
        vk.caps.vendor_id = vk.physical_device_properties.vendorID;
        vk.caps.adapter_id = vk.physical_device_properties.deviceID;
        memcpy(vk.caps.adapter_name, vk.physical_device_properties.deviceName, strlen(vk.physical_device_properties.deviceName));

        // Detect adapter type.
        switch (vk.physical_device_properties.deviceType)
        {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            vk.caps.adapter_type = VGPU_ADAPTER_TYPE_INTEGRATED_GPU;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            vk.caps.adapter_type = VGPU_ADAPTER_TYPE_DISCRETE_GPU;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            vk.caps.adapter_type = VGPU_ADAPTER_TYPE_VIRTUAL_GPU;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            vk.caps.adapter_type = VGPU_ADAPTER_TYPE_CPU;
            break;
        default:
            vk.caps.adapter_type = VGPU_ADAPTER_TYPE_OTHER;
            break;
        }

        vk.caps.features.independentBlend = true;
        vk.caps.features.computeShader = true;
        vk.caps.features.indexUInt32 = true;
        vk.caps.features.fillModeNonSolid = true;
        vk.caps.features.samplerAnisotropy = true;
        vk.caps.features.textureCompressionETC2 = false;
        vk.caps.features.textureCompressionASTC_LDR = false;
        vk.caps.features.textureCompressionBC = true;
        vk.caps.features.textureCubeArray = true;
        vk.caps.features.raytracing = vk.extensions.raytracing;

        // Limits
        vk.caps.limits.maxVertexAttributes = vk.physical_device_properties.limits.maxVertexInputAttributes;
        vk.caps.limits.maxVertexBindings = vk.physical_device_properties.limits.maxVertexInputBindings;
        vk.caps.limits.maxVertexAttributeOffset = vk.physical_device_properties.limits.maxVertexInputAttributeOffset;
        vk.caps.limits.maxVertexBindingStride = vk.physical_device_properties.limits.maxVertexInputBindingStride;

        //limits.maxTextureDimension1D = physicalDeviceProperties.limits.maxImageDimension1D;
        vk.caps.limits.maxTextureDimension2D = vk.physical_device_properties.limits.maxImageDimension2D;
        vk.caps.limits.maxTextureDimension3D = vk.physical_device_properties.limits.maxImageDimension3D;
        vk.caps.limits.maxTextureDimensionCube = vk.physical_device_properties.limits.maxImageDimensionCube;
        vk.caps.limits.maxTextureArrayLayers = vk.physical_device_properties.limits.maxImageArrayLayers;
        vk.caps.limits.maxColorAttachments = vk.physical_device_properties.limits.maxColorAttachments;
        vk.caps.limits.max_uniform_buffer_range = vk.physical_device_properties.limits.maxUniformBufferRange;
        vk.caps.limits.min_uniform_buffer_offset_alignment = vk.physical_device_properties.limits.minUniformBufferOffsetAlignment;
        vk.caps.limits.max_storage_buffer_range = vk.physical_device_properties.limits.maxStorageBufferRange;
        vk.caps.limits.min_storage_buffer_offset_alignment = vk.physical_device_properties.limits.minStorageBufferOffsetAlignment;
        vk.caps.limits.max_sampler_anisotropy = vk.physical_device_properties.limits.maxSamplerAnisotropy;
        vk.caps.limits.max_viewports = vk.physical_device_properties.limits.maxViewports;
        vk.caps.limits.max_viewport_width = vk.physical_device_properties.limits.maxViewportDimensions[0];
        vk.caps.limits.max_viewport_height = vk.physical_device_properties.limits.maxViewportDimensions[1];
        vk.caps.limits.max_tessellation_patch_size = vk.physical_device_properties.limits.maxTessellationPatchSize;
        vk.caps.limits.point_size_range_min = vk.physical_device_properties.limits.pointSizeRange[0];
        vk.caps.limits.point_size_range_max = vk.physical_device_properties.limits.pointSizeRange[1];
        vk.caps.limits.line_width_range_min = vk.physical_device_properties.limits.lineWidthRange[0];
        vk.caps.limits.line_width_range_max = vk.physical_device_properties.limits.lineWidthRange[1];
        vk.caps.limits.max_compute_shared_memory_size = vk.physical_device_properties.limits.maxComputeSharedMemorySize;
        vk.caps.limits.max_compute_work_group_count[0] = vk.physical_device_properties.limits.maxComputeWorkGroupCount[0];
        vk.caps.limits.max_compute_work_group_count[1] = vk.physical_device_properties.limits.maxComputeWorkGroupCount[1];
        vk.caps.limits.max_compute_work_group_count[2] = vk.physical_device_properties.limits.maxComputeWorkGroupCount[2];
        vk.caps.limits.max_compute_work_group_invocations = vk.physical_device_properties.limits.maxComputeWorkGroupInvocations;
        vk.caps.limits.max_compute_work_group_size[0] = vk.physical_device_properties.limits.maxComputeWorkGroupSize[0];
        vk.caps.limits.max_compute_work_group_size[1] = vk.physical_device_properties.limits.maxComputeWorkGroupSize[1];
        vk.caps.limits.max_compute_work_group_size[2] = vk.physical_device_properties.limits.maxComputeWorkGroupSize[2];
    }

    /* Frame data */
    {
        vk.frame_count = 0;
        vk.max_inflight_frames = 2u;
        vk.frame_index = 0;

        const VkCommandPoolCreateInfo graphics_command_pool_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = vk.queue_families.graphics
        };

        const VkFenceCreateInfo fence_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        for (uint32_t i = 0; i < VGPU_COUNT_OF(vk.frames); i++) {
            if (vkCreateCommandPool(vk.device, &graphics_command_pool_info, NULL, &vk.frames[i].command_pool)) {
                vgpu_shutdown();
                return false;
            }

            VkCommandBufferAllocateInfo command_buffer_info = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = vk.frames[i].command_pool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1
            };

            if (vkAllocateCommandBuffers(vk.device, &command_buffer_info, &vk.frames[i].command_buffer)) {
                vgpu_shutdown();
                return false;
            }

            if (vkCreateFence(vk.device, &fence_info, NULL, &vk.frames[i].fence)) {
                vgpu_shutdown();
                return false;
            }
        }
    }

    return true;
}


static void vk_shutdown(void) {
    if (vk.device) {
        vkDeviceWaitIdle(vk.device);
    }

    _vgpu_vk_shutdown_swapchain(&vk.swapchain);

    for (uint32_t i = 0; i < VGPU_COUNT_OF(vk.frames); i++) {
        vgpu_vk_frame* frame = &vk.frames[i];

        if (frame->fence) {
            vkDestroyFence(vk.device, frame->fence, NULL);
        }

        if (frame->command_buffer) {
            vkFreeCommandBuffers(vk.device, frame->command_pool, 1, &frame->command_buffer);
        }

        if (frame->command_pool) {
            vkDestroyCommandPool(vk.device, frame->command_pool, NULL);
        }
    }

    if (vk.memory_allocator != VK_NULL_HANDLE)
    {
        VmaStats stats;
        vmaCalculateStats(vk.memory_allocator, &stats);

        vgpu_log(VGPU_LOG_LEVEL_INFO, "Total device memory leaked: % "PRIu64" bytes.", stats.total.usedBytes);
        vmaDestroyAllocator(vk.memory_allocator);
    }

    if (vk.device) {
        vkDestroyDevice(vk.device, NULL);
    }

    //if (vk.surface) {
    //    vkDestroySurfaceKHR(vk.instance, vk.surface, NULL);
    //    vk.surface = VK_NULL_HANDLE;
    //}

    if (vk.messenger) vkDestroyDebugUtilsMessengerEXT(vk.instance, vk.messenger, NULL);
    if (vk.instance) vkDestroyInstance(vk.instance, NULL);

#if defined(_WIN32)
    FreeLibrary(vk.library);
#else
    dlclose(vk.library);
#endif

    memset(&vk, 0, sizeof(vk));
}

static bool vk_frame_begin(void) {
    VGPU_VK_CHECK(vkWaitForFences(vk.device, 1, &vk.frames[vk.frame_index].fence, VK_FALSE, ~0ull));
    VGPU_VK_CHECK(vkResetFences(vk.device, 1, &vk.frames[vk.frame_index].fence));
    VGPU_VK_CHECK(vkResetCommandPool(vk.device, vk.frames[vk.frame_index].command_pool, 0));
    _vgpu_vk_destroy_free_list(&vk.frames[vk.frame_index]);

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    VGPU_VK_CHECK(vkBeginCommandBuffer(vk.frames[vk.frame_index].command_buffer, &begin_info));

    return true;
}

static void vk_frame_finish(void) {
    VGPU_VK_CHECK(vkEndCommandBuffer(vk.frames[vk.frame_index].command_buffer));

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pCommandBuffers = &vk.frames[vk.frame_index].command_buffer,
        .commandBufferCount = 1
    };

    VGPU_VK_CHECK(vkQueueSubmit(vk.graphics_queue, 1, &submit_info, vk.frames[vk.frame_index].fence));

    vk.frame_count++;
    vk.frame_index = vk.frame_count % vk.max_inflight_frames;
}

static void vk_query_caps(vgpu_caps* caps) {
    *caps = vk.caps;
}

/* Buffer */
static vgpu_buffer vk_buffer_create(const vgpu_buffer_info* info)
{
    vgpu_vk_buffer* buffer = VGPU_ALLOC(vgpu_vk_buffer);
    return (vgpu_buffer)buffer;
}

static void vk_buffer_destroy(vgpu_buffer handle)
{
}

/* Shader */
static vgpu_shader vk_shader_create(const vgpu_shader_info* info)
{
    vgpu_vk_shader* shader = VGPU_ALLOC(vgpu_vk_shader);
    return (vgpu_shader)shader;
}

static void vk_shader_destroy(vgpu_shader handle)
{

}

/* Texture */
static vgpu_texture vk_texture_create(const vgpu_texture_info* info) {
    vgpu_vk_texture* texture = VGPU_ALLOC(vgpu_vk_texture);
    if (info->external_handle) {
        texture->handle = (VkImage) info->external_handle;
        texture->allocation = VK_NULL_HANDLE;
    }
    else {

    }

    texture->layout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vgpu_is_depth_stencil_format(info->format)) {
        texture->aspect = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (vgpu_is_stencil_format(info->format)) {
            texture->aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else {
        texture->aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    _vgpu_vk_set_name(texture->handle, VK_OBJECT_TYPE_IMAGE, info->label);

    return (vgpu_texture)texture;
}

static void vk_texture_destroy(vgpu_texture handle) {
    vgpu_vk_texture* texture = (vgpu_vk_texture*)handle;
    if (texture->allocation) {
        _vgpu_vk_defer_destroy(VK_OBJECT_TYPE_IMAGE, texture->handle, texture->allocation);
    }

    VGPU_FREE(texture);
}

static uint64_t vk_texture_get_native_handle(vgpu_texture handle) {
    vgpu_vk_texture* texture = (vgpu_vk_texture*)handle;
    return VOIDP_TO_U64(texture->handle);
}

/* Pipeline */
static vgpu_pipeline vk_pipeline_create(const vgpu_pipeline_info* info) {
    vgpu_vk_pipeline* pipeline = VGPU_ALLOC(vgpu_vk_pipeline);
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

static void _vgpu_vk_destroy_free_list(vgpu_vk_frame* frame) {
    for (size_t i = 0; i < frame->freelist.length; i++) {
        vgpu_vk_ref* ref = &frame->freelist.data[i];
        switch (ref->type) {
        case VK_OBJECT_TYPE_BUFFER: vmaDestroyBuffer(vk.memory_allocator, (VkBuffer)ref->handle, ref->allocation); break;
        case VK_OBJECT_TYPE_IMAGE: vmaDestroyImage(vk.memory_allocator, (VkImage)ref->handle, ref->allocation); break;
        case VK_OBJECT_TYPE_IMAGE_VIEW: vkDestroyImageView(vk.device, (VkImageView)ref->handle, NULL); break;
        case VK_OBJECT_TYPE_SAMPLER: vkDestroySampler(vk.device, (VkSampler)ref->handle, NULL); break;
        case VK_OBJECT_TYPE_RENDER_PASS: vkDestroyRenderPass(vk.device, (VkRenderPass)ref->handle, NULL); break;
        case VK_OBJECT_TYPE_FRAMEBUFFER: vkDestroyFramebuffer(vk.device, (VkFramebuffer)ref->handle, NULL); break;
        case VK_OBJECT_TYPE_PIPELINE: vkDestroyPipeline(vk.device, (VkPipeline)ref->handle, NULL); break;
        default: VGPU_UNREACHABLE(); break;
        }
    }
    frame->freelist.length = 0;
}

static bool _vgpu_vk_init_swapchain(vgpu_vk_swapchain* swapchain) {
    VkSurfaceCapabilitiesKHR surface_properties;
    VGPU_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.physical_device, swapchain->surface, &surface_properties));

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physical_device, swapchain->surface, &format_count, NULL);
    VkSurfaceFormatKHR* formats = VGPU_ALLOCA(VkSurfaceFormatKHR, format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physical_device, swapchain->surface, &format_count, formats);

    VkSurfaceFormatKHR format;
    if (format_count == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
    {
        // There is no preferred format, so pick a default one
        format = formats[0];
        format.format = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else
    {
        if (format_count == 0)
        {
            vgpu_log(VGPU_LOG_LEVEL_ERROR, "Vulkan: Surface has no formats.");
            return false;
        }

        format.format = VK_FORMAT_UNDEFINED;
        for (uint32_t i = 0; i < format_count; ++i)
        {
            switch (formats[i].format)
            {
            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_B8G8R8A8_UNORM:
            case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
                format = formats[i];
                break;

            default:
                break;
            }

            if (format.format != VK_FORMAT_UNDEFINED)
            {
                break;
            }
        }

        if (format.format == VK_FORMAT_UNDEFINED)
        {
            format = formats[0];
        }
    }

    swapchain->size.width = surface_properties.currentExtent.width;
    swapchain->size.height = surface_properties.currentExtent.height;
    swapchain->size.depth = 1u;

    // FIFO must be supported by all implementations.
    VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;

    // Determine the number of VkImage's to use in the swapchain.
    // Ideally, we desire to own 1 image at a time, the rest of the images can
    // either be rendered to and/or being queued up for display.
    uint32_t desired_swapchain_images = surface_properties.minImageCount + 1;
    if ((surface_properties.maxImageCount > 0) && (desired_swapchain_images > surface_properties.maxImageCount))
    {
        // Application must settle for fewer images than desired.
        desired_swapchain_images = surface_properties.maxImageCount;
    }

    // Figure out a suitable surface transform.
    VkSurfaceTransformFlagBitsKHR pre_transform;
    if (surface_properties.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
        pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
        pre_transform = surface_properties.currentTransform;
    }

    VkSwapchainKHR old_swapchain = swapchain->handle;

    // Find a supported composite type.
    VkCompositeAlphaFlagBitsKHR composite = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if (surface_properties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
    {
        composite = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    }
    else if (surface_properties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
    {
        composite = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    }
    else if (surface_properties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
    {
        composite = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
    }
    else if (surface_properties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
    {
        composite = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
    }

    const VkSwapchainCreateInfoKHR swapchain_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = swapchain->surface,
        .minImageCount = desired_swapchain_images,
        .imageFormat = format.format,
        .imageColorSpace = format.colorSpace,
        .imageExtent.width = swapchain->size.width,
        .imageExtent.height = swapchain->size.height,
        .imageArrayLayers = 1u,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = pre_transform,
        .compositeAlpha = composite,
        .presentMode = swapchain_present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = old_swapchain
    };
    VGPU_VK_CHECK(vkCreateSwapchainKHR(vk.device, &swapchain_info, NULL, &swapchain->handle));

    swapchain->format = format.format;

    uint32_t image_count;
    VkImage images[8];
    VGPU_VK_CHECK(vkGetSwapchainImagesKHR(vk.device, swapchain->handle, &image_count, NULL));
    VGPU_VK_CHECK(vkGetSwapchainImagesKHR(vk.device, swapchain->handle, &image_count, images));

    for (uint32_t i = 0; i < image_count; i++)
    {
        const vgpu_texture_info color_texture_info = {
            .type = VGPU_TEXTURE_TYPE_2D,
            .usage = VGPU_TEXTURE_USAGE_RENDER_TARGET,
            .format = VGPU_TEXTURE_FORMAT_BGRA8,
            .size = swapchain->size,
            .external_handle = VOIDP_TO_U64(images[i]),
            .label = "Backbuffer"
        };

        vgpu_create_texture(&color_texture_info);

        /*const VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapchain->format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_R,
                .g = VK_COMPONENT_SWIZZLE_G,
                .b = VK_COMPONENT_SWIZZLE_B,
                .a = VK_COMPONENT_SWIZZLE_A
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0u,
                .levelCount = 1u,
                .baseArrayLayer = 0u,
                .layerCount = 1u
            }
        };

        VkImageView image_view;
        VGPU_VK_CHECK(vkCreateImageView(vk.device, &view_info, NULL, &image_view));*/
    }

    return true;
}

static void _vgpu_vk_shutdown_swapchain(vgpu_vk_swapchain* swapchain) {
    if (swapchain->handle != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(vk.device, swapchain->handle, NULL);
        swapchain->handle = VK_NULL_HANDLE;
    }

    if (swapchain->surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(vk.instance, swapchain->surface, NULL);
        swapchain->surface = VK_NULL_HANDLE;
    }
}

void _vgpu_vk_set_name(void* handle, VkObjectType type, const char* name) {
    if (name && vk.config.debug && vk.debug_utils) {
        VkDebugUtilsObjectNameInfoEXT info = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .objectType = type,
            .objectHandle = VOIDP_TO_U64(handle),
            .pObjectName = name
        };

        VGPU_VK_CHECK(vkSetDebugUtilsObjectNameEXT(vk.device, &info));
    }
}

static void _vgpu_vk_defer_destroy(VkObjectType type, void* handle, VmaAllocation allocation) {
    vgpu_vk_freelist* freelist = &vk.frames[vk.frame_index].freelist;

    if (freelist->length >= freelist->capacity) {
        freelist->capacity = freelist->capacity ? (freelist->capacity * 2) : 1;
        freelist->data = realloc(freelist->data, freelist->capacity * sizeof(*freelist->data));
        VGPU_CHECK(freelist->data, "Out of memory");
    }

    freelist->data[freelist->length++] = (vgpu_vk_ref){ type, handle, allocation };
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
