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

#include "agpu_backend.h"

#include "vk/vk.h"
#include "vk/vk_mem_alloc.h"

#define VK_CHECK(res) do { VkResult r = (res); GPU_CHECK(r >= 0, vk_get_error_string(r)); } while (0)

#if !defined(NDEBUG) || defined(DEBUG) || defined(_DEBUG)
#   define VULKAN_DEBUG 
#endif

#define _GPU_MAX_PHYSICAL_DEVICES (32u)

typedef struct {
    bool swapchain;
    bool maintenance_1;
    bool maintenance_2;
    bool maintenance_3;
    bool get_memory_requirements2;
    bool dedicated_allocation;
    bool image_format_list;
    bool debug_marker;
    bool full_screen_exclusive;
} vk_physical_device_features;

typedef struct {
    uint32_t graphics_queue_family;
    uint32_t compute_queue_family;
    uint32_t copy_queue_family;
} vk_queue_family_indices;

typedef struct agpu_vk_context {
    VkSurfaceKHR surface;
    uint32_t width;
    uint32_t height;
    uint32_t image_count;
    VkSwapchainKHR handle;
} agpu_vk_context;

typedef struct agpu_vk_buffer {
    VkBuffer handle;
    VmaAllocation allocation;
} agpu_vk_buffer;

typedef struct agpu_vk_renderer {
    /* Associated device */
    agpu_device* gpu_device;

    uint32_t max_inflight_frames;

    VkPhysicalDevice physical_device;
    vk_queue_family_indices queue_families;

    bool api_version_12;
    bool api_version_11;
    vk_physical_device_features device_features;

    agpu_features features;
    agpu_limits limits;

    VkDevice device;
    VkQueue graphics_queue;
    VkQueue compute_queue;
    VkQueue copy_queue;
    VmaAllocator memory_allocator;

} agpu_vk_renderer;

/* Global vulkan data. */
static struct {
    bool apiVersion12;
    bool apiVersion11;
    bool debug_utils;
    bool headless;
    bool surface_capabilities2;
    bool physical_device_properties2;
    bool external_memory_capabilities;
    bool external_semaphore_capabilities;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_utils_messenger;
    VkDebugReportCallbackEXT debug_report_callback;

    uint32_t physical_device_count;
    VkPhysicalDevice physical_devices[_GPU_MAX_PHYSICAL_DEVICES];

    /* Number of logical gpu device created. */
    uint32_t device_count;
} vk;

static const char* vk_get_error_string(VkResult result);
static bool _agpu_query_presentation_support(VkPhysicalDevice physical_device, uint32_t queue_family_index);
static vk_queue_family_indices _agpu_query_queue_families(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
static vk_physical_device_features _agpu_query_device_extension_support(VkPhysicalDevice physical_device);
static bool _agpu_is_device_suitable(VkPhysicalDevice physical_device, VkSurfaceKHR surface, bool headless);
static VkSurfaceKHR vk_create_surface(uintptr_t native_handle, uint32_t* width, uint32_t* height);

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

static void vk_destroy_device(agpu_device* device) {
    agpu_vk_renderer* renderer = (agpu_vk_renderer*)device->renderer;

    if (renderer->device != VK_NULL_HANDLE) {
        agpu_wait_idle(device);
    }

    /* TODO: Release data. */

    if (renderer->device != VK_NULL_HANDLE) {
        vkDestroyDevice(renderer->device, NULL);
    }

    vk.device_count--;

    if (vk.device_count == 0)
    {
        if (vk.debug_utils_messenger != VK_NULL_HANDLE)
            vkDestroyDebugUtilsMessengerEXT(vk.instance, vk.debug_utils_messenger, NULL);
        else if (vk.debug_report_callback != VK_NULL_HANDLE)
            vkDestroyDebugReportCallbackEXT(vk.instance, vk.debug_report_callback, NULL);

        if (vk.instance != VK_NULL_HANDLE)
            vkDestroyInstance(vk.instance, NULL);

        vk.instance = VK_NULL_HANDLE;
    }

    AGPU_FREE(renderer);
    AGPU_FREE(device);
}

static void vk_wait_idle(agpu_renderer* renderer) {
    agpu_vk_renderer* vk_renderer = (agpu_vk_renderer*)renderer;
    vkDeviceWaitIdle(vk_renderer->device);
}


static void vk_begin_frame(agpu_renderer* renderer) {
    agpu_vk_renderer* vk_renderer = (agpu_vk_renderer*)renderer;
}

static void vk_end_frame(agpu_renderer* renderer) {
    agpu_vk_renderer* vk_renderer = (agpu_vk_renderer*)renderer;
}

static VkSurfaceKHR vk_create_surface(uintptr_t native_handle, uint32_t* width, uint32_t* height) {
    VkResult result = VK_SUCCESS;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

#if defined(_WIN32)
    HWND window = (HWND)native_handle;

    const VkWin32SurfaceCreateInfoKHR surface_info = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .hinstance = GetModuleHandle(NULL),
        .hwnd = window
    };
    result = vkCreateWin32SurfaceKHR(vk.instance, &surface_info, NULL, &surface);
    if (result != VK_SUCCESS) {
        GPU_THROW("Failed to create surface");
    }

    RECT rect;
    BOOL success = GetClientRect(window, &rect);
    GPU_CHECK(success, "GetWindowRect error.");
    *width = rect.right - rect.left;
    *height = rect.bottom - rect.top;
#endif

    return surface;
}

static bool vk_init_or_update_context(agpu_vk_renderer* renderer, agpu_context* context) {
    agpu_vk_context* vk_context = (agpu_vk_context*)context;

    VkSurfaceCapabilitiesKHR surface_caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(renderer->physical_device, vk_context->surface, &surface_caps);

    VkSwapchainKHR old_swapchain = vk_context->handle;
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;

    /* Detect image count. */
    uint32_t image_count = vk_context->image_count;
    if (surface_caps.maxImageCount != 0)
    {
        image_count = _gpu_min(image_count, surface_caps.maxImageCount);
    }
    image_count = _gpu_max(image_count, surface_caps.minImageCount);

    VkExtent2D swapchain_size = { vk_context->width, vk_context->height };
    
    const VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .surface = vk_context->surface,
        .minImageCount = image_count,
        //.imageFormat = requested_format,
        //.imageColorSpace = color_space,
        .imageExtent = swapchain_size,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        //.imageSharingMode = sharing_mode,
        //.queueFamilyIndexCount = num_sharing_queue_families,
        //.pQueueFamilyIndices = sharing_queue_families,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = old_swapchain
    };

    VkResult result = vkCreateSwapchainKHR(renderer->device, &create_info, NULL, &vk_context->handle);
    if (result != VK_SUCCESS) {
        /* TODO: Call destroy */
        return false;
    }

    return true;
}

static agpu_context* vk_create_context(agpu_renderer* renderer, const agpu_swapchain_desc* desc) {
    agpu_vk_renderer* vk_renderer = (agpu_vk_renderer*)renderer;
    agpu_vk_context* context = (agpu_vk_context*)AGPU_MALLOC(sizeof(agpu_vk_context));

    context->surface = vk_create_surface(desc->native_handle, &context->width, &context->height);
    context->handle = VK_NULL_HANDLE;

    if (!vk_init_or_update_context(vk_renderer, (agpu_context*)context))
    {
        AGPU_FREE(context);
        return NULL;
    }

    return (agpu_context*)context;
}

/*
static agpu_buffer* vk_create_swapchain(agpu_renderer* renderer, const agpu_swapchain_desc* desc) {
}*/

static agpu_buffer* vk_create_buffer(agpu_renderer* renderer, const agpu_buffer_desc* desc) {
    agpu_vk_renderer* vk_renderer = (agpu_vk_renderer*)renderer;

    // Create buffer
    const VkBufferCreateInfo buffer_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = desc->size,
      .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    };

    VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    const VmaAllocationCreateInfo memory_info = {
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = memory_usage
    };

    VkBuffer handle;
    VmaAllocation allocation;
    VmaAllocationInfo allocation_info;
    VkResult result = vmaCreateBuffer(vk_renderer->memory_allocator,
        &buffer_info,
        &memory_info,
        &handle, &allocation, &allocation_info);
    if (result != VK_SUCCESS) {
        GPU_THROW("[Vulkan]: Failed to create buffer");
    }

    agpu_vk_buffer* buffer = (agpu_vk_buffer*)AGPU_MALLOC(sizeof(agpu_vk_buffer));
    buffer->handle = handle;
    buffer->allocation = allocation;
    return (agpu_buffer*)result;
}

static void vk_destroy_buffer(agpu_renderer* renderer, agpu_buffer* buffer) {
    agpu_vk_renderer* vk_renderer = (agpu_vk_renderer*)renderer;
    agpu_vk_buffer* vk_uffer = (agpu_vk_buffer*)buffer;
}

static agpu_backend vk_query_backend() {
    return AGPU_BACKEND_VULKAN;
}

static agpu_features vk_query_features(agpu_renderer* renderer) {
    agpu_vk_renderer* vk_renderer = (agpu_vk_renderer*)renderer;
    return vk_renderer->features;
}

static agpu_limits vk_query_limits(agpu_renderer* renderer) {
    agpu_vk_renderer* vk_renderer = (agpu_vk_renderer*)renderer;
    return vk_renderer->limits;
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
    VkQueueFamilyProperties* queue_families = gpu_alloca(VkQueueFamilyProperties, queue_count);
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

    VkExtensionProperties* available_extensions = gpu_alloca(VkExtensionProperties, ext_count);
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
        else if (strcmp(available_extensions[i].extensionName, "VK_KHR_get_memory_requirements2") == 0) {
            result.get_memory_requirements2 = true;
        }
        else if (strcmp(available_extensions[i].extensionName, "VK_KHR_dedicated_allocation") == 0) {
            result.dedicated_allocation = true;
        }
        else if (strcmp(available_extensions[i].extensionName, "VK_KHR_image_format_list") == 0) {
            result.image_format_list = true;
        }
        else if (strcmp(available_extensions[i].extensionName, "VK_EXT_debug_marker") == 0) {
            result.debug_marker = true;
        }
        else if (strcmp(available_extensions[i].extensionName, "VK_EXT_full_screen_exclusive") == 0) {
            result.full_screen_exclusive = true;
        }
    }

    return result;
}

static bool _agpu_is_device_suitable(VkPhysicalDevice physical_device, VkSurfaceKHR surface, bool headless) {
    vk_queue_family_indices indices = _agpu_query_queue_families(physical_device, surface);

    if (indices.graphics_queue_family == VK_QUEUE_FAMILY_IGNORED)
        return false;

    vk_physical_device_features features = _agpu_query_device_extension_support(physical_device);
    if (!headless && !features.swapchain) {
        return false;
    }

    return true;
}

static agpu_device* vk_create_device(const char* application_name, const agpu_desc* desc) {
    agpu_vk_renderer* renderer;
    agpu_device* device;

    /* Create the device */
    device = (agpu_device*)AGPU_MALLOC(sizeof(agpu_device));
    ASSIGN_DRIVER(vk);

    /* Init the renderer */
    renderer = (agpu_vk_renderer*)AGPU_MALLOC(sizeof(agpu_vk_renderer));
    memset(renderer, 0, sizeof(agpu_vk_renderer));

    /* The FNA3D_Device and OpenGLRenderer need to reference each other */
    renderer->gpu_device = device;
    device->renderer = (agpu_renderer*)renderer;

    renderer->max_inflight_frames = _gpu_min(_gpu_def(desc->max_inflight_frames, 2u), 2u);

    bool headless = true;
    if (desc->swapchain) {
        headless = false;
    }

    /* Setup instance only once. */
    if (!vk.instance) {
        bool validation = false;
#if defined(VULKAN_DEBUG)
        if (desc->flags & AGPU_CONFIG_FLAGS_VALIDATION) {
            validation = true;
        }
#endif
        if (!agpu_vk_init_loader()) {
            return false;
        }

        uint32_t instance_extension_count;
        VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, NULL));

        VkExtensionProperties* available_instance_extensions = gpu_alloca(VkExtensionProperties, instance_extension_count);
        VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, available_instance_extensions));

        uint32_t enabled_ext_count = 0;
        char* enabled_exts[16];

        for (uint32_t i = 0; i < instance_extension_count; ++i) {
            if (strcmp(available_instance_extensions[i].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
                vk.debug_utils = true;
                enabled_exts[enabled_ext_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
            }
            else if (strcmp(available_instance_extensions[i].extensionName, VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME) == 0)
            {
                vk.headless = true;
            }
            else if (strcmp(available_instance_extensions[i].extensionName, VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME) == 0)
            {
                vk.surface_capabilities2 = true;
            }
            else if (strcmp(available_instance_extensions[i].extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0)
            {
                vk.physical_device_properties2 = true;
                enabled_exts[enabled_ext_count++] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
            }
            else if (strcmp(available_instance_extensions[i].extensionName, VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME) == 0)
            {
                vk.external_memory_capabilities = true;
                enabled_exts[enabled_ext_count++] = VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME;
            }
            else if (strcmp(available_instance_extensions[i].extensionName, VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME) == 0)
            {
                vk.external_semaphore_capabilities = true;
                enabled_exts[enabled_ext_count++] = VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME;
            }
        }

        if (headless) {
            if (!vk.headless) {

            }
            else
            {
                enabled_exts[enabled_ext_count++] = VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME;
            }
        }
        else {
            enabled_exts[enabled_ext_count++] = VK_KHR_SURFACE_EXTENSION_NAME;

#if defined(_WIN32)
            enabled_exts[enabled_ext_count++] = "VK_KHR_win32_surface";
#endif

            if (vk.surface_capabilities2) {
                enabled_exts[enabled_ext_count++] = VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME;
            }
        }

        uint32_t enabled_instance_layers_count = 0;
        const char* enabled_instance_layers[8];

#if defined(VULKAN_DEBUG)
        if (validation) {
            uint32_t instance_layer_count;
            VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr));

            VkLayerProperties* supported_validation_layers = gpu_alloca(VkLayerProperties, instance_layer_count);
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

        const uint32_t instance_version = agpu_vk_get_instance_version();
        vk.apiVersion12 = instance_version >= VK_API_VERSION_1_2;
        vk.apiVersion11 = instance_version >= VK_API_VERSION_1_1;
        if (vk.apiVersion12) {
            vk.apiVersion11 = true;
        }

        VkInstanceCreateInfo instance_info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &(VkApplicationInfo) {
                .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .pApplicationName = application_name,
                .applicationVersion = 0,
                .pEngineName = "alimer",
                .engineVersion = 0,
                .apiVersion = instance_version
            },
            .enabledLayerCount = enabled_instance_layers_count,
            .ppEnabledLayerNames = enabled_instance_layers,
            .enabledExtensionCount = enabled_ext_count,
            .ppEnabledExtensionNames = enabled_exts
        };

#if defined(VULKAN_DEBUG)
        VkDebugUtilsMessengerCreateInfoEXT debug_utils_create_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
        VkDebugReportCallbackCreateInfoEXT debug_report_create_info = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT };
        if (vk.debug_utils)
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

        VkResult result = vkCreateInstance(&instance_info, NULL, &vk.instance);
        if (result != VK_SUCCESS) {
            agpu_destroy_device(device);
            return false;
        }

        agpu_vk_init_instance(vk.instance);

#if defined(VULKAN_DEBUG)
        if (vk.debug_utils)
        {
            result = vkCreateDebugUtilsMessengerEXT(vk.instance, &debug_utils_create_info, NULL, &vk.debug_utils_messenger);
            if (result != VK_SUCCESS)
            {
                GPU_THROW("Could not create debug utils messenger");
                agpu_destroy_device(device);
                return false;
            }
        }
        else
        {
            result = vkCreateDebugReportCallbackEXT(vk.instance, &debug_report_create_info, NULL, &vk.debug_report_callback);
            if (result != VK_SUCCESS) {
                GPU_THROW("Could not create debug report callback");
                agpu_destroy_device(device);
                return false;
            }
        }
#endif

        /* Enumerate all physical device. */
        vk.physical_device_count = _GPU_MAX_PHYSICAL_DEVICES;
        result = vkEnumeratePhysicalDevices(vk.instance, &vk.physical_device_count, vk.physical_devices);
        if (result != VK_SUCCESS) {
            GPU_THROW("Cannot enumerate physical devices.");
            agpu_destroy_device(device);
            return false;
        }
    }

    /* Create surface if required. */
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    uint32_t backbuffer_width;
    uint32_t backbuffer_height;
    if (desc->swapchain)
    {
        surface = vk_create_surface(desc->swapchain->native_handle, &backbuffer_width, &backbuffer_height);
    }

    /* Find best supported physical device. */
    uint32_t best_device_score = 0U;
    uint32_t best_device_index = VK_QUEUE_FAMILY_IGNORED;
    for (uint32_t i = 0; i < vk.physical_device_count; ++i)
    {
        if (!_agpu_is_device_suitable(vk.physical_devices[i], surface, headless)) {
            continue;
        }

        VkPhysicalDeviceProperties physical_device_props;
        vkGetPhysicalDeviceProperties(vk.physical_devices[i], &physical_device_props);

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
        agpu_destroy_device(device);
        return false;
    }
    renderer->physical_device = vk.physical_devices[best_device_index];
    renderer->queue_families = _agpu_query_queue_families(renderer->physical_device, surface);
    renderer->device_features = _agpu_query_device_extension_support(renderer->physical_device);

    VkPhysicalDeviceProperties gpu_props;
    vkGetPhysicalDeviceProperties(renderer->physical_device, &gpu_props);

    if (gpu_props.apiVersion >= VK_API_VERSION_1_2)
    {
        renderer->api_version_12 = true;
        renderer->api_version_11 = true;
    }
    else if (gpu_props.apiVersion >= VK_API_VERSION_1_1)
    {
        renderer->api_version_11 = true;
    }

    /* Setup device queue's. */
    uint32_t queue_count;
    vkGetPhysicalDeviceQueueFamilyProperties(renderer->physical_device, &queue_count, NULL);
    VkQueueFamilyProperties* queue_families = gpu_alloca(VkQueueFamilyProperties, queue_count);
    vkGetPhysicalDeviceQueueFamilyProperties(renderer->physical_device, &queue_count, queue_families);

    uint32_t universal_queue_index = 1;
    uint32_t graphics_queue_index = 0;
    uint32_t compute_queue_index = 0;
    uint32_t copy_queue_index = 0;

    if (renderer->queue_families.compute_queue_family == VK_QUEUE_FAMILY_IGNORED)
    {
        renderer->queue_families.compute_queue_family = renderer->queue_families.graphics_queue_family;
        compute_queue_index = _gpu_min(queue_families[renderer->queue_families.graphics_queue_family].queueCount - 1, universal_queue_index);
        universal_queue_index++;
    }

    if (renderer->queue_families.copy_queue_family == VK_QUEUE_FAMILY_IGNORED)
    {
        renderer->queue_families.copy_queue_family = renderer->queue_families.graphics_queue_family;
        copy_queue_index = _gpu_min(queue_families[renderer->queue_families.graphics_queue_family].queueCount - 1, universal_queue_index);
        universal_queue_index++;
    }
    else if (renderer->queue_families.copy_queue_family == renderer->queue_families.compute_queue_family)
    {
        copy_queue_index = _gpu_min(queue_families[renderer->queue_families.compute_queue_family].queueCount - 1, 1u);
    }

    static const float graphics_queue_prio = 0.5f;
    static const float compute_queue_prio = 1.0f;
    static const float transfer_queue_prio = 1.0f;
    float prio[3] = { graphics_queue_prio, compute_queue_prio, transfer_queue_prio };

    uint32_t queue_family_count = 0;
    VkDeviceQueueCreateInfo queue_info[3] = { 0 };
    queue_info[queue_family_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info[queue_family_count].queueFamilyIndex = renderer->queue_families.graphics_queue_family;
    queue_info[queue_family_count].queueCount = _gpu_min(universal_queue_index, queue_families[renderer->queue_families.graphics_queue_family].queueCount);
    queue_info[queue_family_count].pQueuePriorities = prio;
    queue_family_count++;

    if (renderer->queue_families.compute_queue_family != renderer->queue_families.graphics_queue_family)
    {
        queue_info[queue_family_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[queue_family_count].queueFamilyIndex = renderer->queue_families.compute_queue_family;
        queue_info[queue_family_count].queueCount = _gpu_min(renderer->queue_families.copy_queue_family == renderer->queue_families.compute_queue_family ? 2u : 1u,
            queue_families[renderer->queue_families.compute_queue_family].queueCount);
        queue_info[queue_family_count].pQueuePriorities = prio + 1;
        queue_family_count++;
    }

    if (renderer->queue_families.copy_queue_family != renderer->queue_families.graphics_queue_family &&
        renderer->queue_families.copy_queue_family != renderer->queue_families.compute_queue_family)
    {
        queue_info[queue_family_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[queue_family_count].queueFamilyIndex = renderer->queue_families.copy_queue_family;
        queue_info[queue_family_count].queueCount = 1;
        queue_info[queue_family_count].pQueuePriorities = prio + 2;
        queue_family_count++;
    }

    /* Setup device extensions now. */
    uint32_t enabled_device_ext_count = 0;
    char* enabled_device_exts[64];
    if (!headless) {
        enabled_device_exts[enabled_device_ext_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    }

    if (renderer->device_features.maintenance_1) {
        enabled_device_exts[enabled_device_ext_count++] = "VK_KHR_maintenance1";
    }

    if (renderer->device_features.maintenance_2) {
        enabled_device_exts[enabled_device_ext_count++] = "VK_KHR_maintenance2";
    }

    if (renderer->device_features.maintenance_3) {
        enabled_device_exts[enabled_device_ext_count++] = "VK_KHR_maintenance3";
    }

    if (renderer->device_features.get_memory_requirements2 &&
        renderer->device_features.dedicated_allocation)
    {
        enabled_device_exts[enabled_device_ext_count++] = "VK_KHR_get_memory_requirements2";
        enabled_device_exts[enabled_device_ext_count++] = "VK_KHR_dedicated_allocation";
    }

#ifdef _WIN32
    if (vk.surface_capabilities2 &&
        renderer->device_features.full_screen_exclusive)
    {
        enabled_device_exts[enabled_device_ext_count++] = "VK_EXT_full_screen_exclusive";
    }
#endif

    VkPhysicalDeviceFeatures2KHR features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR
    };
    if (renderer->api_version_11 && vk.apiVersion11)
        vkGetPhysicalDeviceFeatures2(renderer->physical_device, &features);
    else if (vk.physical_device_properties2)
        vkGetPhysicalDeviceFeatures2KHR(renderer->physical_device, &features);
    else
        vkGetPhysicalDeviceFeatures(renderer->physical_device, &features.features);

    // Enable device features we might care about.
    {
        VkPhysicalDeviceFeatures enabled_features;
        memset(&enabled_features, 0, sizeof(VkPhysicalDeviceFeatures));
        if (features.features.textureCompressionETC2)
            enabled_features.textureCompressionETC2 = VK_TRUE;
        if (features.features.textureCompressionBC)
            enabled_features.textureCompressionBC = VK_TRUE;
        if (features.features.textureCompressionASTC_LDR)
            enabled_features.textureCompressionASTC_LDR = VK_TRUE;
        if (features.features.fullDrawIndexUint32)
            enabled_features.fullDrawIndexUint32 = VK_TRUE;
        if (features.features.imageCubeArray)
            enabled_features.imageCubeArray = VK_TRUE;
        if (features.features.fillModeNonSolid)
            enabled_features.fillModeNonSolid = VK_TRUE;
        if (features.features.independentBlend)
            enabled_features.independentBlend = VK_TRUE;
    }

    VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = queue_family_count,
        .pQueueCreateInfos = queue_info,
        .enabledExtensionCount = enabled_device_ext_count,
        .ppEnabledExtensionNames = enabled_device_exts
    };

    if (vk.physical_device_properties2)
        device_info.pNext = &features;
    else
        device_info.pEnabledFeatures = &features.features;

    VkResult result = vkCreateDevice(renderer->physical_device, &device_info, NULL, &renderer->device);
    if (result != VK_SUCCESS) {
        agpu_destroy_device(device);
        return false;
    }
    agpu_vk_init_device(renderer->device);
    vkGetDeviceQueue(renderer->device, renderer->queue_families.graphics_queue_family, graphics_queue_index, &renderer->graphics_queue);
    vkGetDeviceQueue(renderer->device, renderer->queue_families.compute_queue_family, compute_queue_index, &renderer->compute_queue);
    vkGetDeviceQueue(renderer->device, renderer->queue_families.copy_queue_family, copy_queue_index, &renderer->copy_queue);

    /* Create memory allocator. */
    {
        VmaVulkanFunctions vma_vulkan_func;
        vma_vulkan_func.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
        vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
        vma_vulkan_func.vkAllocateMemory = vkAllocateMemory;
        vma_vulkan_func.vkFreeMemory = vkFreeMemory;
        vma_vulkan_func.vkMapMemory = vkMapMemory;
        vma_vulkan_func.vkUnmapMemory = vkUnmapMemory;
        vma_vulkan_func.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
        vma_vulkan_func.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
        vma_vulkan_func.vkBindBufferMemory = vkBindBufferMemory;
        vma_vulkan_func.vkBindImageMemory = vkBindImageMemory;
        vma_vulkan_func.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
        vma_vulkan_func.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
        vma_vulkan_func.vkCreateBuffer = vkCreateBuffer;
        vma_vulkan_func.vkDestroyBuffer = vkDestroyBuffer;
        vma_vulkan_func.vkCreateImage = vkCreateImage;
        vma_vulkan_func.vkDestroyImage = vkDestroyImage;
        vma_vulkan_func.vkCmdCopyBuffer = vkCmdCopyBuffer;

        VmaAllocatorCreateFlags allocator_flags = 0;
        if (renderer->device_features.get_memory_requirements2 &&
            renderer->device_features.dedicated_allocation)
        {
            allocator_flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
        }

        const VmaAllocatorCreateInfo allocator_info = {
            .flags = allocator_flags,
            .physicalDevice = renderer->physical_device,
            .device = renderer->device,
            .pVulkanFunctions = &vma_vulkan_func,
            .instance = vk.instance,
            .vulkanApiVersion = agpu_vk_get_instance_version()
        };
        result = vmaCreateAllocator(&allocator_info, &renderer->memory_allocator);
        if (result != VK_SUCCESS) {
            GPU_THROW("Cannot create memory allocator.");
            agpu_destroy_device(device);
            return false;
        }
    }

    /* Init features and limits. */
    renderer->features.independent_blend = features.features.independentBlend;
    renderer->features.compute_shader = true;
    renderer->features.geometry_shader = features.features.geometryShader;
    renderer->features.tessellation_shader = features.features.tessellationShader;
    renderer->features.multi_viewport = features.features.multiViewport;
    renderer->features.index_uint32 = features.features.fullDrawIndexUint32;
    renderer->features.multi_draw_indirect = features.features.multiDrawIndirect;
    renderer->features.fill_mode_non_solid = features.features.fillModeNonSolid;
    renderer->features.sampler_anisotropy = features.features.samplerAnisotropy;
    renderer->features.texture_compression_BC = features.features.textureCompressionBC;
    renderer->features.texture_compression_PVRTC = false;
    renderer->features.texture_compression_ETC2 = features.features.textureCompressionETC2;
    renderer->features.texture_compression_ASTC = features.features.textureCompressionASTC_LDR;
    //renderer->features.texture_1D = true;
    renderer->features.texture_3D = true;
    renderer->features.texture_2D_array = true;
    renderer->features.texture_cube_array = features.features.imageCubeArray;
    //renderer->features.raytracing = vk.KHR_get_physical_device_properties2
    //    && renderer->device_features.get_memory_requirements2
    //    || HasExtension(VK_NV_RAY_TRACING_EXTENSION_NAME);

    // Limits
    renderer->limits.max_vertex_attributes = gpu_props.limits.maxVertexInputAttributes;
    renderer->limits.max_vertex_bindings = gpu_props.limits.maxVertexInputBindings;
    renderer->limits.max_vertex_attribute_offset = gpu_props.limits.maxVertexInputAttributeOffset;
    renderer->limits.max_vertex_binding_stride = gpu_props.limits.maxVertexInputBindingStride;

    renderer->limits.max_texture_size_1d = gpu_props.limits.maxImageDimension1D;
    renderer->limits.max_texture_size_2d = gpu_props.limits.maxImageDimension2D;
    renderer->limits.max_texture_size_3d = gpu_props.limits.maxImageDimension3D;
    renderer->limits.max_texture_size_cube = gpu_props.limits.maxImageDimensionCube;
    renderer->limits.max_texture_array_layers = gpu_props.limits.maxImageArrayLayers;
    renderer->limits.max_color_attachments = gpu_props.limits.maxColorAttachments;
    renderer->limits.max_uniform_buffer_size = gpu_props.limits.maxUniformBufferRange;
    renderer->limits.min_uniform_buffer_offset_alignment = gpu_props.limits.minUniformBufferOffsetAlignment;
    renderer->limits.max_storage_buffer_size = gpu_props.limits.maxStorageBufferRange;
    renderer->limits.min_storage_buffer_offset_alignment = gpu_props.limits.minStorageBufferOffsetAlignment;
    renderer->limits.max_sampler_anisotropy = (uint32_t)gpu_props.limits.maxSamplerAnisotropy;
    renderer->limits.max_viewports = gpu_props.limits.maxViewports;
    renderer->limits.max_viewport_width = gpu_props.limits.maxViewportDimensions[0];
    renderer->limits.max_viewport_height = gpu_props.limits.maxViewportDimensions[1];
    renderer->limits.max_tessellation_patch_size = gpu_props.limits.maxTessellationPatchSize;
    renderer->limits.point_size_range_min = gpu_props.limits.pointSizeRange[0];
    renderer->limits.point_size_range_max = gpu_props.limits.pointSizeRange[1];
    renderer->limits.line_width_range_min = gpu_props.limits.lineWidthRange[0];
    renderer->limits.line_width_range_max = gpu_props.limits.lineWidthRange[1];
    renderer->limits.max_compute_shared_memory_size = gpu_props.limits.maxComputeSharedMemorySize;
    renderer->limits.max_compute_work_group_count_x = gpu_props.limits.maxComputeWorkGroupCount[0];
    renderer->limits.max_compute_work_group_count_y = gpu_props.limits.maxComputeWorkGroupCount[1];
    renderer->limits.max_compute_work_group_count_z = gpu_props.limits.maxComputeWorkGroupCount[2];
    renderer->limits.max_compute_work_group_invocations = gpu_props.limits.maxComputeWorkGroupInvocations;
    renderer->limits.max_compute_work_group_size_x =  gpu_props.limits.maxComputeWorkGroupSize[0];
    renderer->limits.max_compute_work_group_size_y =  gpu_props.limits.maxComputeWorkGroupSize[1];
    renderer->limits.max_compute_work_group_size_z =  gpu_props.limits.maxComputeWorkGroupSize[2];

    /* Create main context and set it as active. */
    if (surface != VK_NULL_HANDLE)
    {
        agpu_vk_context* context = (agpu_vk_context*)AGPU_MALLOC(sizeof(agpu_vk_context));
        context->surface = surface;
        context->width = backbuffer_width;
        context->height = backbuffer_height;
        context->handle = VK_NULL_HANDLE;

        if (!vk_init_or_update_context(renderer, (agpu_context*)context))
        {
            GPU_THROW("Cannot create main context.");
            agpu_destroy_device(device);
            return false;
        }
    }

    /* Increase device being created. */
    vk.device_count++;

    return device;
}

agpu_driver vulkan_driver = {
    AGPU_BACKEND_VULKAN,
    vk_create_device
};
