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

#define gpu_alloca(type)     ((type*) alloca(sizeof(type)))
#define gpu_allocan(type, count) ((type*) alloca(sizeof(type) * count))

#define gpu_def(val, def) (((val) == 0) ? (def) : (val))
#define gpu_min(a,b) ((a<b)?a:b)
#define gpu_max(a,b) ((a>b)?a:b)
#define GPU_THROW(s) if (state.desc.callback) { state.desc.callback(state.desc.context, s, AGPU_LOG_LEVEL_ERROR); }
#define GPU_CHECK(c, s) if (!(c)) { GPU_THROW(s); }
#define VK_CHECK(res) do { VkResult r = (res); GPU_CHECK(r >= 0, vk_get_error_string(r)); } while (0)

#if !defined(NDEBUG) || defined(DEBUG) || defined(_DEBUG)
#   define VULKAN_DEBUG 
#endif

typedef struct {
    bool debug_utils;
    bool headless;
    bool surface_capabilities2;
    bool physical_device_properties2;
    bool external_memory_capabilities;
    bool external_semaphore_capabilities;
} vk_features;

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
    VkDevice device;
    VmaAllocator memory_allocator;
} state;

static const char* vk_get_error_string(VkResult result);

#if defined(VULKAN_DEBUG)
VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
{
    // Log debug messge
    /*if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        LOGW("{} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
    }
    else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        LOGE("{} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
    }*/

    return VK_FALSE;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT /*type*/,
    uint64_t /*object*/,
    size_t /*location*/,
    int32_t /*message_code*/,
    const char* layer_prefix,
    const char* message,
    void* /*user_data*/)
{
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        //agpu_log("{}: {}", layer_prefix, message);
    }
    else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
    {
        //LOGW("{}: {}", layer_prefix, message);
    }
    else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
    {
        //LOGW("{}: {}", layer_prefix, message);
    }
    else
    {
        //LOGI("{}: {}", layer_prefix, message);
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

    state.max_inflight_frames = gpu_min(gpu_def(desc->max_inflight_frames, 2u), 2u);

    uint32_t instance_extension_count;
    VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, NULL));

    VkExtensionProperties* available_instance_extensions = gpu_allocan(VkExtensionProperties, instance_extension_count);
    VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, available_instance_extensions));

    uint32_t enabled_extension_count = 0;
    const char* enabled_extensions[32];

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
        //.enabledLayerCount = state.config.debug ? COUNTOF(layers) : 0,
        //.ppEnabledLayerNames = layers,
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

    if (vkCreateInstance(&instance_info, NULL, &state.instance)) {
        agpu_shutdown();
        return false;
    }

    agpu_vk_init_instance(state.instance);

    return true;
}

void agpu_shutdown(void) {
    /*if (state.device) {
        vkDeviceWaitIdle(state.device);
    }
    */

    if (state.device) {
        vkDestroyDevice(state.device, NULL);
    }

    if (state.debug_utils_messenger != VK_NULL_HANDLE) {
        vkDestroyDebugUtilsMessengerEXT(state.instance, state.debug_utils_messenger, NULL);
    }

    if (state.debug_report_callback != VK_NULL_HANDLE) {
        vkDestroyDebugReportCallbackEXT(state.instance, state.debug_report_callback, NULL);
    }

    if (state.instance) vkDestroyInstance(state.instance, NULL);
    memset(&state, 0, sizeof(state));
}

void agpu_wait_idle(void) {

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
