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

#include "agpu.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#if defined(_WIN32)
#   include <malloc.h>
#   if defined(alloca)
#       undef alloca
#   endif
#   define alloca _malloca
#else
#   include <alloca.h>
#endif

#include "vk/vk.h"
#include "vk/vk_mem_alloc.h"

#ifndef __cplusplus
#  define nullptr ((void*)0)
#endif

#define gpu_alloca(type)     ((type*) alloca(sizeof(type)))
#define gpu_allocan(type, count) ((type*) alloca(sizeof(type) * count))

#define _gpu_def(val, def) (((val) == 0) ? (def) : (val))
#define _gpu_min(a,b) ((a<b)?a:b)
#define _gpu_max(a,b) ((a>b)?a:b)
#define GPU_UNUSED(x) do { (void)sizeof(x); } while(0)
#define GPU_THROW(s) agpu_log(s, AGPU_LOG_LEVEL_ERROR);
#define GPU_CHECK(c, s) if (!(c)) { GPU_THROW(s); }
#define VK_CHECK(res) do { VkResult r = (res); GPU_CHECK(r >= 0, vk_get_error_string(r)); } while (0)

#if !defined(NDEBUG) || defined(DEBUG) || defined(_DEBUG)
#   define VULKAN_DEBUG 
#endif

#define _GPU_MAX_PHYSICAL_DEVICES (32u)

typedef struct {
    bool debug_utils;
    bool headless;
    bool surface_capabilities2;
    bool physical_device_properties2;
    bool external_memory_capabilities;
    bool external_semaphore_capabilities;
} vk_features;

typedef struct {
    bool swapchain;
    bool maintenance_1;
    bool maintenance_2;
    bool maintenance_3;
} vk_physical_device_features;

typedef struct {
    uint32_t graphics_queue_family;
    uint32_t compute_queue_family;
    uint32_t copy_queue_family;
} vk_queue_family_indices;

static struct {
    agpu_desc desc;
    bool headless;
    bool validation;
    uint32_t max_inflight_frames;

    vk_features vk_features;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_utils_messenger;
    VkDebugReportCallbackEXT debug_report_callback;

    VkPhysicalDevice physical_device;
    vk_queue_family_indices queue_families;
    vk_physical_device_features device_features;

    VkDevice device;
    VkQueue graphics_queue;
    VkQueue compute_queue;
    VkQueue copy_queue;
    VmaAllocator memory_allocator;
} state;

struct gpu_swapchain {
    VkSurfaceKHR surface;
    VkSwapchainKHR handle;
};

static const char* vk_get_error_string(VkResult result);
static bool _agpu_query_presentation_support(VkPhysicalDevice physical_device, uint32_t queue_family_index);
static vk_queue_family_indices _agpu_query_queue_families(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
static vk_physical_device_features _agpu_query_device_extension_support(VkPhysicalDevice physical_device);
static bool _agpu_is_device_suitable(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

#if defined(VULKAN_DEBUG)
VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
{
    // Log debug messge
    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        agpu_log_format(AGPU_LOG_LEVEL_WARNING, "%u - %s: %s", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
    }
    else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        agpu_log_format(AGPU_LOG_LEVEL_ERROR, "%u - %s: %s", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
    }

    return VK_FALSE;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT type,
    uint64_t object,
    size_t location,
    int32_t message_code,
    const char* layer_prefix,
    const char* message,
    void* user_data)
{
    GPU_UNUSED(type);
    GPU_UNUSED(object);
    GPU_UNUSED(location);
    GPU_UNUSED(message_code);
    GPU_UNUSED(user_data);
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        agpu_log_format(AGPU_LOG_LEVEL_ERROR, "%s: %s", layer_prefix, message);
    }
    else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
    {
        agpu_log_format(AGPU_LOG_LEVEL_WARNING, "%s: %s", layer_prefix, message);
    }
    else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
    {
        agpu_log_format(AGPU_LOG_LEVEL_WARNING, "%s: %s", layer_prefix, message);
    }
    else
    {
        agpu_log_format(AGPU_LOG_LEVEL_INFO, "%s: %s", layer_prefix, message);
    }
    return VK_FALSE;
}
#endif

bool agpu_init(const char* application_name, const agpu_desc* desc) {
    // Copy settings
    memcpy(&state.desc, desc, sizeof(*desc));

    if (desc->flags & AGPU_CONFIG_FLAGS_HEADLESS) {
        state.headless = true;
    }

#if defined(VULKAN_DEBUG)
    if (desc->flags & AGPU_CONFIG_FLAGS_VALIDATION) {
        state.validation = true;
    }
#endif

    if (!agpu_vk_init_loader()) {
        return false;
    }

    state.max_inflight_frames = _gpu_min(_gpu_def(desc->max_inflight_frames, 2u), 2u);

    uint32_t instance_extension_count;
    VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, NULL));

    VkExtensionProperties* available_instance_extensions = gpu_allocan(VkExtensionProperties, instance_extension_count);
    VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, available_instance_extensions));

    uint32_t enabled_extension_count = 0;
    char* enabled_extensions[64];

    for (uint32_t i = 0; i < instance_extension_count; ++i) {
        if (strcmp(available_instance_extensions[i].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
            state.vk_features.debug_utils = true;
            enabled_extensions[enabled_extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        }
        else if (strcmp(available_instance_extensions[i].extensionName, VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME) == 0)
        {
            state.vk_features.headless = true;
        }
        else if (strcmp(available_instance_extensions[i].extensionName, VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME) == 0)
        {
            state.vk_features.surface_capabilities2 = true;
        }
        else if (strcmp(available_instance_extensions[i].extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0)
        {
            state.vk_features.physical_device_properties2 = true;
            enabled_extensions[enabled_extension_count++] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
        }
        else if (strcmp(available_instance_extensions[i].extensionName, VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME) == 0)
        {
            state.vk_features.external_memory_capabilities = true;
            enabled_extensions[enabled_extension_count++] = VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME;
        }
        else if (strcmp(available_instance_extensions[i].extensionName, VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME) == 0)
        {
            state.vk_features.external_semaphore_capabilities = true;
            enabled_extensions[enabled_extension_count++] = VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME;
        }
    }

    if (state.headless) {
        if (!state.vk_features.headless) {

        }
        else
        {
            enabled_extensions[enabled_extension_count++] = VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME;
        }
    }
    else {
        enabled_extensions[enabled_extension_count++] = VK_KHR_SURFACE_EXTENSION_NAME;

#if defined(_WIN32)
        enabled_extensions[enabled_extension_count++] = "VK_KHR_win32_surface";
#endif

        if (state.vk_features.surface_capabilities2) {
            enabled_extensions[enabled_extension_count++] = VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME;
        }
    }

    uint32_t enabled_instance_layers_count = 0;
    const char* enabled_instance_layers[8];

#if defined(VULKAN_DEBUG)
    if (state.validation) {
        uint32_t instance_layer_count;
        VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr));

        VkLayerProperties* supported_validation_layers = gpu_allocan(VkLayerProperties, instance_layer_count);
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
#endif

    VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &(VkApplicationInfo) {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = application_name,
            .applicationVersion = 0,
            .pEngineName = "alimer",
            .engineVersion = 0,
            .apiVersion = agpu_vk_get_instance_version()
        },
        .enabledLayerCount = enabled_instance_layers_count,
        .ppEnabledLayerNames = enabled_instance_layers,
        .enabledExtensionCount = enabled_extension_count,
        .ppEnabledExtensionNames = enabled_extensions
    };

#if defined(VULKAN_DEBUG)
    VkDebugUtilsMessengerCreateInfoEXT debug_utils_create_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    VkDebugReportCallbackCreateInfoEXT debug_report_create_info = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT };
    if (state.vk_features.debug_utils)
    {
        debug_utils_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        debug_utils_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        debug_utils_create_info.pfnUserCallback = debug_utils_messenger_callback;

        instance_info.pNext = &debug_utils_create_info;
    }
    else
    {
        debug_report_create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        debug_report_create_info.pfnCallback = debug_callback;

        instance_info.pNext = &debug_report_create_info;
    }
#endif

    VkResult result = vkCreateInstance(&instance_info, NULL, &state.instance);
    if (result != VK_SUCCESS) {
        agpu_shutdown();
        return false;
    }

    agpu_vk_init_instance(state.instance);

#if defined(VULKAN_DEBUG)
    if (state.vk_features.debug_utils)
    {
        result = vkCreateDebugUtilsMessengerEXT(state.instance, &debug_utils_create_info, NULL, &state.debug_utils_messenger);
        if (result != VK_SUCCESS)
        {
            GPU_THROW("Could not create debug utils messenger");
            agpu_shutdown();
            return false;
        }
    }
    else
    {
        result = vkCreateDebugReportCallbackEXT(state.instance, &debug_report_create_info, NULL, &state.debug_report_callback);
        if (result != VK_SUCCESS) {
            GPU_THROW("Could not create debug report callback");
            agpu_shutdown();
            return false;
        }
    }
#endif

    VkSurfaceKHR surface = VK_NULL_HANDLE;

    /* Detect physical device. */
    uint32_t physical_device_count = _GPU_MAX_PHYSICAL_DEVICES;
    VkPhysicalDevice physical_devices[_GPU_MAX_PHYSICAL_DEVICES];
    result = vkEnumeratePhysicalDevices(state.instance, &physical_device_count, physical_devices);
    if (result != VK_SUCCESS) {
        GPU_THROW("Cannot enumerate physical devices.");
        agpu_shutdown();
        return false;
    }

    uint32_t best_device_score = 0U;
    uint32_t best_device_index = VK_QUEUE_FAMILY_IGNORED;
    for (uint32_t i = 0; i < physical_device_count; ++i)
    {
        if (!_agpu_is_device_suitable(physical_devices[i], surface)) {
            continue;
        }

        VkPhysicalDeviceProperties physical_device_props;
        vkGetPhysicalDeviceProperties(physical_devices[i], &physical_device_props);

        uint32_t score = 0u;

        if (physical_device_props.apiVersion >= VK_API_VERSION_1_2) {
            score += 10000u;
        }
        else if (physical_device_props.apiVersion >= VK_API_VERSION_1_1) {
            score += 5000u;
        }

        switch (physical_device_props.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            score += 100U;
            if (desc->preferred_adapter == AGPU_ADAPTER_TYPE_DISCRETE_GPU) {
                score += 1000u;
            }
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            score += 90U;
            if (desc->preferred_adapter == AGPU_ADAPTER_TYPE_INTEGRATED_GPU) {
                score += 1000u;
            }
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            score += 80U;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            score += 70U;
            if (desc->preferred_adapter == AGPU_ADAPTER_TYPE_CPU) {
                score += 1000u;
            }
            break;
        default: score += 10U;
        }
        if (score > best_device_score) {
            best_device_index = i;
            best_device_score = score;
        }
    }

    if (best_device_index == VK_QUEUE_FAMILY_IGNORED) {
        GPU_THROW("Cannot find suitable physical device.");
        agpu_shutdown();
        return false;
    }
    state.physical_device = physical_devices[best_device_index];
    state.queue_families = _agpu_query_queue_families(state.physical_device, surface);
    state.device_features = _agpu_query_device_extension_support(state.physical_device);

    /* Setup device queue's. */
    uint32_t queue_count = 0u;
    vkGetPhysicalDeviceQueueFamilyProperties(state.physical_device, &queue_count, NULL);
    VkQueueFamilyProperties* queue_families = gpu_allocan(VkQueueFamilyProperties, queue_count);
    assert(queue_families);
    vkGetPhysicalDeviceQueueFamilyProperties(state.physical_device, &queue_count, queue_families);

    uint32_t universal_queue_index = 1;
    uint32_t graphics_queue_index = 0;
    uint32_t compute_queue_index = 0;
    uint32_t copy_queue_index = 0;

    if (state.queue_families.compute_queue_family == VK_QUEUE_FAMILY_IGNORED)
    {
        state.queue_families.compute_queue_family = state.queue_families.graphics_queue_family;
        compute_queue_index = _gpu_min(queue_families[state.queue_families.graphics_queue_family].queueCount - 1, universal_queue_index);
        universal_queue_index++;
    }

    if (state.queue_families.copy_queue_family == VK_QUEUE_FAMILY_IGNORED)
    {
        state.queue_families.copy_queue_family = state.queue_families.graphics_queue_family;
        copy_queue_index = _gpu_min(queue_families[state.queue_families.graphics_queue_family].queueCount - 1, universal_queue_index);
        universal_queue_index++;
    }
    else if (state.queue_families.copy_queue_family == state.queue_families.compute_queue_family)
    {
        copy_queue_index = _gpu_min(queue_families[state.queue_families.compute_queue_family].queueCount - 1, 1u);
    }

    static const float graphics_queue_prio = 0.5f;
    static const float compute_queue_prio = 1.0f;
    static const float transfer_queue_prio = 1.0f;
    float prio[3] = { graphics_queue_prio, compute_queue_prio, transfer_queue_prio };

    uint32_t queue_family_count = 0;
    VkDeviceQueueCreateInfo queue_info[3] = {0};
    queue_info[queue_family_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info[queue_family_count].queueFamilyIndex = state.queue_families.graphics_queue_family;
    queue_info[queue_family_count].queueCount = _gpu_min(universal_queue_index, queue_families[state.queue_families.graphics_queue_family].queueCount);
    queue_info[queue_family_count].pQueuePriorities = prio;
    queue_family_count++;

    if (state.queue_families.compute_queue_family != state.queue_families.graphics_queue_family)
    {
        queue_info[queue_family_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[queue_family_count].queueFamilyIndex = state.queue_families.compute_queue_family;
        queue_info[queue_family_count].queueCount = _gpu_min(state.queue_families.copy_queue_family == state.queue_families.compute_queue_family ? 2u : 1u,
            queue_families[state.queue_families.compute_queue_family].queueCount);
        queue_info[queue_family_count].pQueuePriorities = prio + 1;
        queue_family_count++;
    }

    if (state.queue_families.copy_queue_family != state.queue_families.graphics_queue_family &&
        state.queue_families.copy_queue_family != state.queue_families.compute_queue_family)
    {
        queue_info[queue_family_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[queue_family_count].queueFamilyIndex = state.queue_families.copy_queue_family;
        queue_info[queue_family_count].queueCount = 1;
        queue_info[queue_family_count].pQueuePriorities = prio + 2;
        queue_family_count++;
    }

    /* Setup device extensions now. */
    enabled_extension_count = 0;
    memset(&enabled_extensions, 0, sizeof(*enabled_extensions));
    if (!state.headless) {
        enabled_extensions[enabled_extension_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    }

    if (state.device_features.maintenance_1) {
        enabled_extensions[enabled_extension_count++] = VK_KHR_MAINTENANCE1_EXTENSION_NAME;
    }

    if (state.device_features.maintenance_2) {
        enabled_extensions[enabled_extension_count++] = VK_KHR_MAINTENANCE2_EXTENSION_NAME;
    }

    if (state.device_features.maintenance_3) {
        enabled_extensions[enabled_extension_count++] = VK_KHR_MAINTENANCE3_EXTENSION_NAME;
    }

    VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = queue_family_count,
        .pQueueCreateInfos = queue_info,
        .enabledExtensionCount = enabled_extension_count,
        .ppEnabledExtensionNames = enabled_extensions
    };

    if (vkCreateDevice(state.physical_device, &device_info, NULL, &state.device)) {
        agpu_shutdown();
        return false;
    }

    agpu_vk_init_device(state.device);
    vkGetDeviceQueue(state.device, state.queue_families.graphics_queue_family, graphics_queue_index, &state.graphics_queue);
    vkGetDeviceQueue(state.device, state.queue_families.compute_queue_family, compute_queue_index, &state.compute_queue);
    vkGetDeviceQueue(state.device, state.queue_families.copy_queue_family, copy_queue_index, &state.copy_queue);

    return true;
}

void agpu_shutdown(void) {
    if (state.device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(state.device);
    }

    /* TODO: Release data. */

    if (state.device != VK_NULL_HANDLE) {
        vkDestroyDevice(state.device, NULL);
    }

    if (state.debug_utils_messenger != VK_NULL_HANDLE) {
        vkDestroyDebugUtilsMessengerEXT(state.instance, state.debug_utils_messenger, NULL);
    }

    if (state.debug_report_callback != VK_NULL_HANDLE) {
        vkDestroyDebugReportCallbackEXT(state.instance, state.debug_report_callback, NULL);
    }

    if (state.instance != VK_NULL_HANDLE) {
        vkDestroyInstance(state.instance, NULL);
    }

    memset(&state, 0, sizeof(state));
}

void agpu_wait_idle(void) {
    vkDeviceWaitIdle(state.device);
}

const char* vk_get_error_string(VkResult result) {
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

static bool _agpu_query_presentation_support(VkPhysicalDevice physical_device, uint32_t queue_family_index) {
#if defined(_WIN32) 
    return vkGetPhysicalDeviceWin32PresentationSupportKHR(physical_device, queue_family_index);
#elif defined(__ANDROID__)
    return true;
#else
    /* TODO: */
    return true;
#endif
}

static vk_queue_family_indices _agpu_query_queue_families(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    uint32_t queue_count = 0u;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_count, NULL);
    VkQueueFamilyProperties* queue_families = gpu_allocan(VkQueueFamilyProperties, queue_count);
    assert(queue_families);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_count, queue_families);

    vk_queue_family_indices result;
    result.graphics_queue_family = VK_QUEUE_FAMILY_IGNORED;
    result.compute_queue_family = VK_QUEUE_FAMILY_IGNORED;
    result.copy_queue_family = VK_QUEUE_FAMILY_IGNORED;

    for (uint32_t i = 0; i < queue_count; i++)
    {
        VkBool32 present_support = VK_TRUE;
        if (surface != VK_NULL_HANDLE) {
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
        }
        else {
            present_support = _agpu_query_presentation_support(physical_device, i);
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
            result.copy_queue_family = i;
            break;
        }
    }

    if (result.copy_queue_family == VK_QUEUE_FAMILY_IGNORED)
    {
        for (uint32_t i = 0; i < queue_count; i++)
        {
            static const VkQueueFlags required = VK_QUEUE_TRANSFER_BIT;
            if (i != result.graphics_queue_family &&
                (queue_families[i].queueFlags & required) == required)
            {
                result.copy_queue_family = i;
                break;
            }
        }
    }

    return result;
}

vk_physical_device_features _agpu_query_device_extension_support(VkPhysicalDevice physical_device) {
    uint32_t ext_count = 0;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, NULL, &ext_count, nullptr));

    VkExtensionProperties* available_extensions = gpu_allocan(VkExtensionProperties, ext_count);
    assert(available_extensions);
    VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &ext_count, available_extensions));

    vk_physical_device_features result;
    memset(&result, 0, sizeof(result));
    for (uint32_t i = 0; i < ext_count; ++i) {
        if (strcmp(available_extensions[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            result.swapchain = true;
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
    }

    return result;
}

static bool _agpu_is_device_suitable(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    vk_queue_family_indices indices = _agpu_query_queue_families(physical_device, surface);

    if (indices.graphics_queue_family == VK_QUEUE_FAMILY_IGNORED)
        return false;

    vk_physical_device_features features =  _agpu_query_device_extension_support(physical_device);
    if (!state.headless && !features.swapchain) {
        return false;
    }

    return true;
}
