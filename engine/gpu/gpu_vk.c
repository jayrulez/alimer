//
// Copyright (c) 2019 Amer Koleci.
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

#if defined(GPU_VK_BACKEND) && defined(TODO_VK)

#include "gpu_backend.h"
#include "stb_ds.h"

#include <vulkan/vulkan.h>
#include "vk/vk_mem_alloc.h"

/* Used by vk_mem_alloc */
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
#define GPU_FOREACH_INSTANCE_SURFACE(X)\
X(vkCreateAndroidSurfaceKHR)
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
#define GPU_FOREACH_INSTANCE_SURFACE(X)\
X(vkCreateWin32SurfaceKHR)\
X(vkGetPhysicalDeviceWin32PresentationSupportKHR)
#elif defined(VK_USE_PLATFORM_METAL_EXT)
#define GPU_FOREACH_INSTANCE_SURFACE(X)\
X(vkCreateMetalSurfaceEXT)
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
#define GPU_FOREACH_INSTANCE_SURFACE(X)\
X(vkCreateMacOSSurfaceMVK)
#elif defined(VK_USE_PLATFORM_XCB_KHR)
#define GPU_FOREACH_INSTANCE_SURFACE(X)\
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

#define _VK_GPU_MAX_PHYSICAL_DEVICES (32u)

#if !defined(NDEBUG) || defined(DEBUG) || defined(_DEBUG)
#   define VULKAN_DEBUG
#endif

#define GPU_DECLARE(fn) static PFN_##fn fn;
#define GPU_LOAD_ANONYMOUS(fn) fn = (PFN_##fn) vkGetInstanceProcAddr(NULL, #fn);
#define GPU_LOAD_INSTANCE(fn) fn = (PFN_##fn) vkGetInstanceProcAddr(vk.instance, #fn);
#define GPU_LOAD_DEVICE(fn) fn = (PFN_##fn) vkGetDeviceProcAddr(vk.device, #fn);

/* Declare function pointers */
GPU_FOREACH_ANONYMOUS(GPU_DECLARE);
GPU_FOREACH_INSTANCE(GPU_DECLARE);
GPU_FOREACH_INSTANCE_SURFACE(GPU_DECLARE);
GPU_FOREACH_DEVICE(GPU_DECLARE);

typedef struct {
    bool swapchain;
    bool maintenance_1;
    bool maintenance_2;
    bool maintenance_3;
    bool KHR_get_memory_requirements2;
    bool KHR_dedicated_allocation;
    bool image_format_list;
    bool debug_marker;
    bool raytracing;
    bool buffer_device_address;
    bool deferred_host_operations;
    bool descriptor_indexing;
    bool pipeline_library;
} vk_physical_device_features;

typedef struct VGPUVkQueueFamilyIndices {
    uint32_t graphics_queue_family;
    uint32_t compute_queue_family;
    uint32_t copy_queue_family;
} VGPUVkQueueFamilyIndices;

#define _VGPU_VK_MAX_SWAPCHAINS (16u)
typedef struct VGPUSwapchainVk {
    VkSurfaceKHR surface;
    VkSwapchainKHR handle;

    uint32_t preferredImageCount;
    uint32_t width;
    uint32_t height;
    VkPresentModeKHR    presentMode;
    vgpu_pixel_format   colorFormat;
    vgpu_color          clear_color;
    vgpu_pixel_format   depthStencilFormat;
    uint32_t imageIndex;
    uint32_t imageCount;
    vgpu_texture backbufferTextures[4];
    vgpu_texture depthStencilTexture;
    VGPURenderPass renderPasses[4];
} VGPUSwapchainVk;

typedef struct {
    VkBuffer handle;
    VmaAllocation memory;
} VGPUBufferVk;

typedef struct {
    VkFormat format;
    VkImage handle;
    VkImageView view;
    VmaAllocation memory;
    bool external;
    vgpu_texture_desc desc;
    VGPUTextureLayout layout;
} VGPUTextureVk;

typedef struct {
    VkSampler handle;
} VGPUSamplerVk;

typedef struct {
    VkRenderPass renderPass;
    VkFramebuffer framebuffer;
    VkRect2D renderArea;
    uint32_t color_attachment_count;
    vgpu_texture textures[VGPU_MAX_COLOR_ATTACHMENTS + 1];
    VkClearValue clears[VGPU_MAX_COLOR_ATTACHMENTS + 1];
} VGPURenderPassVk;

typedef struct {
    VkObjectType type;
    void* handle1;
    void* handle2;
} gpu_vk_ref;

typedef struct {
    gpu_vk_ref* data;
    size_t capacity;
    size_t length;
} vgpu_vk_free_list;

typedef struct {
    uint32_t index;
    VkFence fence;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderCompleteSemaphore;
    VkCommandPool command_pool;
    vgpu_vk_free_list free_list;
} gpu_vk_frame;

typedef struct
{
    uint32_t colorFormatsCount;
    VkFormat colorFormats[VGPU_MAX_COLOR_ATTACHMENTS];
    vgpu_load_action loadOperations[VGPU_MAX_COLOR_ATTACHMENTS];
    VkFormat depthStencilFormat;
} RenderPassHash;

typedef struct
{
    RenderPassHash key;
    VkRenderPass value;
} RenderPassHashMap;

static struct {
    bool available_initialized;
    bool available;

    void* library;

    gpu_config config;
    bool headless;

    bool debug_utils;
    bool headless_extension;
    bool surface_capabilities2;
    bool KHR_get_physical_device_properties2;
    bool external_memory_capabilities;
    bool external_semaphore_capabilities;
    bool win32_full_screen_exclusive;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_utils_messenger;
    VkDebugReportCallbackEXT debug_report_callback;

    uint32_t physical_device_count;
    VkPhysicalDevice physical_devices[_VK_GPU_MAX_PHYSICAL_DEVICES];

    VkPhysicalDevice physical_device;
    VGPUVkQueueFamilyIndices queue_families;

    vk_physical_device_features device_features;
    vgpu_caps caps;

    VkDevice device;
    VkQueue graphics_queue;
    VkQueue compute_queue;
    VkQueue copy_queue;
    VmaAllocator allocator;

    gpu_vk_frame frames[3];
    gpu_vk_frame* frame;
    uint32_t max_inflight_frames;

    VGPUSwapchainVk swapchains[_VGPU_VK_MAX_SWAPCHAINS];
    RenderPassHashMap* renderPassHashMap;
} vk;

#define GPU_VK_THROW(str) if (vk.config.log_callback) { vk.config.log_callback(vk.config.context, GPU_LOG_LEVEL_ERROR, str); }
#define GPU_VK_CHECK(c, str) if (!(c)) { GPU_VK_THROW(str); }
#define VK_CHECK(res) do { VkResult r = (res); GPU_VK_CHECK(r >= 0, _gpu_vk_get_error_string(r)); } while (0)

#if defined(VULKAN_DEBUG)
VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT flags,
    const VkDebugUtilsMessengerCallbackDataEXT* pData, void* user_data)
{
    if (vk.config.log_callback) {
        if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            //vgpu_log_format(GPU_LOG_LEVEL_WARN, "%u - %s: %s", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
            vk.config.log_callback(user_data, GPU_LOG_LEVEL_WARN, pData->pMessage);
        }
        else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            vk.config.log_callback(user_data, GPU_LOG_LEVEL_ERROR, pData->pMessage);
        }
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
    _VGPU_UNUSED(type);
    _VGPU_UNUSED(object);
    _VGPU_UNUSED(location);
    _VGPU_UNUSED(message_code);

    if (vk.config.log_callback) {
        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        {
            vk.config.log_callback(user_data, GPU_LOG_LEVEL_ERROR, message);
        }
        else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
        {
            vk.config.log_callback(user_data, GPU_LOG_LEVEL_WARN, message);
        }
        else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
        {
            vk.config.log_callback(user_data, GPU_LOG_LEVEL_WARN, message);
        }
        else
        {
            vk.config.log_callback(user_data, GPU_LOG_LEVEL_INFO, message);
        }
    }

    return VK_FALSE;
}
#endif

/* Helper */
static const char* _gpu_vk_get_error_string(VkResult result) {
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

static vk_physical_device_features vgpuVkQueryDeviceExtensionSupport(VkPhysicalDevice physical_device)
{
    uint32_t ext_count = 0;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, NULL, &ext_count, nullptr));

    VkExtensionProperties* available_extensions = VGPU_ALLOCA(VkExtensionProperties, ext_count);
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
        else if (strcmp(available_extensions[i].extensionName, VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME) == 0) {
            result.KHR_get_memory_requirements2 = true;
        }
        else if (strcmp(available_extensions[i].extensionName, VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME) == 0) {
            result.KHR_dedicated_allocation = true;
        }
        else if (strcmp(available_extensions[i].extensionName, VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME) == 0) {
            result.image_format_list = true;
        }
        else if (strcmp(available_extensions[i].extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0) {
            result.debug_marker = true;
        }
        else if (strcmp(available_extensions[i].extensionName, VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME) == 0) {
            vk.win32_full_screen_exclusive = true;
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
    }

    return result;
}

static bool _gpu_vk_query_presentation_support(VkPhysicalDevice device, uint32_t queue_family_index)
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    return vkGetPhysicalDeviceWin32PresentationSupportKHR(device, queue_family_index);
#elif defined(__ANDROID__)
    return true;
#else
    /* TODO: */
    return true;
#endif
}

static VGPUVkQueueFamilyIndices vgpuVkQueryQueueFamilies(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    uint32_t queue_count = 0u;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_count, NULL);
    VkQueueFamilyProperties* queue_families = VGPU_ALLOCA(VkQueueFamilyProperties, queue_count);
    assert(queue_families);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_count, queue_families);

    VGPUVkQueueFamilyIndices result;
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
            present_support = _gpu_vk_query_presentation_support(physical_device, i);
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

static bool _gpu_vk_is_device_suitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    VkPhysicalDeviceProperties gpuProps;
    vkGetPhysicalDeviceProperties(physicalDevice, &gpuProps);

    /* We run on vulkan 1.1 or higher. */
    if (gpuProps.apiVersion < VK_API_VERSION_1_1)
    {
        return false;
    }

    VGPUVkQueueFamilyIndices indices = vgpuVkQueryQueueFamilies(physicalDevice, surface);

    if (indices.graphics_queue_family == VK_QUEUE_FAMILY_IGNORED)
        return false;

    vk_physical_device_features features = vgpuVkQueryDeviceExtensionSupport(physicalDevice);
    if (surface != VK_NULL_HANDLE && !features.swapchain) {
        return false;
    }

    /* We required maintenance_1 to support viewport flipping to match DX style. */
    if (!features.maintenance_1) {
        return false;
    }

    return true;
}

static bool _gpu_vk_create_surface(void* window_handle, VkSurfaceKHR* pSurface) {
    VkResult result = VK_SUCCESS;

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    HWND hWnd = (HWND)window_handle;
    VGPU_ASSERT(IsWindow(hWnd));

    const VkWin32SurfaceCreateInfoKHR surface_info = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .hinstance = (HINSTANCE)GetModuleHandle(NULL),
        .hwnd = hWnd
    };
    result = vkCreateWin32SurfaceKHR(vk.instance, &surface_info, NULL, pSurface);
#endif
    if (result != VK_SUCCESS) {
        //vgpu_log_error("Failed to create surface");
        return false;
    }

    return true;
}

/* Helper functions .*/
static void _gpu_vk_destroy(void* handle1, void* handle2, VkObjectType type)
{
    vgpu_vk_free_list* freelist = &vk.frame->free_list;

    if (freelist->length >= freelist->capacity) {
        freelist->capacity = freelist->capacity ? (freelist->capacity * 2) : 1;
        freelist->data = (gpu_vk_ref*)realloc(freelist->data, freelist->capacity * sizeof(*freelist->data));
        GPU_VK_CHECK(freelist->data, "Out of memory");
    }

    freelist->data[freelist->length++] = (gpu_vk_ref){ type, handle1, handle2 };
}

static void _gpu_vk_process_destroy(gpu_vk_frame* frame)
{
    for (size_t i = 0; i < frame->free_list.length; i++)
    {
        gpu_vk_ref* ref = &frame->free_list.data[i];
        switch (ref->type)
        {
        case VK_OBJECT_TYPE_BUFFER:
            vmaDestroyBuffer(vk.allocator, (VkBuffer)ref->handle1, (VmaAllocation)ref->handle2);
            break;

        case VK_OBJECT_TYPE_IMAGE:
            vmaDestroyImage(vk.allocator, (VkImage)ref->handle1, (VmaAllocation)ref->handle2);
            break;

        case VK_OBJECT_TYPE_IMAGE_VIEW:
            vkDestroyImageView(vk.device, (VkImageView)ref->handle1, NULL);
            break;

        case VK_OBJECT_TYPE_SAMPLER:
            vkDestroySampler(vk.device, (VkSampler)ref->handle1, NULL);
            break;

        case VK_OBJECT_TYPE_RENDER_PASS:
            vkDestroyRenderPass(vk.device, (VkRenderPass)ref->handle1, NULL);
            break;

        case VK_OBJECT_TYPE_FRAMEBUFFER:
            vkDestroyFramebuffer(vk.device, (VkFramebuffer)ref->handle1, NULL);
            break;

        case VK_OBJECT_TYPE_PIPELINE:
            vkDestroyPipeline(vk.device, (VkPipeline)ref->handle1, NULL);
            break;
        default:
            GPU_VK_THROW("Vulkan: Invalid object type");
            break;
        }
    }

    frame->free_list.length = 0;

    /* Reset command pool. */
    VK_CHECK(vkResetCommandPool(vk.device, frame->command_pool, 0));
}

static void _gpu_vk_set_name(void* handle, VkObjectType type, const char* name)
{
    if (name && vk.config.debug)
    {
        const VkDebugUtilsObjectNameInfoEXT info = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .pNext = NULL,
            .objectType = type,
            .objectHandle = GPU_VK_VOIDP_TO_U64(handle),
            .pObjectName = name
        };

        VK_CHECK(vkSetDebugUtilsObjectNameEXT(vk.device, &info));
    }
}


/* Conversion functions */
static inline VkFormat GetVkFormat(vgpu_pixel_format format)
{
    static VkFormat formats[VGPU_PIXEL_FORMAT_COUNT] = {
        VK_FORMAT_UNDEFINED,
        // 8-bit pixel formats
        VK_FORMAT_R8_UNORM,
        VK_FORMAT_R8_SNORM,
        VK_FORMAT_R8_UINT,
        VK_FORMAT_R8_SINT,
        // 16-bit pixel formats
        VK_FORMAT_R16_UNORM,
        VK_FORMAT_R16_SNORM,
        VK_FORMAT_R16_UINT,
        VK_FORMAT_R16_SINT,
        VK_FORMAT_R16_SFLOAT,
        VK_FORMAT_R8G8_UNORM,
        VK_FORMAT_R8G8_SNORM,
        VK_FORMAT_R8G8_UINT,
        VK_FORMAT_R8G8_SINT,

        // Packed 16-bit pixel formats
        //VK_FORMAT_B5G6R5_UNORM_PACK16,
        //VK_FORMAT_B4G4R4A4_UNORM_PACK16,

        // 32-bit pixel formats
        VK_FORMAT_R32_UINT,
        VK_FORMAT_R32_SINT,
        VK_FORMAT_R32_SFLOAT,
        //VK_FORMAT_R16G16_UNORM,
        //VK_FORMAT_R16G16_SNORM,
        VK_FORMAT_R16G16_UINT,
        VK_FORMAT_R16G16_SINT,
        VK_FORMAT_R16G16_SFLOAT,

        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_R8G8B8A8_SNORM,
        VK_FORMAT_R8G8B8A8_UINT,
        VK_FORMAT_R8G8B8A8_SINT,

        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_B8G8R8A8_SRGB,

        // Packed 32-Bit Pixel formats
        VK_FORMAT_A2B10G10R10_UNORM_PACK32,
        VK_FORMAT_B10G11R11_UFLOAT_PACK32,

        // 64-Bit Pixel Formats
        VK_FORMAT_R32G32_UINT,
        VK_FORMAT_R32G32_SINT,
        VK_FORMAT_R32G32_SFLOAT,
        //VK_FORMAT_R16G16B16A16_UNORM,
        //VK_FORMAT_R16G16B16A16_SNORM,
        VK_FORMAT_R16G16B16A16_UINT,
        VK_FORMAT_R16G16B16A16_SINT,
        VK_FORMAT_R16G16B16A16_SFLOAT,

        // 128-Bit Pixel Formats
        VK_FORMAT_R32G32B32A32_UINT,
        VK_FORMAT_R32G32B32A32_SINT,
        VK_FORMAT_R32G32B32A32_SFLOAT,

        // Depth-stencil formats
        VK_FORMAT_D16_UNORM,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT, /* Dawn maps to VK_FORMAT_D32_SFLOAT */
        VK_FORMAT_D32_SFLOAT_S8_UINT,

        // Compressed BC formats
        VK_FORMAT_BC1_RGB_UNORM_BLOCK,
        VK_FORMAT_BC1_RGB_SRGB_BLOCK,
        VK_FORMAT_BC2_UNORM_BLOCK,
        VK_FORMAT_BC2_SRGB_BLOCK,
        VK_FORMAT_BC3_UNORM_BLOCK,
        VK_FORMAT_BC3_SRGB_BLOCK,
        VK_FORMAT_BC4_UNORM_BLOCK,
        VK_FORMAT_BC4_SNORM_BLOCK,
        VK_FORMAT_BC5_UNORM_BLOCK,
        VK_FORMAT_BC5_SNORM_BLOCK,
        VK_FORMAT_BC6H_UFLOAT_BLOCK,
        VK_FORMAT_BC6H_SFLOAT_BLOCK,
        VK_FORMAT_BC7_UNORM_BLOCK,
        VK_FORMAT_BC7_SRGB_BLOCK,
    };

    return formats[format];
}

static inline VkImageAspectFlags GetVkAspectMask(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_UNDEFINED:
        return 0;

    case VK_FORMAT_S8_UINT:
        return VK_IMAGE_ASPECT_STENCIL_BIT;

    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT;

    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
        return VK_IMAGE_ASPECT_DEPTH_BIT;

    default:
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

static VkCompareOp get_vk_compare_op(vgpu_compare_function function)
{
    switch (function)
    {
    case VGPU_COMPARE_FUNCTION_NEVER:
        return VK_COMPARE_OP_NEVER;
    case VGPU_COMPARE_FUNCTION_LESS:
        return VK_COMPARE_OP_LESS;
    case VGPU_COMPARE_FUNCTION_LESS_EQUAL:
        return VK_COMPARE_OP_LESS_OR_EQUAL;
    case VGPU_COMPARE_FUNCTION_GREATER:
        return VK_COMPARE_OP_GREATER;
    case VGPU_COMPARE_FUNCTION_GREATER_EQUAL:
        return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case VGPU_COMPARE_FUNCTION_EQUAL:
        return VK_COMPARE_OP_EQUAL;
    case VGPU_COMPARE_FUNCTION_NOT_EQUAL:
        return VK_COMPARE_OP_NOT_EQUAL;
    case VGPU_COMPARE_FUNCTION_ALWAYS:
        return VK_COMPARE_OP_ALWAYS;

    default:
        _VGPU_UNREACHABLE();
    }
}

static bool vk_init(void* window_handle, const gpu_config* config) {
    if (!gpu_vk_supported()) {
        return false;
    }

    memcpy(&vk.config, config, sizeof(gpu_config));
    vk.headless = window_handle == NULL;

    uint32_t instance_extension_count = 0;
    VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, NULL));

    VkExtensionProperties* available_instance_extensions = VGPU_ALLOCA(VkExtensionProperties, instance_extension_count);
    VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, available_instance_extensions));

    uint32_t enabled_ext_count = 0;
    char* enabled_instance_exts[16];

    for (uint32_t i = 0; i < instance_extension_count; ++i)
    {
        if (strcmp(available_instance_extensions[i].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
            vk.debug_utils = true;
            enabled_instance_exts[enabled_ext_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        }
        else if (strcmp(available_instance_extensions[i].extensionName, VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME) == 0)
        {
            vk.headless_extension = true;
        }
        else if (strcmp(available_instance_extensions[i].extensionName, VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME) == 0)
        {
            vk.surface_capabilities2 = true;
        }
        else if (strcmp(available_instance_extensions[i].extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0)
        {
            vk.KHR_get_physical_device_properties2 = true;
            enabled_instance_exts[enabled_ext_count++] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
        }
        else if (strcmp(available_instance_extensions[i].extensionName, VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME) == 0)
        {
            vk.external_memory_capabilities = true;
            enabled_instance_exts[enabled_ext_count++] = VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME;
        }
        else if (strcmp(available_instance_extensions[i].extensionName, VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME) == 0)
        {
            vk.external_semaphore_capabilities = true;
            enabled_instance_exts[enabled_ext_count++] = VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME;
        }
    }

    if (vk.headless)
    {
        if (vk.headless_extension) {
            enabled_instance_exts[enabled_ext_count++] = VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME;
        }
    }
    else
    {
        enabled_instance_exts[enabled_ext_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        enabled_instance_exts[enabled_ext_count++] = "VK_KHR_win32_surface";
#endif

        if (vk.surface_capabilities2) {
            enabled_instance_exts[enabled_ext_count++] = "VK_KHR_get_surface_capabilities2";
        }
    }

    uint32_t enabled_instance_layers_count = 0;
    const char* enabled_instance_layers[8];

#if defined(VULKAN_DEBUG)
    if (config->debug) {
        uint32_t instance_layer_count;
        VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr));

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
#endif

    /* We require version 1.1 or higher */
    if (!vkEnumerateInstanceVersion) {
        return false;
    }

    uint32_t apiVersion;
    if (vkEnumerateInstanceVersion(&apiVersion) != VK_SUCCESS)
    {
        apiVersion = VK_API_VERSION_1_1;
    }

    if (apiVersion < VK_API_VERSION_1_1) {
        return false;
    }

    VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &(VkApplicationInfo) {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .apiVersion = apiVersion
        },
        .enabledLayerCount = enabled_instance_layers_count,
        .ppEnabledLayerNames = enabled_instance_layers,
        .enabledExtensionCount = enabled_ext_count,
        .ppEnabledExtensionNames = enabled_instance_exts
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
        gpu_shutdown();
        return false;
    }

    GPU_FOREACH_INSTANCE(GPU_LOAD_INSTANCE);
    if (!vk.headless) {
        GPU_FOREACH_INSTANCE_SURFACE(GPU_LOAD_INSTANCE);
    }

    vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(vk.instance, "vkGetDeviceProcAddr");

#if defined(VULKAN_DEBUG)
    if (vk.debug_utils)
    {
        result = vkCreateDebugUtilsMessengerEXT(vk.instance, &debug_utils_create_info, NULL, &vk.debug_utils_messenger);
        if (result != VK_SUCCESS)
        {
            GPU_VK_THROW("Could not create debug utils messenger");
            gpu_shutdown();
            return false;
        }
    }
    else
    {
        result = vkCreateDebugReportCallbackEXT(vk.instance, &debug_report_create_info, NULL, &vk.debug_report_callback);
        if (result != VK_SUCCESS) {
            GPU_VK_THROW("Could not create debug report callback");
            gpu_shutdown();
            return false;
        }
    }
#endif

    /* Enumerate all physical device. */
    vk.physical_device_count = _VK_GPU_MAX_PHYSICAL_DEVICES;
    result = vkEnumeratePhysicalDevices(vk.instance, &vk.physical_device_count, vk.physical_devices);
    if (result != VK_SUCCESS) {
        GPU_VK_THROW("Vulkan: Cannot enumerate physical devices.");
        gpu_shutdown();
        return false;
    }

    VkSurfaceKHR surface;
    if (!_gpu_vk_create_surface(window_handle, &surface)) {
        gpu_shutdown();
        return false;
    }

    /* Find best supported physical device. */
    uint32_t best_device_score = 0U;
    uint32_t best_device_index = VK_QUEUE_FAMILY_IGNORED;
    for (uint32_t i = 0; i < vk.physical_device_count; ++i)
    {
        if (!_gpu_vk_is_device_suitable(vk.physical_devices[i], surface)) {
            continue;
        }

        VkPhysicalDeviceProperties physical_device_props;
        vkGetPhysicalDeviceProperties(vk.physical_devices[i], &physical_device_props);

        uint32_t score = 0u;

        if (physical_device_props.apiVersion >= VK_API_VERSION_1_2) {
            score += 10000u;
        }

        switch (physical_device_props.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            score += 100;
            if (config->device_preference == GPU_DEVICE_PREFERENCE_DISCRETE) {
                score += 1000;
            }
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            score += 90;
            if (config->device_preference == GPU_DEVICE_PREFERENCE_INTEGRATED) {
                score += 1000;
            }
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            score += 80;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            score += 70;
            break;
        default: score += 10U;
        }
        if (score > best_device_score) {
            best_device_index = i;
            best_device_score = score;
        }
    }

    if (best_device_index == VK_QUEUE_FAMILY_IGNORED) {
        GPU_VK_THROW("Vulkan: Cannot find suitable physical device.");
        gpu_shutdown();
        return false;
    }
    vk.physical_device = vk.physical_devices[best_device_index];
    vk.queue_families = vgpuVkQueryQueueFamilies(vk.physical_device, VK_NULL_HANDLE);
    vk.device_features = vgpuVkQueryDeviceExtensionSupport(vk.physical_device);

    VkPhysicalDeviceProperties gpu_props;
    vkGetPhysicalDeviceProperties(vk.physical_device, &gpu_props);

    /* Setup device queue's. */
    uint32_t queue_count;
    vkGetPhysicalDeviceQueueFamilyProperties(vk.physical_device, &queue_count, NULL);
    VkQueueFamilyProperties* queue_families = VGPU_ALLOCA(VkQueueFamilyProperties, queue_count);
    vkGetPhysicalDeviceQueueFamilyProperties(vk.physical_device, &queue_count, queue_families);

    uint32_t universal_queue_index = 1;
    uint32_t graphics_queue_index = 0;
    uint32_t compute_queue_index = 0;
    uint32_t copy_queue_index = 0;

    if (vk.queue_families.compute_queue_family == VK_QUEUE_FAMILY_IGNORED)
    {
        vk.queue_families.compute_queue_family = vk.queue_families.graphics_queue_family;
        compute_queue_index = _vgpu_min(queue_families[vk.queue_families.graphics_queue_family].queueCount - 1, universal_queue_index);
        universal_queue_index++;
    }

    if (vk.queue_families.copy_queue_family == VK_QUEUE_FAMILY_IGNORED)
    {
        vk.queue_families.copy_queue_family = vk.queue_families.graphics_queue_family;
        copy_queue_index = _vgpu_min(queue_families[vk.queue_families.graphics_queue_family].queueCount - 1, universal_queue_index);
        universal_queue_index++;
    }
    else if (vk.queue_families.copy_queue_family == vk.queue_families.compute_queue_family)
    {
        copy_queue_index = _vgpu_min(queue_families[vk.queue_families.compute_queue_family].queueCount - 1, 1u);
    }

    static const float graphics_queue_prio = 0.5f;
    static const float compute_queue_prio = 1.0f;
    static const float transfer_queue_prio = 1.0f;
    float prio[3] = { graphics_queue_prio, compute_queue_prio, transfer_queue_prio };

    uint32_t queue_family_count = 0;
    VkDeviceQueueCreateInfo queue_info[3] = { 0 };
    queue_info[queue_family_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info[queue_family_count].queueFamilyIndex = vk.queue_families.graphics_queue_family;
    queue_info[queue_family_count].queueCount = _vgpu_min(universal_queue_index, queue_families[vk.queue_families.graphics_queue_family].queueCount);
    queue_info[queue_family_count].pQueuePriorities = prio;
    queue_family_count++;

    if (vk.queue_families.compute_queue_family != vk.queue_families.graphics_queue_family)
    {
        queue_info[queue_family_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[queue_family_count].queueFamilyIndex = vk.queue_families.compute_queue_family;
        queue_info[queue_family_count].queueCount = _vgpu_min(vk.queue_families.copy_queue_family == vk.queue_families.compute_queue_family ? 2u : 1u,
            queue_families[vk.queue_families.compute_queue_family].queueCount);
        queue_info[queue_family_count].pQueuePriorities = prio + 1;
        queue_family_count++;
    }

    if (vk.queue_families.copy_queue_family != vk.queue_families.graphics_queue_family &&
        vk.queue_families.copy_queue_family != vk.queue_families.compute_queue_family)
    {
        queue_info[queue_family_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[queue_family_count].queueFamilyIndex = vk.queue_families.copy_queue_family;
        queue_info[queue_family_count].queueCount = 1;
        queue_info[queue_family_count].pQueuePriorities = prio + 2;
        queue_family_count++;
    }

    /* Setup device extensions now. */
    uint32_t enabled_device_ext_count = 0;
    char* enabled_device_exts[64];
    enabled_device_exts[enabled_device_ext_count++] = VK_KHR_MAINTENANCE1_EXTENSION_NAME;

    if (!vk.headless) {
        enabled_device_exts[enabled_device_ext_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    }

    if (vk.device_features.maintenance_2) {
        enabled_device_exts[enabled_device_ext_count++] = VK_KHR_MAINTENANCE2_EXTENSION_NAME;
    }

    if (vk.device_features.maintenance_3) {
        enabled_device_exts[enabled_device_ext_count++] = VK_KHR_MAINTENANCE3_EXTENSION_NAME;
    }

    if (vk.device_features.KHR_get_memory_requirements2 &&
        vk.device_features.KHR_dedicated_allocation)
    {
        enabled_device_exts[enabled_device_ext_count++] = VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME;
        enabled_device_exts[enabled_device_ext_count++] = VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME;
    }

#ifdef _WIN32
    if (vk.surface_capabilities2 &&
        vk.win32_full_screen_exclusive)
    {
        enabled_device_exts[enabled_device_ext_count++] = VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME;
    }
#endif

    if (vk.device_features.buffer_device_address)
    {
        enabled_device_exts[enabled_device_ext_count++] = VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME;
    }

    VkPhysicalDeviceFeatures2 features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2
    };
    vkGetPhysicalDeviceFeatures2(vk.physical_device, &features);

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

    if (vk.KHR_get_physical_device_properties2)
        device_info.pNext = &features;
    else
        device_info.pEnabledFeatures = &features.features;

    result = vkCreateDevice(vk.physical_device, &device_info, NULL, &vk.device);
    if (result != VK_SUCCESS) {
        gpu_shutdown();
        return false;
    }

    GPU_FOREACH_DEVICE(GPU_LOAD_DEVICE);

    vkGetDeviceQueue(vk.device, vk.queue_families.graphics_queue_family, graphics_queue_index, &vk.graphics_queue);
    vkGetDeviceQueue(vk.device, vk.queue_families.compute_queue_family, compute_queue_index, &vk.compute_queue);
    vkGetDeviceQueue(vk.device, vk.queue_families.copy_queue_family, copy_queue_index, &vk.copy_queue);

    /* Init hash maps */
    hmdefault(vk.renderPassHashMap, NULL);

    /* Create memory allocator. */
    {
        VmaAllocatorCreateFlags allocator_flags = 0;
        if (vk.device_features.KHR_get_memory_requirements2 &&
            vk.device_features.KHR_dedicated_allocation)
        {
            allocator_flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
        }

        if (vk.device_features.buffer_device_address)
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
            GPU_VK_THROW("Vulkan: Cannot create memory allocator.");
            gpu_shutdown();
            return false;
        }
    }

    /* Init features and limits. */
    vk.caps.backend = VGPU_BACKEND_VULKAN;
    vk.caps.vendor_id = gpu_props.vendorID;
    vk.caps.device_id = gpu_props.deviceID;
    /*switch (gpu_props.deviceType)
    {
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        renderer->caps.adapter_type = VGPU_ADAPTER_TYPE_INTEGRATED_GPU;
        break;
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        renderer->caps.adapter_type = VGPU_ADAPTER_TYPE_DISCRETE_GPU;
        break;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        renderer->caps.adapter_type = VGPU_ADAPTER_TYPE_CPU;
        break;

    default:
        renderer->caps.adapter_type = VGPU_ADAPTER_TYPE_UNKNOWN;
        break;
    }*/
    memcpy(vk.caps.adapter_name, gpu_props.deviceName, VGPU_MAX_DEVICE_NAME_SIZE);

    vk.caps.features.independentBlend = features.features.independentBlend;
    vk.caps.features.computeShader = true;
    vk.caps.features.geometryShader = features.features.geometryShader;
    vk.caps.features.tessellationShader = features.features.tessellationShader;
    vk.caps.features.multiViewport = features.features.multiViewport;
    vk.caps.features.indexUint32 = features.features.fullDrawIndexUint32;
    vk.caps.features.multiDrawIndirect = features.features.multiDrawIndirect;
    vk.caps.features.fillModeNonSolid = features.features.fillModeNonSolid;
    vk.caps.features.samplerAnisotropy = features.features.samplerAnisotropy;
    vk.caps.features.textureCompressionETC2 = features.features.textureCompressionETC2;
    vk.caps.features.textureCompressionASTC_LDR = features.features.textureCompressionASTC_LDR;
    vk.caps.features.textureCompressionBC = features.features.textureCompressionBC;
    vk.caps.features.textureCubeArray = features.features.imageCubeArray;
    vk.caps.features.raytracing =
        vk.KHR_get_physical_device_properties2 &&
        vk.device_features.KHR_get_memory_requirements2 &&
        vk.device_features.raytracing;

    // Limits
    vk.caps.limits.max_vertex_attributes = gpu_props.limits.maxVertexInputAttributes;
    vk.caps.limits.max_vertex_bindings = gpu_props.limits.maxVertexInputBindings;
    vk.caps.limits.max_vertex_attribute_offset = gpu_props.limits.maxVertexInputAttributeOffset;
    vk.caps.limits.max_vertex_binding_stride = gpu_props.limits.maxVertexInputBindingStride;

    vk.caps.limits.max_texture_size_1d = gpu_props.limits.maxImageDimension1D;
    vk.caps.limits.max_texture_size_2d = gpu_props.limits.maxImageDimension2D;
    vk.caps.limits.max_texture_size_3d = gpu_props.limits.maxImageDimension3D;
    vk.caps.limits.max_texture_size_cube = gpu_props.limits.maxImageDimensionCube;
    vk.caps.limits.max_texture_array_layers = gpu_props.limits.maxImageArrayLayers;
    vk.caps.limits.max_color_attachments = gpu_props.limits.maxColorAttachments;
    vk.caps.limits.max_uniform_buffer_size = gpu_props.limits.maxUniformBufferRange;
    vk.caps.limits.min_uniform_buffer_offset_alignment = gpu_props.limits.minUniformBufferOffsetAlignment;
    vk.caps.limits.max_storage_buffer_size = gpu_props.limits.maxStorageBufferRange;
    vk.caps.limits.min_storage_buffer_offset_alignment = gpu_props.limits.minStorageBufferOffsetAlignment;
    vk.caps.limits.max_sampler_anisotropy = (uint32_t)gpu_props.limits.maxSamplerAnisotropy;
    vk.caps.limits.max_viewports = gpu_props.limits.maxViewports;
    vk.caps.limits.max_viewport_width = gpu_props.limits.maxViewportDimensions[0];
    vk.caps.limits.max_viewport_height = gpu_props.limits.maxViewportDimensions[1];
    vk.caps.limits.max_tessellation_patch_size = gpu_props.limits.maxTessellationPatchSize;
    vk.caps.limits.point_size_range_min = gpu_props.limits.pointSizeRange[0];
    vk.caps.limits.point_size_range_max = gpu_props.limits.pointSizeRange[1];
    vk.caps.limits.line_width_range_min = gpu_props.limits.lineWidthRange[0];
    vk.caps.limits.line_width_range_max = gpu_props.limits.lineWidthRange[1];
    vk.caps.limits.max_compute_shared_memory_size = gpu_props.limits.maxComputeSharedMemorySize;
    vk.caps.limits.max_compute_work_group_count_x = gpu_props.limits.maxComputeWorkGroupCount[0];
    vk.caps.limits.max_compute_work_group_count_y = gpu_props.limits.maxComputeWorkGroupCount[1];
    vk.caps.limits.max_compute_work_group_count_z = gpu_props.limits.maxComputeWorkGroupCount[2];
    vk.caps.limits.max_compute_work_group_invocations = gpu_props.limits.maxComputeWorkGroupInvocations;
    vk.caps.limits.max_compute_work_group_size_x = gpu_props.limits.maxComputeWorkGroupSize[0];
    vk.caps.limits.max_compute_work_group_size_y = gpu_props.limits.maxComputeWorkGroupSize[1];
    vk.caps.limits.max_compute_work_group_size_z = gpu_props.limits.maxComputeWorkGroupSize[2];


    vk.max_inflight_frames = 2u;
    {
        // Frame state
        vk.frame = &vk.frames[0];

        const VkSemaphoreCreateInfo semaphoreInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0
        };

        for (uint32_t i = 0; i < vk.max_inflight_frames; i++)
        {
            vk.frames[i].index = i;

            const VkCommandPoolCreateInfo command_pool_info = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .pNext = NULL,
                .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                .queueFamilyIndex = vk.queue_families.graphics_queue_family
            };

            if (vkCreateCommandPool(vk.device, &command_pool_info, NULL, &vk.frames[i].command_pool) != VK_SUCCESS)
            {
                gpu_shutdown();
                return false;
            }

            const VkFenceCreateInfo fence_info = {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .pNext = NULL,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT
            };

            if (vkCreateFence(vk.device, &fence_info, NULL, &vk.frames[i].fence) != VK_SUCCESS)
            {
                gpu_shutdown();
                return false;
            }

            if (vkCreateSemaphore(vk.device, &semaphoreInfo, NULL, &vk.frames[i].imageAvailableSemaphore) != VK_SUCCESS)
            {
                gpu_shutdown();
                return false;
            }

            if (vkCreateSemaphore(vk.device, &semaphoreInfo, NULL, &vk.frames[i].renderCompleteSemaphore) != VK_SUCCESS)
            {
                gpu_shutdown();
                return false;
            }
        }
    }

    return true;
}

static void vk_shutdown(void) {
    /* Destroy swap chains.*/
    /*for (uint32_t i = 0; i < _VGPU_VK_MAX_SWAPCHAINS; i++)
    {
        if (renderer->swapchains[i].handle == VK_NULL_HANDLE)
            continue;

        vgpuVkSwapchainDestroy(device->renderer, &renderer->swapchains[i]);
    }*/

    /* Destroy hashed objects */
    {
        for (uint32_t i = 0; i < hmlenu(vk.renderPassHashMap); i++)
        {
            vkDestroyRenderPass(
                vk.device,
                vk.renderPassHashMap[i].value,
                NULL
            );
        }
    }

    /* Destroy frame data. */
    for (uint32_t i = 0; i < vk.max_inflight_frames; i++)
    {
        gpu_vk_frame* frame = &vk.frames[i];

        _gpu_vk_process_destroy(frame);
        free(frame->free_list.data);

        if (frame->fence) {
            vkDestroyFence(vk.device, frame->fence, NULL);
        }

        if (frame->imageAvailableSemaphore) {
            vkDestroySemaphore(vk.device, frame->imageAvailableSemaphore, NULL);
        }

        if (frame->renderCompleteSemaphore) {
            vkDestroySemaphore(vk.device, frame->renderCompleteSemaphore, NULL);
        }

        if (frame->command_pool) {
            vkDestroyCommandPool(vk.device, frame->command_pool, NULL);
        }
    }

    if (vk.allocator != VK_NULL_HANDLE)
    {
        VmaStats stats;
        vmaCalculateStats(vk.allocator, &stats);

        if (stats.total.usedBytes > 0) {
            //vgpu_log_format(GPU_LOG_LEVEL_ERROR, "Total device memory leaked: %llx bytes.", stats.total.usedBytes);
        }

        vmaDestroyAllocator(vk.allocator);
    }

    if (vk.device != VK_NULL_HANDLE) {
        vkDestroyDevice(vk.device, NULL);
    }

    if (vk.debug_utils_messenger != VK_NULL_HANDLE)
    {
        vkDestroyDebugUtilsMessengerEXT(vk.instance, vk.debug_utils_messenger, NULL);
    }
    else if (vk.debug_report_callback != VK_NULL_HANDLE)
    {
        vkDestroyDebugReportCallbackEXT(vk.instance, vk.debug_report_callback, NULL);
    }

    if (vk.instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(vk.instance, NULL);
    }

    vk.instance = VK_NULL_HANDLE;
}

static vgpu_caps vk_query_caps(void) {
    return vk.caps;
}

static VGPURenderPass vk_get_default_render_pass(void) {
    return nullptr;
}

bool _vgpu_vkimage_format_is_supported(VkFormat format, VkFormatFeatureFlags required, VkImageTiling tiling)
{
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(vk.physical_device, format, &props);
    VkFormatFeatureFlags flags = tiling == VK_IMAGE_TILING_OPTIMAL ? props.optimalTilingFeatures : props.linearTilingFeatures;
    return (flags & required) == required;
}

static vgpu_pixel_format vk_get_default_depth_format(void)
{
    if (_vgpu_vkimage_format_is_supported(VK_FORMAT_D32_SFLOAT, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL))
    {
        return VGPU_PIXELFORMAT_DEPTH32_FLOAT;
    }

    //if (_vgpu_vkimage_format_is_supported(renderer, VK_FORMAT_X8_D24_UNORM_PACK32, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL))
    //    return VK_FORMAT_X8_D24_UNORM_PACK32;
    if (_vgpu_vkimage_format_is_supported(VK_FORMAT_D16_UNORM, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL))
    {
        return VGPU_PIXELFORMAT_DEPTH16_UNORM;
    }

    return VGPU_PIXELFORMAT_UNDEFINED;
}

static vgpu_pixel_format vk_get_default_depth_stencil_format(void)
{
    if (_vgpu_vkimage_format_is_supported(VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL))
    {
        return VGPU_PIXELFORMAT_DEPTH24_PLUS;
    }

    if (_vgpu_vkimage_format_is_supported(VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL))
    {
        return VGPU_PIXELFORMAT_DEPTH24_PLUS_STENCIL8;
    }

    return VGPU_PIXELFORMAT_UNDEFINED;
}

static void vk_wait_idle(void) {
    VK_CHECK(vkDeviceWaitIdle(vk.device));
}

static void vk_begin_frame(void) {
    VK_CHECK(vkWaitForFences(vk.device, 1, &vk.frame->fence, VK_FALSE, ~0ull));
    VK_CHECK(vkResetFences(vk.device, 1, &vk.frame->fence));
    _gpu_vk_process_destroy(vk.frame);

    if (!vk.headless)
    {
        /*VkResult result = renderer->api.vkAcquireNextImageKHR(
            renderer->device,
            renderer->swapchains[0].handle,
            UINT64_MAX,
            renderer->frame->imageAvailableSemaphore,
            VK_NULL_HANDLE,
            &renderer->swapchains[0].imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR ||
            result == VK_SUBOPTIMAL_KHR)
        {
        }
        else
        {
            VK_CHECK(result);
        }*/
    }

    /*const VkCommandBufferBeginInfo beginfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    VK_CHECK(renderer->api.vkBeginCommandBuffer(renderer->frame->commandBuffer, &beginfo));*/
}


static void vk_end_frame(void) {

    /*VGPURendererVk* renderer = (VGPURendererVk*)driver_data;

    uint32_t imageIndex = renderer->swapchains[0].imageIndex;
    vgpuVkTextureBarrier(driver_data,
        renderer->frame->commandBuffer,
        renderer->swapchains[0].backbufferTextures[imageIndex],
        VGPUTextureLayout_Present
    );

    VK_CHECK(renderer->api.vkEndCommandBuffer(renderer->frame->commandBuffer));*/

    const VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    uint32_t wait_sem_count = 0u;
    VkSemaphore* wait_sems = NULL;
    const VkPipelineStageFlags* wait_stage_masks = NULL;

    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = wait_sem_count,
        .pWaitSemaphores = wait_sems,
        .pWaitDstStageMask = wait_stage_masks,
        .pCommandBuffers = NULL, // &renderer->frame->commandBuffer,
        .commandBufferCount = 0u,
        .signalSemaphoreCount = 0u,
        .pSignalSemaphores = NULL // &renderer->frame->renderCompleteSemaphore
    };

    VK_CHECK(vkQueueSubmit(vk.graphics_queue, 1, &submit_info, vk.frame->fence));

    if (!vk.headless) {
        /* Present swap chains. */
        /*const VkPresentInfoKHR presentInfo = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = NULL,
            .waitSemaphoreCount = 1u,
            .pWaitSemaphores = &renderer->frame->renderCompleteSemaphore,
            .swapchainCount = 1u,
            .pSwapchains = &renderer->swapchains[0].handle,
            .pImageIndices = &renderer->swapchains[0].imageIndex,
            .pResults = NULL
        };

        VkResult result = renderer->api.vkQueuePresentKHR(renderer->graphics_queue, &presentInfo);
        if (!((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR)))
        {
            if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                //windowResize();
                return;
            }
            else {
                VK_CHECK(result);
            }
        }*/
    }

    /* Advance to next frame. */
    vk.frame = &vk.frames[(vk.frame->index + 1) % vk.max_inflight_frames];
}

bool gpu_vk_supported(void) {
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

    GPU_FOREACH_ANONYMOUS(GPU_LOAD_ANONYMOUS);

    uint32_t ext_count;
    VkResult result = vkEnumerateInstanceExtensionProperties(NULL, &ext_count, NULL);
    if (result != VK_SUCCESS)
    {
        vk.available = false;
        return false;
    }

    vk.available = true;
    return true;
}

gpu_renderer* vk_gpu_create_renderer(void) {
    static gpu_renderer renderer = { NULL };
    ASSIGN_DRIVER(vk);
    return &renderer;
}

#endif /* defined(GPU_VK_BACKEND) */
