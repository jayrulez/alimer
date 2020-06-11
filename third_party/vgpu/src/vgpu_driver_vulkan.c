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

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "vgpu_driver.h"

/* Needed by vk_mem_alloc */
PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = NULL;
PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = NULL;

// Functions that don't require an instance
#define GPU_FOREACH_ANONYMOUS(X)\
    X(vkCreateInstance)\
    X(vkEnumerateInstanceExtensionProperties)\
    X(vkEnumerateInstanceLayerProperties)\
    X(vkEnumerateInstanceVersion)

// Functions that require an instance but don't require a device
#define GPU_FOREACH_INSTANCE(X)\
    X(vkCreateDevice)\
    X(vkDestroyInstance)\
    X(vkCreateDebugUtilsMessengerEXT)\
    X(vkDestroyDebugUtilsMessengerEXT)\
    X(vkEnumeratePhysicalDevices)\
    X(vkGetPhysicalDeviceProperties)\
    X(vkEnumerateDeviceExtensionProperties)\
    X(vkGetPhysicalDeviceFeatures)\
    X(vkGetPhysicalDeviceMemoryProperties)\
    X(vkGetPhysicalDeviceQueueFamilyProperties)\
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
#define VGPU_FOREACH_DEVICE(X)\
    X(vkDestroyDevice)\
    X(vkGetDeviceQueue)\
    X(vkDeviceWaitIdle)

#define VGPU_DECLARE(fn) static PFN_##fn fn;
#define GPU_LOAD_ANONYMOUS(fn) fn = (PFN_##fn) vkGetInstanceProcAddr(NULL, #fn);
#define VGPU_LOAD_INSTANCE(fn) fn = (PFN_##fn) vkGetInstanceProcAddr(vk.instance, #fn);
#define VGPU_LOAD_DEVICE(fn) fn = (PFN_##fn) vkGetDeviceProcAddr(vk.device, #fn);

/* Declare function pointers */
GPU_FOREACH_ANONYMOUS(VGPU_DECLARE);
GPU_FOREACH_INSTANCE(VGPU_DECLARE);
VGPU_FOREACH_INSTANCE_SURFACE(VGPU_DECLARE);
VGPU_FOREACH_DEVICE(VGPU_DECLARE);

#define _VK_GPU_MAX_PHYSICAL_DEVICES (32u)
#define GPU_CHECK(c, s) if (!(c)) { vgpu_log(VGPU_LOG_LEVEL_ERROR, s); VGPU_ASSERT(0); }
#define VK_CHECK(f) do { VkResult r = (f); GPU_CHECK(r >= 0, _vgpu_vk_get_error_string(r)); } while (0)

/* Vulkan structures */
typedef struct vk_queue_family_indices {
    uint32_t graphics_queue_family;
    uint32_t compute_queue_family;
    uint32_t transfer_queue_family;
} vk_queue_family_indices;

typedef struct vk_physical_device_features {
    bool swapchain;
    bool depth_clip_enable;
    bool maintenance_1;
    bool maintenance_2;
    bool maintenance_3;
    bool get_memory_requirements2;
    bool dedicated_allocation;
    bool bind_memory2;
    bool memory_budget;
    bool image_format_list;
    bool debug_marker;
    bool win32_full_screen_exclusive;
    bool raytracing;
    bool buffer_device_address;
    bool deferred_host_operations;
    bool descriptor_indexing;
    bool pipeline_library;
    bool multiview;
} vk_physical_device_features;

typedef struct vk_texture {
    VkImage handle;
    uint32_t width;
    uint32_t height;
} vk_texture;

typedef struct vk_framebuffer {
    uint32_t width;
    uint32_t height;
    uint32_t layers;
} vk_framebuffer;

typedef struct vk_swapchain {
    VkSurfaceKHR surface;
    VkSwapchainKHR handle;
    uint32_t width;
    uint32_t height;
} vk_swapchain;

/* Global data */
static struct {
    bool available_initialized;
    bool available;
    void* library;
    vgpu_caps caps;

    bool debug_utils;
    bool headless_extension;
    bool surface;
    bool get_surface_capabilities2;
    bool get_physical_device_properties2;
    bool external_memory_capabilities;
    bool external_semaphore_capabilities;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_utils_messenger;

    VkPhysicalDevice physical_device;
    VkPhysicalDeviceProperties physical_device_properties;
    vk_queue_family_indices physical_device_queue_families;
    vk_physical_device_features physical_device_features;

    VkDevice device;
    VkQueue graphics_queue;
    VkQueue compute_queue;
    VkQueue transfer_queue;

    VmaAllocator allocator;
} vk;

/* Helper methods */
static const char* _vgpu_vk_get_error_string(VkResult result);
static VkBool32 VKAPI_CALL debug_utils_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT flags, const VkDebugUtilsMessengerCallbackDataEXT* data, void* context);
static bool _vgpu_vk_create_surface(uintptr_t native_handle, VkSurfaceKHR* pSurface);
static bool _vgpu_vk_is_device_suitable(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
static bool _vgpu_vk_query_presentation_support(VkPhysicalDevice physical_device, uint32_t queue_family_index);
static vk_queue_family_indices _vgpu_vk_query_queue_families(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
static vk_physical_device_features _vgpu_vk_query_device_extensions(VkPhysicalDevice physical_device);

static bool vk_init(const vgpu_config* config) {
    VkResult result = VK_SUCCESS;

    // Create Instance
    {
        uint32_t instance_extension_count;
        VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, NULL));

        VkExtensionProperties* available_instance_extensions = VGPU_ALLOCA(VkExtensionProperties, instance_extension_count);
        VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, available_instance_extensions));

        for (uint32_t i = 0; i < instance_extension_count; ++i)
        {
            if (strcmp(available_instance_extensions[i].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
                vk.debug_utils = true;
            }
            else if (strcmp(available_instance_extensions[i].extensionName, VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME) == 0)
            {
                vk.headless_extension = true;
            }
            else if (strcmp(available_instance_extensions[i].extensionName, VK_KHR_SURFACE_EXTENSION_NAME) == 0)
            {
                vk.surface = true;
            }
            else if (strcmp(available_instance_extensions[i].extensionName, VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME) == 0)
            {
                vk.get_surface_capabilities2 = true;
            }
            else if (strcmp(available_instance_extensions[i].extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0)
            {
                vk.get_physical_device_properties2 = true;
            }
            else if (strcmp(available_instance_extensions[i].extensionName, VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME) == 0)
            {
                vk.external_memory_capabilities = true;
            }
            else if (strcmp(available_instance_extensions[i].extensionName, VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME) == 0)
            {
                vk.external_semaphore_capabilities = true;
            }
        }

        uint32_t enabled_instance_exts_count = 0;
        uint32_t enabled_instance_layers_count = 0;
        char* enabled_instance_exts[16];
        char* enabled_instance_layers[6];

        /* Features promoted to 1.1 */
        if (vk.get_physical_device_properties2) {
            enabled_instance_exts[enabled_instance_exts_count++] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;

            if (vk.external_memory_capabilities &&
                vk.external_semaphore_capabilities)
            {
                enabled_instance_exts[enabled_instance_exts_count++] = VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME;
                enabled_instance_exts[enabled_instance_exts_count++] = VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME;
            }
        }

        if (config->debug && vk.debug_utils) {
            enabled_instance_exts[enabled_instance_exts_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        }

        if (vk.surface) {
            enabled_instance_exts[enabled_instance_exts_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
            enabled_instance_exts[enabled_instance_exts_count++] = VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
            enabled_instance_exts[enabled_instance_exts_count++] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
            enabled_instance_exts[enabled_instance_exts_count++] = VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
#elif defined(VK_USE_PLATFORM_XCB_KHR)
            enabled_instance_exts[enabled_instance_exts_count++] = VK_KHR_XCB_SURFACE_EXTENSION_NAME;
#elif defined(VK_USE_PLATFORM_IOS_MVK)
            enabled_instance_exts[enabled_instance_exts_count++] = VK_MVK_IOS_SURFACE_EXTENSION_NAME;
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
            // TODO: Support VK_EXT_metal_surface
            enabled_instance_exts[enabled_instance_exts_count++] = VK_MVK_MACOS_SURFACE_EXTENSION_NAME;
#endif

            if (vk.get_surface_capabilities2) {
                enabled_instance_exts[enabled_instance_exts_count++] = VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME;
            }
        }

        /* Setup layers */
        if (config->debug || config->profile) {
            uint32_t instance_layer_count;
            VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL));

            VkLayerProperties* supported_validation_layers = VGPU_ALLOCA(VkLayerProperties, instance_layer_count);
            assert(supported_validation_layers);
            VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, supported_validation_layers));

            // Search for VK_LAYER_KHRONOS_validation first.
            bool found = false;
            for (uint32_t i = 0; i < instance_layer_count; ++i) {
                if (strcmp(supported_validation_layers[i].layerName, "VK_LAYER_KHRONOS_validation") == 0) {
                    enabled_instance_layers[enabled_instance_layers_count++] = "VK_LAYER_KHRONOS_validation";
                    found = true;
                    break;
                }
            }

            // Fallback to VK_LAYER_LUNARG_standard_validation.
            if (!found) {
                for (uint32_t i = 0; i < instance_layer_count; ++i) {
                    if (strcmp(supported_validation_layers[i].layerName, "VK_LAYER_LUNARG_standard_validation") == 0) {
                        enabled_instance_layers[enabled_instance_layers_count++] = "VK_LAYER_LUNARG_standard_validation";
                    }
                }
            }
        }

        uint32_t apiVersion = VK_API_VERSION_1_1;
        if (vkEnumerateInstanceVersion &&
            vkEnumerateInstanceVersion(&apiVersion) != VK_SUCCESS)
        {
            apiVersion = VK_API_VERSION_1_1;
        }

        if (apiVersion < VK_API_VERSION_1_1) {
            apiVersion = VK_API_VERSION_1_1;
        }

        VkInstanceCreateInfo instance_info = {
          .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
          .pApplicationInfo = &(VkApplicationInfo) {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .apiVersion = apiVersion
          },
          .enabledLayerCount = enabled_instance_layers_count,
          .ppEnabledLayerNames = enabled_instance_layers,
          .enabledExtensionCount = enabled_instance_exts_count,
          .ppEnabledExtensionNames = enabled_instance_exts
        };

        VkDebugUtilsMessengerCreateInfoEXT debug_utils_create_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
        if (config->debug || config->profile) {
            if (vk.debug_utils)
            {
                debug_utils_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
                debug_utils_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
                debug_utils_create_info.pfnUserCallback = debug_utils_messenger_callback;

                instance_info.pNext = &debug_utils_create_info;
            }
        }

        if (vkCreateInstance(&instance_info, NULL, &vk.instance)) {
            vgpu_shutdown();
            return false;
        }

        GPU_FOREACH_INSTANCE(VGPU_LOAD_INSTANCE);
        vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(vk.instance, "vkGetDeviceProcAddr");
        VGPU_FOREACH_INSTANCE_SURFACE(VGPU_LOAD_INSTANCE);

        if (config->debug || config->profile) {
            if (vk.debug_utils)
            {
                result = vkCreateDebugUtilsMessengerEXT(vk.instance, &debug_utils_create_info, NULL, &vk.debug_utils_messenger);
                if (result != VK_SUCCESS)
                {
                    vgpu_log_error("Could not create debug utils messenger");
                    vgpu_shutdown();
                    return false;
                }
            }
        }

        vgpu_log(VGPU_LOG_LEVEL_INFO, "Created VkInstance with version: %u.%u.%u",
            VK_VERSION_MAJOR(instance_info.pApplicationInfo->apiVersion),
            VK_VERSION_MINOR(instance_info.pApplicationInfo->apiVersion),
            VK_VERSION_PATCH(instance_info.pApplicationInfo->apiVersion));

        if (enabled_instance_layers_count) {
            for (uint32_t i = 0; i < enabled_instance_layers_count; i++) {
                vgpu_log(VGPU_LOG_LEVEL_INFO, "Instance layer '%s'", enabled_instance_layers[i]);
            }
        }

        for (uint32_t i = 0; i < enabled_instance_exts_count; i++) {
            vgpu_log(VGPU_LOG_LEVEL_INFO, "Instance extension '%s'", enabled_instance_exts[i]);
        }
    }

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (config->window_handle) {
        if (!_vgpu_vk_create_surface(config->window_handle, &surface))
        {
            vgpu_shutdown();
            return false;
        }
    }

    // Find physical device
    {
        uint32_t device_count = _VK_GPU_MAX_PHYSICAL_DEVICES;
        VkPhysicalDevice physical_devices[_VK_GPU_MAX_PHYSICAL_DEVICES];

        if (vkEnumeratePhysicalDevices(vk.instance, &device_count, physical_devices) != VK_SUCCESS) {
            vgpu_shutdown();
            return false;
        }

        uint32_t best_device_score = 0U;
        uint32_t best_device_index = VK_QUEUE_FAMILY_IGNORED;
        for (uint32_t i = 0; i < device_count; i++)
        {
            if (!_vgpu_vk_is_device_suitable(physical_devices[i], surface)) {
                continue;
            }

            VkPhysicalDeviceProperties physical_device_props;
            vkGetPhysicalDeviceProperties(physical_devices[i], &physical_device_props);

            uint32_t score = 0u;

            if (physical_device_props.apiVersion >= VK_API_VERSION_1_2) {
                score += 10000u;
            }

            switch (physical_device_props.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                score += 100U;
                if (config->device_preference == VGPU_DEVICE_PREFERENCE_HIGH_PERFORMANCE)
                    score += 1000u;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                score += 90U;
                if (config->device_preference == VGPU_DEVICE_PREFERENCE_LOW_POWER)
                    score += 1000u;
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

        if (best_device_index == VK_QUEUE_FAMILY_IGNORED) {
            vgpu_log(VGPU_LOG_LEVEL_ERROR, "Vulkan: Cannot find suitable physical device.");
            vgpu_shutdown();
            return false;
        }

        vk.physical_device = physical_devices[best_device_index];
        vkGetPhysicalDeviceProperties(vk.physical_device, &vk.physical_device_properties);
        vk.physical_device_queue_families = _vgpu_vk_query_queue_families(vk.physical_device, surface);
        vk.physical_device_features = _vgpu_vk_query_device_extensions(vk.physical_device);
    }

    /* Setup device queue's. */
    {
        uint32_t queue_count;
        vkGetPhysicalDeviceQueueFamilyProperties(vk.physical_device, &queue_count, NULL);
        VkQueueFamilyProperties* queue_families = VGPU_ALLOCA(VkQueueFamilyProperties, queue_count);
        vkGetPhysicalDeviceQueueFamilyProperties(vk.physical_device, &queue_count, queue_families);

        uint32_t universal_queue_index = 1;
        uint32_t graphics_queue_index = 0;
        uint32_t compute_queue_index = 0;
        uint32_t copy_queue_index = 0;

        if (vk.physical_device_queue_families.compute_queue_family == VK_QUEUE_FAMILY_IGNORED)
        {
            vk.physical_device_queue_families.compute_queue_family = vk.physical_device_queue_families.graphics_queue_family;
            compute_queue_index = _vgpu_min(queue_families[vk.physical_device_queue_families.graphics_queue_family].queueCount - 1, universal_queue_index);
            universal_queue_index++;
        }

        if (vk.physical_device_queue_families.transfer_queue_family == VK_QUEUE_FAMILY_IGNORED)
        {
            vk.physical_device_queue_families.transfer_queue_family = vk.physical_device_queue_families.graphics_queue_family;
            copy_queue_index = _vgpu_min(queue_families[vk.physical_device_queue_families.graphics_queue_family].queueCount - 1, universal_queue_index);
            universal_queue_index++;
        }
        else if (vk.physical_device_queue_families.transfer_queue_family == vk.physical_device_queue_families.compute_queue_family)
        {
            copy_queue_index = _vgpu_min(queue_families[vk.physical_device_queue_families.compute_queue_family].queueCount - 1, 1u);
        }

        static const float graphics_queue_prio = 0.5f;
        static const float compute_queue_prio = 1.0f;
        static const float transfer_queue_prio = 1.0f;
        float prio[3] = { graphics_queue_prio, compute_queue_prio, transfer_queue_prio };

        uint32_t queue_family_count = 0;
        VkDeviceQueueCreateInfo queue_info[3] = { 0 };
        queue_info[queue_family_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[queue_family_count].queueFamilyIndex = vk.physical_device_queue_families.graphics_queue_family;
        queue_info[queue_family_count].queueCount = _vgpu_min(universal_queue_index, queue_families[vk.physical_device_queue_families.graphics_queue_family].queueCount);
        queue_info[queue_family_count].pQueuePriorities = prio;
        queue_family_count++;

        if (vk.physical_device_queue_families.compute_queue_family != vk.physical_device_queue_families.graphics_queue_family)
        {
            queue_info[queue_family_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_info[queue_family_count].queueFamilyIndex = vk.physical_device_queue_families.compute_queue_family;
            queue_info[queue_family_count].queueCount = _vgpu_min(vk.physical_device_queue_families.transfer_queue_family == vk.physical_device_queue_families.compute_queue_family ? 2u : 1u,
                queue_families[vk.physical_device_queue_families.compute_queue_family].queueCount);
            queue_info[queue_family_count].pQueuePriorities = prio + 1;
            queue_family_count++;
        }

        if (vk.physical_device_queue_families.transfer_queue_family != vk.physical_device_queue_families.graphics_queue_family
            && vk.physical_device_queue_families.transfer_queue_family != vk.physical_device_queue_families.compute_queue_family)
        {
            queue_info[queue_family_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_info[queue_family_count].queueFamilyIndex = vk.physical_device_queue_families.transfer_queue_family;
            queue_info[queue_family_count].queueCount = 1;
            queue_info[queue_family_count].pQueuePriorities = prio + 2;
            queue_family_count++;
        }

        /* Setup device extensions now. */
        uint32_t enabled_device_ext_count = 0;
        char* enabled_device_exts[64];
        enabled_device_exts[enabled_device_ext_count++] = VK_KHR_MAINTENANCE1_EXTENSION_NAME;

        if (surface != VK_NULL_HANDLE) {
            enabled_device_exts[enabled_device_ext_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        }

        if (vk.physical_device_features.maintenance_2) {
            enabled_device_exts[enabled_device_ext_count++] = VK_KHR_MAINTENANCE2_EXTENSION_NAME;
        }

        if (vk.physical_device_features.maintenance_3) {
            enabled_device_exts[enabled_device_ext_count++] = VK_KHR_MAINTENANCE3_EXTENSION_NAME;
        }

        if (vk.physical_device_features.get_memory_requirements2 &&
            vk.physical_device_features.dedicated_allocation)
        {
            enabled_device_exts[enabled_device_ext_count++] = VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME;
            enabled_device_exts[enabled_device_ext_count++] = VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME;
        }

        /*if (vk.physical_device_features.buffer_device_address)
        {
            enabled_device_exts[enabled_device_ext_count++] = VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME;
        }*/

#ifdef _WIN32
        if (vk.get_surface_capabilities2 && vk.physical_device_features.win32_full_screen_exclusive)
        {
            enabled_device_exts[enabled_device_ext_count++] = VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME;
        }
#endif

        VkPhysicalDeviceFeatures2 features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        VkPhysicalDeviceMultiviewFeatures multiview_features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES };
        void** ppNext = &features.pNext;

        if (vk.physical_device_features.multiview)
        {
            enabled_device_exts[enabled_device_ext_count++] = VK_KHR_MULTIVIEW_EXTENSION_NAME;
            *ppNext = &multiview_features;
            ppNext = &multiview_features.pNext;
        }

        vkGetPhysicalDeviceFeatures2(vk.physical_device, &features);

        // Enable device features we might care about.
        {
            VkPhysicalDeviceFeatures enabled_features;
            memset(&enabled_features, 0, sizeof(VkPhysicalDeviceFeatures));

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

        const VkDeviceCreateInfo device_info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &features,
            .queueCreateInfoCount = queue_family_count,
            .pQueueCreateInfos = queue_info,
            .enabledExtensionCount = enabled_device_ext_count,
            .ppEnabledExtensionNames = enabled_device_exts
        };

        result = vkCreateDevice(vk.physical_device, &device_info, NULL, &vk.device);
        if (result != VK_SUCCESS) {
            vgpu_shutdown();
            return false;
        }

        VGPU_FOREACH_DEVICE(VGPU_LOAD_DEVICE);

        vkGetDeviceQueue(vk.device, vk.physical_device_queue_families.graphics_queue_family, graphics_queue_index, &vk.graphics_queue);
        vkGetDeviceQueue(vk.device, vk.physical_device_queue_families.compute_queue_family, compute_queue_index, &vk.compute_queue);
        vkGetDeviceQueue(vk.device, vk.physical_device_queue_families.transfer_queue_family, copy_queue_index, &vk.transfer_queue);
    }

    /* Create memory allocator. */
    {
        VmaAllocatorCreateFlags allocator_flags = 0;
        if (vk.physical_device_features.get_memory_requirements2 &&
            vk.physical_device_features.dedicated_allocation)
        {
            allocator_flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
        }

        if (vk.physical_device_features.buffer_device_address)
        {
            allocator_flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        }

        const VmaAllocatorCreateInfo allocator_info = {
            .flags = allocator_flags,
            .physicalDevice = vk.physical_device,
            .device = vk.device,
            .instance = vk.instance
        };

        result = vmaCreateAllocator(&allocator_info, &vk.allocator);
        if (result != VK_SUCCESS) {
            vgpu_log_error("Cannot create memory allocator.");
            vgpu_shutdown();
            return false;
        }
    }

    return true;
}

static void vk_shutdown(void) {
    if (vk.device) {
        vkDeviceWaitIdle(vk.device);
    }

    if (vk.allocator != VK_NULL_HANDLE)
    {
        VmaStats stats;
        vmaCalculateStats(vk.allocator, &stats);

        if (stats.total.usedBytes > 0) {
            vgpu_log(VGPU_LOG_LEVEL_ERROR, "Total device memory leaked: %llx bytes.", stats.total.usedBytes);
        }

        vmaDestroyAllocator(vk.allocator);
    }

    if (vk.device) {
        vkDestroyDevice(vk.device, NULL);
    }

    if (vk.debug_utils_messenger != VK_NULL_HANDLE)
    {
        vkDestroyDebugUtilsMessengerEXT(vk.instance, vk.debug_utils_messenger, NULL);
    }

    if (vk.instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(vk.instance, NULL);
    }

#if defined(_WIN32)
    FreeLibrary((HMODULE)vk.library);
#else
    dlclose(vk.library);
#endif
    memset(&vk, 0, sizeof(vk));
}

static void vk_get_caps(vgpu_caps* caps) {
    *caps = vk.caps;
}

static bool vk_frame_begin(void) {
    return true;
}

static void vk_frame_end(void) {

}

static void vk_insertDebugMarker(const char* name)
{
}

static void vk_pushDebugGroup(const char* name)
{
}

static void vk_popDebugGroup(void)
{
}

static void vk_render_begin(vgpu_framebuffer framebuffer) {
}

static void vk_render_finish(void) {

}

/* Texture */
static vgpu_texture vk_texture_create(const vgpu_texture_info* info) {
    vk_texture* texture = VGPU_ALLOC_HANDLE(vk_texture);
    return (vgpu_texture)texture;
}

static void vk_texture_destroy(vgpu_texture handle) {
    vk_texture* texture = (vk_texture*)handle;
    VGPU_FREE(texture);
}

static uint32_t vk_texture_get_width(vgpu_texture handle, uint32_t mipLevel) {
    vk_texture* texture = (vk_texture*)handle;
    return _vgpu_max(1, texture->width >> mipLevel);
}

static uint32_t vk_texture_get_height(vgpu_texture handle, uint32_t mipLevel) {
    vk_texture* texture = (vk_texture*)handle;
    return _vgpu_max(1, texture->height >> mipLevel);
}

/* Framebuffer */
static vgpu_framebuffer vk_framebuffer_create(const vgpu_framebuffer_info* info) {
    vk_framebuffer* framebuffer = VGPU_ALLOC_HANDLE(vk_framebuffer);
    return (vgpu_framebuffer)framebuffer;
}

static void vk_framebuffer_destroy(vgpu_framebuffer handle) {
    vk_framebuffer* framebuffer = (vk_framebuffer*)handle;
    VGPU_FREE(framebuffer);
}

/* Swapchain */
static vgpu_swapchain vk_swapchain_create(uintptr_t window_handle, vgpu_texture_format color_format, vgpu_texture_format depth_stencil_format) {
    vk_swapchain* swapchain = VGPU_ALLOC_HANDLE(vk_swapchain);
    return (vgpu_swapchain)swapchain;
}

static void vk_swapchain_destroy(vgpu_swapchain handle) {
    vk_swapchain* swapchain = (vk_swapchain*)handle;
    VGPU_FREE(swapchain);
}

static void vk_swapchain_resize(vgpu_swapchain handle, uint32_t width, uint32_t height) {
    vk_swapchain* swapchain = (vk_swapchain*)handle;
}

static void vk_swapchain_present(vgpu_swapchain handle) {
    vk_swapchain* swapchain = (vk_swapchain*)handle;
}

/* Helper functions */
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

static VkBool32 VKAPI_CALL debug_utils_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT flags, const VkDebugUtilsMessengerCallbackDataEXT* data, void* context) {
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        vgpu_log(VGPU_LOG_LEVEL_WARN, "%d - %s: %s", data->messageIdNumber, data->pMessageIdName, data->pMessage);
    }
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        vgpu_log(VGPU_LOG_LEVEL_ERROR, "%d - %s: %s", data->messageIdNumber, data->pMessageIdName, data->pMessage);
    }
    return VK_FALSE;
}

static bool _vgpu_vk_create_surface(uintptr_t native_handle, VkSurfaceKHR* pSurface) {
    VkResult result = VK_SUCCESS;

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    HWND hwnd = (HWND)native_handle;

    const VkWin32SurfaceCreateInfoKHR surface_info = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = GetModuleHandle(NULL),
        .hwnd = hwnd
    };
    result = vkCreateWin32SurfaceKHR(vk.instance, &surface_info, NULL, pSurface);
#endif
    
    if (result != VK_SUCCESS) {
        vgpu_log(VGPU_LOG_LEVEL_ERROR, "Failed to create surface");
        return false;
    }

    return true;
}

static bool _vgpu_vk_is_device_suitable(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physical_device, &props);

    /* We run on vulkan 1.1 or higher. */
    if (props.apiVersion < VK_API_VERSION_1_1) {
        return false;
    }

    vk_queue_family_indices indices = _vgpu_vk_query_queue_families(physical_device, surface);

    if (indices.graphics_queue_family == VK_QUEUE_FAMILY_IGNORED)
        return false;

    vk_physical_device_features features = _vgpu_vk_query_device_extensions(physical_device);
    if (surface != VK_NULL_HANDLE && !features.swapchain) {
        return false;
    }

    /* We required maintenance_1 to support viewport flipping to match DX style. */
    if (!features.maintenance_1) {
        return false;
    }

    return true;
}

static bool _vgpu_vk_query_presentation_support(VkPhysicalDevice physical_device, uint32_t queue_family_index) {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    return vkGetPhysicalDeviceWin32PresentationSupportKHR(physical_device, queue_family_index);
#elif defined(__ANDROID__)
    return true;
#else
    /* TODO: */
    return true;
#endif
}

static vk_queue_family_indices _vgpu_vk_query_queue_families(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    uint32_t queue_count = 0u;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_count, NULL);
    VkQueueFamilyProperties* queue_families = VGPU_ALLOCA(VkQueueFamilyProperties, queue_count);
    assert(queue_families);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_count, queue_families);

    vk_queue_family_indices result = {
        .graphics_queue_family = VK_QUEUE_FAMILY_IGNORED,
        .compute_queue_family = VK_QUEUE_FAMILY_IGNORED,
        .transfer_queue_family = VK_QUEUE_FAMILY_IGNORED
    };

    for (uint32_t i = 0; i < queue_count; i++)
    {
        VkBool32 present_support = VK_TRUE;
        if (surface != VK_NULL_HANDLE) {
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
        }
        else {
            present_support = _vgpu_vk_query_presentation_support(physical_device, i);
        }

        static const VkQueueFlags required = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT;
        if (present_support && ((queue_families[i].queueFlags & required) == required))
        {
            result.graphics_queue_family = i;
            break;
        }
    }

    /* Dedicated compute queue. */
    for (uint32_t i = 0; i < queue_count; i++)
    {
        static const VkQueueFlags required = VK_QUEUE_COMPUTE_BIT;
        if (i != result.graphics_queue_family &&
            (queue_families[i].queueFlags & required) == required)
        {
            result.compute_queue_family = i;
            break;
        }
    }

    /* Dedicated transfer queue. */
    for (uint32_t i = 0; i < queue_count; i++)
    {
        static const VkQueueFlags required = VK_QUEUE_TRANSFER_BIT;
        if (i != result.graphics_queue_family &&
            i != result.compute_queue_family &&
            (queue_families[i].queueFlags & required) == required)
        {
            result.transfer_queue_family = i;
            break;
        }
    }

    if (result.transfer_queue_family == VK_QUEUE_FAMILY_IGNORED)
    {
        for (uint32_t i = 0; i < queue_count; i++)
        {
            static const VkQueueFlags required = VK_QUEUE_TRANSFER_BIT;
            if (i != result.graphics_queue_family &&
                (queue_families[i].queueFlags & required) == required)
            {
                result.transfer_queue_family = i;
                break;
            }
        }
    }

    return result;
}

static vk_physical_device_features _vgpu_vk_query_device_extensions(VkPhysicalDevice physical_device) {
    uint32_t ext_count = 0;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, NULL, &ext_count, NULL));

    VkExtensionProperties* available_extensions = VGPU_ALLOCA(VkExtensionProperties, ext_count);
    assert(available_extensions);
    VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, NULL, &ext_count, available_extensions));

    vk_physical_device_features result;
    memset(&result, 0, sizeof(result));
    for (uint32_t i = 0; i < ext_count; ++i) {
        if (strcmp(available_extensions[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            result.swapchain = true;
        }
        else if (strcmp(available_extensions[i].extensionName, VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME) == 0) {
            result.depth_clip_enable = true;
        }
        else if (strcmp(available_extensions[i].extensionName, VK_KHR_MAINTENANCE1_EXTENSION_NAME) == 0) {
            result.maintenance_1 = true;
        }
        else if (strcmp(available_extensions[i].extensionName, VK_KHR_MAINTENANCE2_EXTENSION_NAME) == 0) {
            result.maintenance_2 = true;
        }
        else if (strcmp(available_extensions[i].extensionName, VK_KHR_MAINTENANCE3_EXTENSION_NAME) == 0) {
            result.maintenance_3 = true;
        }
        else if (strcmp(available_extensions[i].extensionName, VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME) == 0) {
            result.get_memory_requirements2 = true;
        }
        else if (strcmp(available_extensions[i].extensionName, VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME) == 0) {
            result.dedicated_allocation = true;
        }
        else if (strcmp(available_extensions[i].extensionName, VK_KHR_BIND_MEMORY_2_EXTENSION_NAME) == 0) {
            result.bind_memory2 = true;
        }
        else if (strcmp(available_extensions[i].extensionName, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME) == 0) {
            result.memory_budget = true;
        }
        else if (strcmp(available_extensions[i].extensionName, VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME) == 0) {
            result.image_format_list = true;
        }
        else if (strcmp(available_extensions[i].extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0) {
            result.debug_marker = true;
        }
        else if (strcmp(available_extensions[i].extensionName, VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME) == 0) {
            result.win32_full_screen_exclusive = true;
        }
        else if (strcmp(available_extensions[i].extensionName, VK_KHR_RAY_TRACING_EXTENSION_NAME) == 0) {
            result.raytracing = true;
        }
        else if (strcmp(available_extensions[i].extensionName, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) == 0) {
            result.buffer_device_address = true;
        }
        else if (strcmp(available_extensions[i].extensionName, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME) == 0) {
            result.deferred_host_operations = true;
        }
        else if (strcmp(available_extensions[i].extensionName, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME) == 0) {
            result.descriptor_indexing = true;
        }
        else if (strcmp(available_extensions[i].extensionName, VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME) == 0) {
            result.pipeline_library = true;
        }
        else if (strcmp(available_extensions[i].extensionName, VK_KHR_MULTIVIEW_EXTENSION_NAME) == 0) {
            result.multiview = true;
        }
    }

    return result;
}

/* Driver functions */
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

    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress((HMODULE)vk.library, "vkGetInstanceProcAddr");
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

    GPU_FOREACH_ANONYMOUS(GPU_LOAD_ANONYMOUS);

    // We reuire vulkan 1.1.0 or higher API
    VkInstanceCreateInfo instance_info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &(VkApplicationInfo) {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = VK_API_VERSION_1_1
      },
    };

    VkInstance instance;
    VkResult result = vkCreateInstance(&instance_info, NULL, &instance);
    if (result != VK_SUCCESS) {
        return false;
    }

    vkDestroyInstance = (PFN_vkDestroyInstance)vkGetInstanceProcAddr(instance, "vkDestroyInstance");
    vkDestroyInstance(instance, NULL);
    vk.available = true;
    return true;
}

static vgpu_context* vulkan_create_context(void) {
    static vgpu_context context = { 0 };
    ASSIGN_DRIVER(vk);
    return &context;
}

vgpu_driver vulkan_driver = {
    VGPU_BACKEND_TYPE_VULKAN,
    vulkan_is_supported,
    vulkan_create_context
};

#endif /* defined(VGPU_DRIVER_VULKAN) */
