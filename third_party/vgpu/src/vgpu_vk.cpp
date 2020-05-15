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

#if defined(VGPU_BACKEND_VULKAN)
#include "vgpu_backend.h"
#include "stb_ds.h"

#if defined(_WIN32) || defined(_WIN64)
#   define   VK_SURFACE_EXT             "VK_KHR_win32_surface"
#   define   VK_CREATE_SURFACE_FN        vkCreateWin32SurfaceKHR
#   define   VK_CREATE_SURFACE_FN_TYPE   PFN_vkCreateWin32SurfaceKHR
#   define   VK_USE_PLATFORM_WIN32_KHR
#   define NOMINMAX
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#elif defined(__ANDROID__)
#   define   VK_SURFACE_EXT             "VK_KHR_android_surface"
#   define   VK_CREATE_SURFACE_FN        vkCreateAndroidSurfaceKHR
#   define   VK_CREATE_SURFACE_FN_TYPE   PFN_vkCreateAndroidSurfaceKHR
#   define   VK_USE_PLATFORM_ANDROID_KHR
#else
#   include <xcb/xcb.h>
#   include <dlfcn.h>
#   include <X11/Xlib-xcb.h>
#   define   VK_SURFACE_EXT             "VK_KHR_xcb_surface"
#   define   VK_CREATE_SURFACE_FN        vkCreateXcbSurfaceKHR
#   define   VK_CREATE_SURFACE_FN_TYPE   PFN_vkCreateXcbSurfaceKHR
#   define   VK_USE_PLATFORM_XCB_KHR
#endif

#ifndef VULKAN_H_
#define VULKAN_H_ 1
#endif
#define VKBIND_IMPLEMENTATION
#include "vkbind/vkbind.h"
#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

#define _VK_GPU_MAX_PHYSICAL_DEVICES (32u)
#define VK_CHECK(res) do { VkResult r = (res); VGPU_CHECK(r >= 0, vkGetErrorString(r)); } while (0)

#if !defined(NDEBUG) || defined(DEBUG) || defined(_DEBUG)
#   define VULKAN_DEBUG
#endif

#ifndef BGFX_CONFIG_MAX_VERTEX_BUFFERS
#	define BGFX_CONFIG_MAX_VERTEX_BUFFERS (4<<10)
#endif // BGFX_CONFIG_MAX_VERTEX_BUFFERS

typedef struct {
    bool swapchain;
    bool maintenance_1;
    bool maintenance_2;
    bool maintenance_3;
    bool get_memory_requirements2;
    bool dedicated_allocation;
    bool image_format_list;
    bool debug_marker;
} vk_physical_device_features;

typedef struct VGPUVkQueueFamilyIndices {
    uint32_t graphics_queue_family;
    uint32_t compute_queue_family;
    uint32_t copy_queue_family;
} VGPUVkQueueFamilyIndices;

struct VGPUSwapchainVk {
    enum { MAX_COUNT = 16 };

    VkSurfaceKHR surface;
    VkSwapchainKHR handle;

    uint32_t preferredImageCount;
    uint32_t width;
    uint32_t height;
    VkPresentModeKHR    presentMode;
    VGPUTextureFormat   colorFormat;
    uint32_t imageIndex;
    uint32_t imageCount;
    VGPUTexture backbufferTextures[4];
};

struct VGPUBufferVk {
    enum { MAX_COUNT = 1024 };

    VkBuffer handle;
    VmaAllocation memory;
};

struct VGPUTextureVk {
    enum { MAX_COUNT = 2048 };

    uint64_t cookie;
    VkFormat format;
    VkImage handle;
    VkImageView view;
    VmaAllocation allocation;
    VGPUTextureDescriptor desc;
    VGPUTextureLayout layout;
};

struct VGPUSamplerVk {
    enum { MAX_COUNT = 2048 };

    VkSampler handle;
};

struct VGPUFramebufferVk {
    VkFramebuffer handle;
    uint32_t width;
    uint32_t height;
    uint32_t attachmentCount;
    VGPUTexture attachments[VGPU_MAX_COLOR_ATTACHMENTS + 1];
};

struct VGPURenderPassVk {
    VkRenderPass renderPass;
    VkFramebuffer framebuffer;
    VkRect2D renderArea;
    uint32_t attachmentCount;
    VGPUTexture textures[VGPU_MAX_COLOR_ATTACHMENTS + 1];
    VkClearValue clears[VGPU_MAX_COLOR_ATTACHMENTS + 1];
};

typedef struct {
    VkObjectType type;
    void* handle1;
    void* handle2;
} VGPUVkObjectRef;

typedef struct {
    VGPUVkObjectRef* data;
    size_t capacity;
    size_t length;
} VGPUVkFreeList;

typedef struct {
    uint32_t index;
    VkFence fence;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderCompleteSemaphore;
    VkCommandBuffer commandBuffer;
    VGPUVkFreeList freeList;
} VGPUVkFrame;

struct RenderPassHashMap {
    vgpu::Hash key;
    VkRenderPass value;
};

struct FramebufferHashMap {
    vgpu::Hash key;
    VGPUFramebufferVk value;
};

static struct {
    bool available_initialized;
    bool available;
    VkbAPI api;

    uint32_t apiVersion;
    bool debug_utils;
    bool headless;
    bool surface_capabilities2;
    bool physical_device_properties2;
    bool external_memory_capabilities;
    bool external_semaphore_capabilities;
    bool full_screen_exclusive;
    VkInstance instance;

    VkDebugUtilsMessengerEXT debug_utils_messenger;

    uint32_t physical_device_count;
    VkPhysicalDevice physical_devices[_VK_GPU_MAX_PHYSICAL_DEVICES];


    bool validation;
    VkPhysicalDevice physical_device;
    VGPUVkQueueFamilyIndices queue_families;

    bool api_version_12;
    vk_physical_device_features device_features;

    vgpu_caps caps;

    VkDevice device;
    struct vk_device_table* table;
    VkQueue graphics_queue;
    VkQueue compute_queue;
    VkQueue copy_queue;
    VmaAllocator allocator;
    VkCommandPool commandPool;

    VGPUVkFrame frames[3];
    VGPUVkFrame* frame;
    uint32_t max_inflight_frames;

    uint64_t cookie = 0;
    vgpu::Pool<VGPUTextureVk, VGPUTextureVk::MAX_COUNT> textures;
    vgpu::Pool<VGPUBufferVk, VGPUBufferVk::MAX_COUNT> buffers;
    vgpu::Pool<VGPUSamplerVk, VGPUSamplerVk::MAX_COUNT> samplers;
    VGPUSwapchainVk swapchains[VGPUSwapchainVk::MAX_COUNT];

    RenderPassHashMap* renderPassHashMap;
    FramebufferHashMap* framebufferHashMap;
} vk;

static uint64_t vk_AllocateCookie(void) {
    vk.cookie += 16;
    return vk.cookie;
}

#if defined(VULKAN_DEBUG)
VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
{
    // Log debug messge
    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        vgpu_log(VGPU_LOG_LEVEL_WARN, "%u - %s: %s", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
    }
    else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        vgpu_log(VGPU_LOG_LEVEL_ERROR, "%u - %s: %s", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
    }

    return VK_FALSE;
}

#endif

static const char* vkGetErrorString(VkResult result) {
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
        else if (strcmp(available_extensions[i].extensionName, "VK_KHR_maintenance1") == 0) {
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
            vk.full_screen_exclusive = true;
        }
    }

    return result;
}

static bool vgpuQueryPresentationSupport(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex)
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    return vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, queueFamilyIndex);
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
            present_support = vgpuQueryPresentationSupport(physical_device, i);
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

static bool vgpuVkIsDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, bool headless)
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
    if (!headless && !features.swapchain) {
        return false;
    }

    /* We required maintenance_1 to support viewport flipping to match DX style. */
    if (!features.maintenance_1) {
        return false;
    }

    return true;
}

static bool vk_create_surface(uintptr_t native_handle, VkSurfaceKHR* pSurface) {
    VkResult result = VK_SUCCESS;

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    HWND hWnd = (HWND)native_handle;

    VkWin32SurfaceCreateInfoKHR surfaceInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
    surfaceInfo.hinstance = GetModuleHandle(NULL);
    surfaceInfo.hwnd = hWnd;
#endif
    result = vk.api.VK_CREATE_SURFACE_FN(vk.instance, &surfaceInfo, NULL, pSurface);
    if (result != VK_SUCCESS) {
        vgpu_log(VGPU_LOG_LEVEL_ERROR, "Failed to create surface");
        return false;
    }

    return true;
}

/* Conversion functions */
static inline VkFormat GetVkFormat(VGPUTextureFormat format)
{
    static VkFormat formats[VGPUTextureFormat_Count] = {
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

static VkCompareOp getVkCompareOp(VGPUCompareFunction function, VkCompareOp defaultValue)
{
    switch (function)
    {
    case VGPUCompareFunction_Never:
        return VK_COMPARE_OP_NEVER;
    case VGPUCompareFunction_Less:
        return VK_COMPARE_OP_LESS;
    case VGPUCompareFunction_LessEqual:
        return VK_COMPARE_OP_LESS_OR_EQUAL;
    case VGPUCompareFunction_Greater:
        return VK_COMPARE_OP_GREATER;
    case VGPUCompareFunction_GreaterEqual:
        return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case VGPUCompareFunction_Equal:
        return VK_COMPARE_OP_EQUAL;
    case VGPUCompareFunction_NotEqual:
        return VK_COMPARE_OP_NOT_EQUAL;
    case VGPUCompareFunction_Always:
        return VK_COMPARE_OP_ALWAYS;

    default:
        return defaultValue;
    }
}

/* Helper functions .*/
static void vgpuVkDeferredDestroy(void* handle1, void* handle2, VkObjectType type)
{
    VGPUVkFreeList* freelist = &vk.frame->freeList;

    if (freelist->length >= freelist->capacity) {
        freelist->capacity = freelist->capacity ? (freelist->capacity * 2) : 1;
        freelist->data = (VGPUVkObjectRef*)realloc(freelist->data, freelist->capacity * sizeof(*freelist->data));
        VGPU_CHECK(freelist->data, "Out of memory");
    }

    freelist->data[freelist->length++] = { type, handle1, handle2 };
}

static void vgpuVkProcessDeferredDestroy(VGPUVkFrame* frame)
{
    for (size_t i = 0; i < frame->freeList.length; i++)
    {
        VGPUVkObjectRef* ref = &frame->freeList.data[i];
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

        case VK_OBJECT_TYPE_PIPELINE:
            vkDestroyPipeline(vk.device, (VkPipeline)ref->handle1, NULL);
            break;
        default:
            VGPU_UNREACHABLE();
            break;
        }
    }

    frame->freeList.length = 0;
}

static void _vgpu_vk_set_name(VkObjectType objectType, uint64_t objectHandle, const char* objectName)
{
    if (!vk.debug_utils)
    {
        return;
    }

    VkDebugUtilsObjectNameInfoEXT nameInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
    nameInfo.objectType = objectType;
    nameInfo.objectHandle = objectHandle;
    nameInfo.pObjectName = objectName;

    VK_CHECK(vkSetDebugUtilsObjectNameEXT(vk.device, &nameInfo));
}

/* Barriers */
static VkAccessFlags vgpuVkGetAccessMask(VGPUTextureLayout state, VkImageAspectFlags aspectMask)
{
    switch (state)
    {
    case VGPUTextureLayout_Undefined:
    case VGPUTextureLayout_General:
    case VGPUTextureLayout_Present:
        //case VGPUTextureLayout_PreInitialized:
        return (VkAccessFlagBits)0;
    case VGPUTextureLayout_RenderTarget:
        if (aspectMask == VK_IMAGE_ASPECT_COLOR_BIT)
        {
            return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        }

        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    case VGPUTextureLayout_ShaderRead:
        return VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;

    case VGPUTextureLayout_ShaderWrite:
        return VK_ACCESS_SHADER_WRITE_BIT;

        /*case VGPUTextureLayout_ResolveDest:
        case VGPUTextureLayout_CopyDest:
            return VK_ACCESS_TRANSFER_WRITE_BIT;
        case VGPUTextureLayout_ResolveSource:
        case VGPUTextureLayout_CopySource:
            return VK_ACCESS_TRANSFER_READ_BIT;*/

    default:
        VGPU_UNREACHABLE();
        return (VkAccessFlagBits)-1;
    }
}

VkImageLayout vgpuVkGetImageLayout(VGPUTextureLayout layout, VkImageAspectFlags aspectMask)
{
    switch (layout)
    {
    case VGPUTextureLayout_Undefined:
        return VK_IMAGE_LAYOUT_UNDEFINED;

    case VGPUTextureLayout_General:
        return VK_IMAGE_LAYOUT_GENERAL;

    case VGPUTextureLayout_RenderTarget:
        if (aspectMask == VK_IMAGE_ASPECT_COLOR_BIT) {
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        //case VGPUTextureLayout_DepthStencilReadOnly:
        //    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    case VGPUTextureLayout_ShaderRead:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    case VGPUTextureLayout_ShaderWrite:
        return VK_IMAGE_LAYOUT_GENERAL;

    case VGPUTextureLayout_Present:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    default:
        _VGPU_UNREACHABLE();
        return (VkImageLayout)-1;
    }
}

static VkPipelineStageFlags vgpuVkGetShaderStageMask(VGPUTextureLayout layout, VkImageAspectFlags aspectMask, bool src)
{
    switch (layout)
    {
    case VGPUTextureLayout_Undefined:
    case VGPUTextureLayout_General:
        assert(src);
        return src ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : (VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    case VGPUTextureLayout_ShaderRead:
    case VGPUTextureLayout_ShaderWrite:
        return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT; // #OPTME Assume the worst
    case VGPUTextureLayout_RenderTarget:
        if (aspectMask == VK_IMAGE_ASPECT_COLOR_BIT)
        {
            return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        }

        return src ? VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT : VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        /*case Resource::State::IndirectArg:
            return VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        case VGPUTextureLayout_CopyDest:
        case VGPUTextureLayout_CopySource:
        case VGPUTextureLayout_ResolveDest:
        case VGPUTextureLayout_ResolveSource:
            return VK_PIPELINE_STAGE_TRANSFER_BIT;*/
    case VGPUTextureLayout_Present:
        return src ? (VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    default:
        _VGPU_UNREACHABLE();
        return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    }
}

static void vgpuVkTextureBarrier(VkCommandBuffer commandBuffer, VGPUTexture handle, VGPUTextureLayout newState)
{
    VGPUTextureVk& texture = vk.textures[handle.id];

    if (texture.layout == newState) {
        return;
    }

    const VkImageAspectFlags aspectMask = GetVkAspectMask(texture.format);

    // Create an image barrier object
    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.pNext = nullptr;
    barrier.srcAccessMask = vgpuVkGetAccessMask(texture.layout, aspectMask);
    barrier.dstAccessMask = vgpuVkGetAccessMask(newState, aspectMask);
    barrier.oldLayout = vgpuVkGetImageLayout(texture.layout, aspectMask);
    barrier.newLayout = vgpuVkGetImageLayout(newState, aspectMask);
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = texture.handle;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    vkCmdPipelineBarrier(
        commandBuffer,
        vgpuVkGetShaderStageMask(texture.layout, aspectMask, true),
        vgpuVkGetShaderStageMask(newState, aspectMask, false),
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    texture.layout = newState;
}

/* Swapchain */
#define _VGPU_VK_MAX_SURFACE_FORMATS (32u)
#define _VGPU_VK_MAX_PRESENT_MODES (16u)

static VkPresentModeKHR vgpuVkGetPresentMode(VGPUPresentMode value) {
    switch (value)
    {
    case VGPUPresentMode_Mailbox:
        return VK_PRESENT_MODE_MAILBOX_KHR;

    case VGPUPresentMode_Immediate:
        return VK_PRESENT_MODE_IMMEDIATE_KHR;

    case VGPUPresentMode_Fifo:
    default:
        return VK_PRESENT_MODE_FIFO_KHR;
    }
}

typedef struct VGPUVkSurfaceCaps {
    bool success;
    VkSurfaceCapabilitiesKHR capabilities;
    uint32_t formatCount;
    uint32_t presentModeCount;
    VkSurfaceFormatKHR formats[_VGPU_VK_MAX_SURFACE_FORMATS];
    VkPresentModeKHR present_modes[_VGPU_VK_MAX_PRESENT_MODES];
} VGPUVkSurfaceCaps;

static VGPUVkSurfaceCaps vgpuVkQuerySwapchainSupport(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    VGPUVkSurfaceCaps caps;
    memset(&caps, 0, sizeof(VGPUVkSurfaceCaps));
    caps.formatCount = _VGPU_VK_MAX_SURFACE_FORMATS;
    caps.presentModeCount = _VGPU_VK_MAX_PRESENT_MODES;

    VkPhysicalDeviceSurfaceInfo2KHR surface_info = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR };
    surface_info.surface = surface;

    if (vk.surface_capabilities2)
    {
        VkSurfaceCapabilities2KHR surface_caps2 = { VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR };

        if (vk.api.vkGetPhysicalDeviceSurfaceCapabilities2KHR(physical_device, &surface_info, &surface_caps2) != VK_SUCCESS)
        {
            return caps;
        }
        caps.capabilities = surface_caps2.surfaceCapabilities;

        if (vk.api.vkGetPhysicalDeviceSurfaceFormats2KHR(physical_device, &surface_info, &caps.formatCount, NULL) != VK_SUCCESS)
        {
            return caps;
        }

        VkSurfaceFormat2KHR* formats2 = VGPU_ALLOCA(VkSurfaceFormat2KHR, caps.formatCount);
        VGPU_ASSERT(formats2);

        for (uint32_t i = 0; i < caps.formatCount; ++i)
        {
            memset(&formats2[i], 0, sizeof(VkSurfaceFormat2KHR));
            formats2[i].sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
        }

        if (vk.api.vkGetPhysicalDeviceSurfaceFormats2KHR(physical_device, &surface_info, &caps.formatCount, formats2) != VK_SUCCESS)
        {
            return caps;
        }

        for (uint32_t i = 0; i < caps.formatCount; ++i)
        {
            caps.formats[i] = formats2[i].surfaceFormat;
        }
    }
    else
    {
        if (vk.api.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &caps.capabilities) != VK_SUCCESS)
        {
            return caps;
        }

        if (vk.api.vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &caps.formatCount, caps.formats) != VK_SUCCESS)
        {
            return caps;
        }
    }

#ifdef _WIN32
    if (vk.surface_capabilities2 && vk.full_screen_exclusive)
    {
        if (vk.api.vkGetPhysicalDeviceSurfacePresentModes2EXT(physical_device, &surface_info, &caps.presentModeCount, caps.present_modes) != VK_SUCCESS)
        {
            return caps;
        }
    }
    else
#endif
    {
        if (vk.api.vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &caps.presentModeCount, caps.present_modes) != VK_SUCCESS)
        {
            return caps;
        }
    }

    caps.success = true;
    return caps;
}

static void vgpuVkSwapchainDestroy(VGPUSwapchainVk* swapchain);

static bool vgpuVkSwapchainInit(VGPUSwapchainVk* swapchain)
{
    VGPUVkSurfaceCaps surface_caps = vgpuVkQuerySwapchainSupport(vk.physical_device, swapchain->surface);

    VkSwapchainKHR oldSwapchain = swapchain->handle;

    /* Detect image count. */
    uint32_t image_count = swapchain->preferredImageCount;
    if (image_count == 0)
    {
        image_count = surface_caps.capabilities.minImageCount + 1;
        if ((surface_caps.capabilities.maxImageCount > 0) &&
            (image_count > surface_caps.capabilities.maxImageCount))
        {
            image_count = surface_caps.capabilities.maxImageCount;
        }
    }
    else
    {
        if (surface_caps.capabilities.maxImageCount != 0)
            image_count = _vgpu_min(image_count, surface_caps.capabilities.maxImageCount);
        image_count = _vgpu_max(image_count, surface_caps.capabilities.minImageCount);
    }

    /* Extent */
    VkExtent2D swapchain_size = { swapchain->width, swapchain->height };
    if (swapchain_size.width < 1 || swapchain_size.height < 1)
    {
        swapchain_size = surface_caps.capabilities.currentExtent;
    }
    else
    {
        swapchain_size.width = _vgpu_max(swapchain_size.width, surface_caps.capabilities.minImageExtent.width);
        swapchain_size.width = _vgpu_min(swapchain_size.width, surface_caps.capabilities.maxImageExtent.width);
        swapchain_size.height = _vgpu_max(swapchain_size.height, surface_caps.capabilities.minImageExtent.height);
        swapchain_size.height = _vgpu_min(swapchain_size.height, surface_caps.capabilities.maxImageExtent.height);
    }

    /* Surface format. */
    VkSurfaceFormatKHR format;
    if (surface_caps.formatCount == 1 &&
        surface_caps.formats[0].format == VK_FORMAT_UNDEFINED)
    {
        format = surface_caps.formats[0];
        format.format = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else
    {
        if (surface_caps.formatCount == 0)
        {
            vgpu_log(VGPU_LOG_LEVEL_ERROR, "Vulkan: Surface has no formats.");
            return false;
        }

        const bool srgb = false;
        bool found = false;
        for (uint32_t i = 0; i < surface_caps.formatCount; i++)
        {
            if (srgb)
            {
                if (surface_caps.formats[i].format == VK_FORMAT_R8G8B8A8_SRGB ||
                    surface_caps.formats[i].format == VK_FORMAT_B8G8R8A8_SRGB ||
                    surface_caps.formats[i].format == VK_FORMAT_A8B8G8R8_SRGB_PACK32)
                {
                    format = surface_caps.formats[i];
                    found = true;
                    break;
                }
            }
            else
            {
                if (surface_caps.formats[i].format == VK_FORMAT_R8G8B8A8_UNORM ||
                    surface_caps.formats[i].format == VK_FORMAT_B8G8R8A8_UNORM ||
                    surface_caps.formats[i].format == VK_FORMAT_A8B8G8R8_UNORM_PACK32)
                {
                    format = surface_caps.formats[i];
                    found = true;
                    break;
                }
            }
        }

        if (!found)
        {
            format = surface_caps.formats[0];
        }
    }

    VGPUTextureUsageFlags textureUsage = VGPUTextureUsage_OutputAttachment;

    VkImageUsageFlags image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Enable transfer source on swap chain images if supported
    if (surface_caps.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
        image_usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        //textureUsage |= VGPUTextureUsage_CopySrc;
    }

    // Enable transfer destination on swap chain images if supported
    if (surface_caps.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
        image_usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        //textureUsage |= VGPUTextureUsage_CopyDst;
    }

    VkSurfaceTransformFlagBitsKHR pre_transform;
    if ((surface_caps.capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) != 0)
        pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    else
        pre_transform = surface_caps.capabilities.currentTransform;

    VkCompositeAlphaFlagBitsKHR composite_mode = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if (surface_caps.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
        composite_mode = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    if (surface_caps.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
        composite_mode = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if (surface_caps.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
        composite_mode = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
    if (surface_caps.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
        composite_mode = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;

    bool present_mode_found = false;
    for (uint32_t i = 0; i < surface_caps.presentModeCount; i++) {
        if (surface_caps.present_modes[i] == swapchain->presentMode) {
            present_mode_found = true;
            break;
        }
    }

    if (!present_mode_found) {
        swapchain->presentMode = VK_PRESENT_MODE_FIFO_KHR;
    }

    /* We use same family for graphics and present so no sharing is necessary. */
    VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    createInfo.surface = swapchain->surface;
    createInfo.minImageCount = image_count;
    createInfo.imageFormat = format.format;
    createInfo.imageColorSpace = format.colorSpace;
    createInfo.imageExtent = swapchain_size;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = image_usage;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = NULL;
    createInfo.preTransform = pre_transform;
    createInfo.compositeAlpha = composite_mode;
    createInfo.presentMode = swapchain->presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = oldSwapchain;

    VkResult result = vkCreateSwapchainKHR(
        vk.device,
        &createInfo,
        NULL,
        &swapchain->handle);
    if (result != VK_SUCCESS) {
        vgpuVkSwapchainDestroy(swapchain);
        return false;
    }

    if (oldSwapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(vk.device, oldSwapchain, NULL);
    }

    // Obtain swapchain images.
    result = vk.api.vkGetSwapchainImagesKHR(vk.device, swapchain->handle, &swapchain->imageCount, NULL);
    if (result != VK_SUCCESS) {
        vgpuVkSwapchainDestroy(swapchain);
        return false;
    }

    VkImage* swapChainImages = VGPU_ALLOCA(VkImage, swapchain->imageCount);
    result = vk.api.vkGetSwapchainImagesKHR(vk.device, swapchain->handle, &swapchain->imageCount, swapChainImages);
    if (result != VK_SUCCESS) {
        vgpuVkSwapchainDestroy(swapchain);
        return false;
    }

    VGPUTextureDescriptor texture_desc = {};
    texture_desc.usage = textureUsage;
    texture_desc.dimension = VGPUTextureDimension_2D;
    texture_desc.size.width = swapchain_size.width;
    texture_desc.size.height = swapchain_size.height;
    texture_desc.size.depth = createInfo.imageArrayLayers;
    texture_desc.format = VGPUTextureFormat_BGRA8Unorm;
    texture_desc.mipLevelCount = 1u;
    texture_desc.sampleCount = 1u;

    VGPURenderPassDescriptor passDesc = {};

    for (uint32_t i = 0; i < swapchain->imageCount; i++)
    {
        texture_desc.externalHandle = (void*)swapChainImages[i];
        swapchain->backbufferTextures[i] = vgpuCreateTexture(&texture_desc);
    }

    return true;
}

static void vgpuVkSwapchainDestroy(VGPUSwapchainVk* swapchain)
{
    for (uint32_t i = 0; i < swapchain->imageCount; i++)
    {
        vgpuDestroyTexture(swapchain->backbufferTextures[i]);
    }

    if (swapchain->handle != VK_NULL_HANDLE)
    {
        vk.api.vkDestroySwapchainKHR(vk.device, swapchain->handle, NULL);
        swapchain->handle = VK_NULL_HANDLE;
    }

    if (swapchain->surface != VK_NULL_HANDLE)
    {
        vk.api.vkDestroySurfaceKHR(vk.instance, swapchain->surface, NULL);
        swapchain->surface = VK_NULL_HANDLE;
    }
}

static bool vk_init(const vgpu_config* info)
{
    if (!vgpu_vk_supported()) {
        return false;
    }

    bool validation = false;
#if defined(VULKAN_DEBUG)
    if (info->debug) {
        validation = true;
    }
#endif

    /* Setup instance only once. */
    if (!vk.instance)
    {
        uint32_t instance_extension_count;
        VK_CHECK(vk.api.vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, NULL));

        VkExtensionProperties* available_instance_extensions = VGPU_ALLOCA(VkExtensionProperties, instance_extension_count);
        VK_CHECK(vk.api.vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, available_instance_extensions));

        uint32_t enabled_ext_count = 0;
        char* enabled_exts[16];

        for (uint32_t i = 0; i < instance_extension_count; ++i)
        {
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

        if (info->swapchain_info == nullptr)
        {
            if (!vk.headless) {

            }
            else
            {
                enabled_exts[enabled_ext_count++] = VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME;
            }
        }
        else
        {
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
            VK_CHECK(vk.api.vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr));

            VkLayerProperties* supported_validation_layers = VGPU_ALLOCA(VkLayerProperties, instance_layer_count);
            assert(supported_validation_layers);
            VK_CHECK(vk.api.vkEnumerateInstanceLayerProperties(&instance_layer_count, supported_validation_layers));

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
        if (!vk.api.vkEnumerateInstanceVersion) {
            return false;
        }

        vk.apiVersion = 0;
        if (vk.api.vkEnumerateInstanceVersion(&vk.apiVersion) != VK_SUCCESS)
        {
            vk.apiVersion = VK_API_VERSION_1_1;
        }

        if (vk.apiVersion < VK_API_VERSION_1_1) {
            return false;
        }

        VkApplicationInfo applicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        applicationInfo.apiVersion = vk.apiVersion;

        VkInstanceCreateInfo instance_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        instance_info.pApplicationInfo = &applicationInfo;
        instance_info.enabledLayerCount = enabled_instance_layers_count;
        instance_info.ppEnabledLayerNames = enabled_instance_layers;
        instance_info.enabledExtensionCount = enabled_ext_count;
        instance_info.ppEnabledExtensionNames = enabled_exts;

#if defined(VULKAN_DEBUG)
        VkDebugUtilsMessengerCreateInfoEXT debug_utils_create_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
        if (vk.debug_utils)
        {
            debug_utils_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
            debug_utils_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
            debug_utils_create_info.pfnUserCallback = debug_utils_messenger_callback;

            instance_info.pNext = &debug_utils_create_info;
        }
#endif

        VkResult result = vk.api.vkCreateInstance(&instance_info, NULL, &vk.instance);
        if (result != VK_SUCCESS) {
            vgpu_shutdown();
            return false;
        }

        result = vkbInitInstanceAPI(vk.instance, &vk.api);
        if (result != VK_SUCCESS) {
            vgpu_log(VGPU_LOG_LEVEL_ERROR, "Failed to initialize instance API.");
            vgpu_shutdown();
            return false;
        }

#if defined(VULKAN_DEBUG)
        if (vk.debug_utils)
        {
            result = vk.api.vkCreateDebugUtilsMessengerEXT(vk.instance, &debug_utils_create_info, NULL, &vk.debug_utils_messenger);
            if (result != VK_SUCCESS)
            {
                vgpu_log(VGPU_LOG_LEVEL_ERROR, "Could not create debug utils messenger");
                vgpu_shutdown();
                return false;
            }
        }
#endif

        /* Enumerate all physical device. */
        vk.physical_device_count = _VK_GPU_MAX_PHYSICAL_DEVICES;
        result = vk.api.vkEnumeratePhysicalDevices(vk.instance, &vk.physical_device_count, vk.physical_devices);
        if (result != VK_SUCCESS) {
            vgpu_log(VGPU_LOG_LEVEL_ERROR, "Vulkan: Cannot enumerate physical devices.");
            vgpu_shutdown();
            return false;
        }
    }

    const bool headless = info->swapchain_info == nullptr;
    vk.validation = validation;

    /* Create surface if required. */
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (!headless)
    {
        if (!vk_create_surface(info->swapchain_info->handle, &surface))
        {
            vgpu_shutdown();
            return false;
        }
    }

    /* Find best supported physical device. */
    //const VGPUAdapterType preferredAdapter = VGPUAdapterType_DiscreteGPU;
    uint32_t best_device_score = 0U;
    uint32_t best_device_index = VK_QUEUE_FAMILY_IGNORED;
    for (uint32_t i = 0; i < vk.physical_device_count; ++i)
    {
        if (!vgpuVkIsDeviceSuitable(vk.physical_devices[i], surface, headless)) {
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
            score += 100U;
            //if (preferredAdapter == VGPUAdapterType_DiscreteGPU)
            {
                score += 1000u;
            }
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            score += 90U;
            /*if (preferredAdapter == VGPUAdapterType_IntegratedGPU) {
                score += 1000u;
            }*/
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

    vk.physical_device = vk.physical_devices[best_device_index];
    vk.queue_families = vgpuVkQueryQueueFamilies(vk.physical_device, surface);
    vk.device_features = vgpuVkQueryDeviceExtensionSupport(vk.physical_device);

    VkPhysicalDeviceProperties gpu_props;
    vkGetPhysicalDeviceProperties(vk.physical_device, &gpu_props);

    if (gpu_props.apiVersion >= VK_API_VERSION_1_2)
    {
        vk.api_version_12 = true;
    }

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
    VkDeviceQueueCreateInfo queue_info[3] = { };
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
    enabled_device_exts[enabled_device_ext_count++] = "VK_KHR_maintenance1";

    if (!headless) {
        enabled_device_exts[enabled_device_ext_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    }

    if (vk.device_features.maintenance_2) {
        enabled_device_exts[enabled_device_ext_count++] = "VK_KHR_maintenance2";
    }

    if (vk.device_features.maintenance_3) {
        enabled_device_exts[enabled_device_ext_count++] = "VK_KHR_maintenance3";
    }

    if (vk.device_features.get_memory_requirements2 &&
        vk.device_features.dedicated_allocation)
    {
        enabled_device_exts[enabled_device_ext_count++] = "VK_KHR_get_memory_requirements2";
        enabled_device_exts[enabled_device_ext_count++] = "VK_KHR_dedicated_allocation";
    }

#ifdef _WIN32
    if (vk.surface_capabilities2 &&
        vk.full_screen_exclusive)
    {
        enabled_device_exts[enabled_device_ext_count++] = "VK_EXT_full_screen_exclusive";
    }
#endif

    VkPhysicalDeviceFeatures2KHR features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR };
    vk.api.vkGetPhysicalDeviceFeatures2(vk.physical_device, &features);

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

    VkDeviceCreateInfo device_info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    device_info.queueCreateInfoCount = queue_family_count;
    device_info.pQueueCreateInfos = queue_info;
    device_info.enabledExtensionCount = enabled_device_ext_count;
    device_info.ppEnabledExtensionNames = enabled_device_exts;

    if (vk.physical_device_properties2)
        device_info.pNext = &features;
    else
        device_info.pEnabledFeatures = &features.features;

    VkResult result = vkCreateDevice(vk.physical_device, &device_info, NULL, &vk.device);
    if (result != VK_SUCCESS) {
        vgpu_shutdown();
        return false;
    }

    result = vkbInitDeviceAPI(vk.device, &vk.api);
    if (result != VK_SUCCESS) {
        vgpu_shutdown();
        return false;
    }

    vk.api.vkGetDeviceQueue(vk.device, vk.queue_families.graphics_queue_family, graphics_queue_index, &vk.graphics_queue);
    vk.api.vkGetDeviceQueue(vk.device, vk.queue_families.compute_queue_family, compute_queue_index, &vk.compute_queue);
    vk.api.vkGetDeviceQueue(vk.device, vk.queue_families.copy_queue_family, copy_queue_index, &vk.copy_queue);

    /* Init pools and hash maps */
    {
        vk.textures.init();
        vk.buffers.init();
        vk.samplers.init();
        hmdefault(vk.renderPassHashMap, nullptr);
        //hmdefault(vk.framebufferHashMap, NULL);
    }

    /* Create memory allocator. */
    {
        VmaAllocatorCreateFlags allocator_flags = 0;
        if (vk.device_features.get_memory_requirements2 &&
            vk.device_features.dedicated_allocation)
        {
            allocator_flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
        }

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.flags = allocator_flags;
        allocatorInfo.physicalDevice = vk.physical_device;
        allocatorInfo.device = vk.device;
        allocatorInfo.instance = vk.instance;
        allocatorInfo.vulkanApiVersion = vk.apiVersion;

        result = vmaCreateAllocator(&allocatorInfo, &vk.allocator);
        if (result != VK_SUCCESS) {
            vgpu_log(VGPU_LOG_LEVEL_ERROR, "Vulkan: Cannot create memory allocator.");
            vgpu_shutdown();
            return false;
        }
    }

    /* Init features and limits. */
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
    //vk.features.raytracing = vk.KHR_get_physical_device_properties2
    //    && vk.device_features.get_memory_requirements2
    //    || HasExtension(VK_NV_RAY_TRACING_EXTENSION_NAME);

    // Limits
    vk.caps.limits.max_vertex_attributes = gpu_props.limits.maxVertexInputAttributes;
    vk.caps.limits.max_vertex_bindings = gpu_props.limits.maxVertexInputBindings;
    vk.caps.limits.max_vertex_attribute_offset = gpu_props.limits.maxVertexInputAttributeOffset;
    vk.caps.limits.max_vertex_binding_stride = gpu_props.limits.maxVertexInputBindingStride;

    //vk.caps.limits.max_texture_size_1d = gpu_props.limits.maxImageDimension1D;
    vk.caps.limits.max_texture_size_2d = gpu_props.limits.maxImageDimension2D;
    vk.caps.limits.max_texture_size_3d = gpu_props.limits.maxImageDimension3D;
    vk.caps.limits.max_texture_size_cube = gpu_props.limits.maxImageDimensionCube;
    vk.caps.limits.max_texture_array_layers = gpu_props.limits.maxImageArrayLayers;
    vk.caps.limits.max_color_attachments = gpu_props.limits.maxColorAttachments;
    vk.caps.limits.max_uniform_buffer_size = gpu_props.limits.maxUniformBufferRange;
    vk.caps.limits.min_uniform_buffer_offset_alignment = gpu_props.limits.minUniformBufferOffsetAlignment;
    vk.caps.limits.max_storage_buffer_size = gpu_props.limits.maxStorageBufferRange;
    vk.caps.limits.min_storage_buffer_offset_alignment = gpu_props.limits.minStorageBufferOffsetAlignment;
    vk.caps.limits.maxSamplerAnisotropy = (uint32_t)gpu_props.limits.maxSamplerAnisotropy;
    vk.caps.limits.maxViewports = gpu_props.limits.maxViewports;
    vk.caps.limits.maxViewportWidth = gpu_props.limits.maxViewportDimensions[0];
    vk.caps.limits.maxViewportHeight = gpu_props.limits.maxViewportDimensions[1];
    vk.caps.limits.maxTessellationPatchSize = gpu_props.limits.maxTessellationPatchSize;
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

    /* Create main context and set it as active. */
    if (surface != VK_NULL_HANDLE)
    {
        vk.swapchains[0].surface = surface;
        vk.swapchains[0].width = info->swapchain_info->width;
        vk.swapchains[0].height = info->swapchain_info->height;
        vk.swapchains[0].colorFormat = info->swapchain_info->format;
        vk.swapchains[0].presentMode = vgpuVkGetPresentMode(info->swapchain_info->presentMode);

        if (!vgpuVkSwapchainInit(&vk.swapchains[0]))
        {
            vgpu_shutdown();
            return false;
        }
    }

    {
        VkCommandPoolCreateInfo commandPoolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolInfo.queueFamilyIndex = vk.queue_families.graphics_queue_family;

        if (vk.api.vkCreateCommandPool(vk.device, &commandPoolInfo, NULL, &vk.commandPool) != VK_SUCCESS)
        {
            vgpu_shutdown();
            return false;
        }
    }

    vk.max_inflight_frames = 2u;
    {
        // Frame state
        vk.frame = &vk.frames[0];

        VkCommandBufferAllocateInfo commandBufferInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        commandBufferInfo.commandPool = vk.commandPool;
        commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferInfo.commandBufferCount = 1;

        VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

        VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (uint32_t i = 0; i < vk.max_inflight_frames; i++)
        {
            vk.frames[i].index = i;

            if (vk.api.vkAllocateCommandBuffers(vk.device, &commandBufferInfo, &vk.frames[i].commandBuffer) != VK_SUCCESS)
            {
                vgpu_shutdown();
                return false;
            }

            if (vk.api.vkCreateFence(vk.device, &fenceInfo, NULL, &vk.frames[i].fence) != VK_SUCCESS)
            {
                vgpu_shutdown();
                return false;
            }

            if (vk.api.vkCreateSemaphore(vk.device, &semaphoreInfo, NULL, &vk.frames[i].imageAvailableSemaphore) != VK_SUCCESS)
            {
                vgpu_shutdown();
                return false;
            }

            if (vk.api.vkCreateSemaphore(vk.device, &semaphoreInfo, NULL, &vk.frames[i].renderCompleteSemaphore) != VK_SUCCESS)
            {
                vgpu_shutdown();
                return false;
            }
        }
    }

    return true;
}

static void vk_destroy(void)
{
    if (vk.device != VK_NULL_HANDLE) {
        VK_CHECK(vkDeviceWaitIdle(vk.device));
    }

    /* Destroy swap chains.*/
    for (uint32_t i = 0; i < VGPUSwapchainVk::MAX_COUNT; i++)
    {
        if (vk.swapchains[i].handle == VK_NULL_HANDLE)
            continue;

        //vgpuVkSwapchainDestroy(device->renderer, &vk.swapchains[i]);
    }

    /* Destroy hashed objects */
    {
        for (uint32_t i = 0; i < hmlenu(vk.framebufferHashMap); i++)
        {
            vk.api.vkDestroyFramebuffer(
                vk.device,
                vk.framebufferHashMap[i].value.handle,
                NULL
            );
        }

        for (uint32_t i = 0; i < hmlenu(vk.renderPassHashMap); i++)
        {
            vk.api.vkDestroyRenderPass(
                vk.device,
                vk.renderPassHashMap[i].value,
                NULL
            );
        }
    }

    /* Destroy frame data. */
    for (uint32_t i = 0; i < vk.max_inflight_frames; i++)
    {
        VGPUVkFrame* frame = &vk.frames[i];

        vgpuVkProcessDeferredDestroy(frame);
        free(frame->freeList.data);

        if (frame->fence) {
            vk.api.vkDestroyFence(vk.device, frame->fence, NULL);
        }

        if (frame->imageAvailableSemaphore) {
            vk.api.vkDestroySemaphore(vk.device, frame->imageAvailableSemaphore, NULL);
        }

        if (frame->renderCompleteSemaphore) {
            vk.api.vkDestroySemaphore(vk.device, frame->renderCompleteSemaphore, NULL);
        }

        if (frame->commandBuffer) {
            vk.api.vkFreeCommandBuffers(vk.device, vk.commandPool, 1, &frame->commandBuffer);
        }
    }

    if (vk.commandPool) {
        vk.api.vkDestroyCommandPool(vk.device, vk.commandPool, NULL);
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

    if (vk.device != VK_NULL_HANDLE) {
        vkDestroyDevice(vk.device, NULL);
    }

    if (vk.debug_utils_messenger != VK_NULL_HANDLE)
    {
        vk.api.vkDestroyDebugUtilsMessengerEXT(vk.instance, vk.debug_utils_messenger, NULL);
    }

    if (vk.instance != VK_NULL_HANDLE)
    {
        vk.api.vkDestroyInstance(vk.instance, NULL);
    }

    vkbUninit();
    vk.instance = VK_NULL_HANDLE;
}

static vgpu_backend_type vk_get_backend(void) {
    return VGPU_BACKEND_TYPE_VULKAN;
}

static vgpu_caps vk_get_caps(void)
{
    return vk.caps;
}

static bool _vgpu_vkImageFormatIsSupported(VkFormat format, VkFormatFeatureFlags required, VkImageTiling tiling)
{
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(vk.physical_device, format, &props);
    VkFormatFeatureFlags flags = tiling == VK_IMAGE_TILING_OPTIMAL ? props.optimalTilingFeatures : props.linearTilingFeatures;
    return (flags & required) == required;
}


static VGPUTextureFormat vk_GetDefaultDepthFormat(void)
{
    if (_vgpu_vkImageFormatIsSupported(VK_FORMAT_D32_SFLOAT, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL))
    {
        return VGPUTextureFormat_Depth32Float;
    }

    //if (_vgpu_vkImageFormatIsSupported(renderer, VK_FORMAT_X8_D24_UNORM_PACK32, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL))
    //    return VK_FORMAT_X8_D24_UNORM_PACK32;

    if (_vgpu_vkImageFormatIsSupported(VK_FORMAT_D16_UNORM, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL))
    {
        return VGPUTextureFormat_Depth16Unorm;
    }

    return VGPUTextureFormat_Undefined;
}

static VGPUTextureFormat vk_GetDefaultDepthStencilFormat(void)
{
    if (_vgpu_vkImageFormatIsSupported(VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL))
    {
        return VGPUTextureFormat_Depth24Plus;
    }

    if (_vgpu_vkImageFormatIsSupported(VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL))
    {
        return VGPUTextureFormat_Depth24PlusStencil8;
    }

    return VGPUTextureFormat_Undefined;
}

static VGPUTexture vk_getCurrentTexture(void)
{
    uint32_t imageIndex = vk.swapchains[0].imageIndex;
    return vk.swapchains[0].backbufferTextures[imageIndex];
}

static void vk_DeviceWaitIdle(void)
{
    VK_CHECK(vkDeviceWaitIdle(vk.device));
}

static void vk_begin_frame(void)
{
    VK_CHECK(vkWaitForFences(vk.device, 1, &vk.frame->fence, VK_FALSE, ~0ull));
    VK_CHECK(vkResetFences(vk.device, 1, &vk.frame->fence));
    vgpuVkProcessDeferredDestroy(vk.frame);

    VkResult result = vk.api.vkAcquireNextImageKHR(
        vk.device,
        vk.swapchains[0].handle,
        UINT64_MAX,
        vk.frame->imageAvailableSemaphore,
        VK_NULL_HANDLE,
        &vk.swapchains[0].imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR ||
        result == VK_SUBOPTIMAL_KHR)
    {
    }
    else
    {
        VK_CHECK(result);
    }

    VkCommandBufferBeginInfo beginfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vk.api.vkBeginCommandBuffer(vk.frame->commandBuffer, &beginfo));
}

static void vk_end_frame(void)
{
    uint32_t imageIndex = vk.swapchains[0].imageIndex;
    vgpuVkTextureBarrier(
        vk.frame->commandBuffer,
        vk.swapchains[0].backbufferTextures[imageIndex],
        VGPUTextureLayout_Present
    );

    VK_CHECK(vk.api.vkEndCommandBuffer(vk.frame->commandBuffer));

    const VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.waitSemaphoreCount = 1u;
    submitInfo.pWaitSemaphores = &vk.frame->imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = &waitDstStageMask;
    submitInfo.pCommandBuffers = &vk.frame->commandBuffer;
    submitInfo.commandBufferCount = 1u;
    submitInfo.signalSemaphoreCount = 1u;
    submitInfo.pSignalSemaphores = &vk.frame->renderCompleteSemaphore;

    VK_CHECK(vkQueueSubmit(vk.graphics_queue, 1, &submitInfo, vk.frame->fence));

    /* Present swap chains. */
    VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.waitSemaphoreCount = 1u;
    presentInfo.pWaitSemaphores = &vk.frame->renderCompleteSemaphore;
    presentInfo.swapchainCount = 1u;
    presentInfo.pSwapchains = &vk.swapchains[0].handle;
    presentInfo.pImageIndices = &vk.swapchains[0].imageIndex;
    presentInfo.pResults = NULL;

    VkResult result = vk.api.vkQueuePresentKHR(vk.graphics_queue, &presentInfo);
    if (!((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR)))
    {
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            //windowResize();
            return;
        }
        else {
            VK_CHECK(result);
        }
    }

    /* Advance to next frame. */
    vk.frame = &vk.frames[(vk.frame->index + 1) % vk.max_inflight_frames];
}

/* Buffer */
static VGPUBuffer vk_bufferCreate(const VGPUBufferDescriptor* descriptor)
{
    if (vk.buffers.isFull()) {
        return vgpu::kInvalidBuffer;
    }

    const int id = vk.buffers.alloc();
    VGPUBufferVk& buffer = vk.buffers[id];
    return { (uint32_t)id };
}

static void vk_bufferDestroy(VGPUBuffer handle)
{
    VGPUBufferVk& buffer = vk.buffers[handle.id];
    vgpuVkDeferredDestroy(buffer.handle, buffer.memory, VK_OBJECT_TYPE_BUFFER);
    vk.buffers.dealloc(handle.id);
}

/* Texture */
VkImageUsageFlags vgpu_vkGetImageUsage(VGPUTextureUsageFlags usage, VGPUTextureFormat format)
{
    VkImageUsageFlags flags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (usage & VGPUTextureUsage_Sampled) {
        flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }

    if (usage & VGPUTextureUsage_Storage) {
        flags |= VK_IMAGE_USAGE_STORAGE_BIT;
    }

    if (usage & VGPUTextureUsage_OutputAttachment) {
        if (vgpu_is_depth_stencil_format(format)) {
            flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        else {
            flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
    }

    return flags;
}

static VGPUTexture vk_create_texture(const VGPUTextureDescriptor* desc)
{
    if (vk.textures.isFull()) {
        return vgpu::kInvalidTexture;
    }

    const int id = vk.textures.alloc();
    VGPUTextureVk& texture = vk.textures[id];
    texture.format = GetVkFormat(desc->format);
    texture.cookie = vk_AllocateCookie();
    if (desc->externalHandle != nullptr)
    {
        texture.handle = (VkImage)desc->externalHandle;
    }
    else
    {
        VkImageCreateInfo createInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        createInfo.flags = 0u;
        createInfo.imageType = VK_IMAGE_TYPE_2D;
        createInfo.format = GetVkFormat(desc->format);
        createInfo.extent.width = desc->size.width;
        createInfo.extent.height = desc->size.height;
        createInfo.extent.depth = desc->size.depth;
        createInfo.mipLevels = desc->mipLevelCount;
        createInfo.arrayLayers = desc->size.depth;
        createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        createInfo.usage = vgpu_vkGetImageUsage(desc->usage, desc->format);
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
        createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        VkResult result = vmaCreateImage(vk.allocator,
            &createInfo, &allocCreateInfo,
            &texture.handle, &texture.allocation,
            nullptr);

        if (result != VK_SUCCESS)
        {
            vk.textures.dealloc(id);
            return vgpu::kInvalidTexture;
        }
    }

    /* Create default image view. */
    VkImageViewCreateInfo viewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewCreateInfo.image = texture.handle;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = texture.format;
    viewCreateInfo.components = VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    viewCreateInfo.subresourceRange.aspectMask = GetVkAspectMask(texture.format);
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    VK_CHECK(vk.api.vkCreateImageView(vk.device, &viewCreateInfo, NULL, &texture.view));

    texture.layout = VGPUTextureLayout_Undefined;
    memcpy(&(texture.desc), desc, sizeof(*desc));
    return { (uint32_t)id };
}

static void vk_destroy_texture(VGPUTexture handle)
{
    VGPUTextureVk& texture = vk.textures[handle.id];

    if (texture.view != VK_NULL_HANDLE)
    {
        vgpuVkDeferredDestroy(texture.view, nullptr, VK_OBJECT_TYPE_IMAGE_VIEW);
        //vk.api.vkDestroyImageView(vk.device, texture.view, NULL);
    }

    if (texture.allocation != VK_NULL_HANDLE) {
        //vmaDestroyImage(vk.allocator, texture.handle, texture.allocation);
        vgpuVkDeferredDestroy(texture.handle, texture.allocation, VK_OBJECT_TYPE_IMAGE);
    }

    vk.textures.dealloc(handle.id);
}

/* Sampler */
static inline VkFilter getVkFilter(VGPUFilterMode filter)
{
    switch (filter)
    {
    case VGPUFilterMode_Nearest:
        return VK_FILTER_NEAREST;
    case VGPUFilterMode_Linear:
        return VK_FILTER_LINEAR;
    default:
        _VGPU_UNREACHABLE();
        return VK_FILTER_NEAREST;
    }
}

static inline VkSamplerMipmapMode getVkMipMapFilterMode(VGPUFilterMode filter)
{
    switch (filter)
    {
    case VGPUFilterMode_Nearest:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    case VGPUFilterMode_Linear:
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    default:
        _VGPU_UNREACHABLE();
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    }
}

VkSamplerAddressMode getVkAddressMode(VGPUAddressMode mode)
{
    switch (mode)
    {
    case VGPUAddressMode_Repeat:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case VGPUAddressMode_MirrorRepeat:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case VGPUAddressMode_ClampToEdge:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case VGPUAddressMode_ClampToBorderColor:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        //case VGPUAddressMode_MirrorClampToEdge:
         //   return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    default:
        _VGPU_UNREACHABLE();
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

static inline VkBorderColor getVkBorderColor(VGPUBorderColor value)
{
    switch (value)
    {
    case VGPUBorderColor_TransparentBlack:
        return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    case VGPUBorderColor_OpaqueBlack:
        return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    case VGPUBorderColor_OpaqueWhite:
        return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    default:
        _VGPU_UNREACHABLE();
        return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    }
}

static VGPUSampler vk_samplerCreate(const VGPUSamplerDescriptor* desc)
{
    const VkBool32 compareEnable =
        (desc->compare != VGPUCompareFunction_Undefined) &&
        (desc->compare != VGPUCompareFunction_Never);

    VkSamplerCreateInfo createInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    createInfo.magFilter = getVkFilter(desc->magFilter);
    createInfo.minFilter = getVkFilter(desc->minFilter);
    createInfo.mipmapMode = getVkMipMapFilterMode(desc->mipmapFilter);
    createInfo.addressModeU = getVkAddressMode(desc->addressModeU);
    createInfo.addressModeV = getVkAddressMode(desc->addressModeV);
    createInfo.addressModeW = getVkAddressMode(desc->addressModeW);
    createInfo.mipLodBias = 0.0f;
    createInfo.anisotropyEnable = desc->maxAnisotropy > 0;
    createInfo.maxAnisotropy = (float)desc->maxAnisotropy;
    createInfo.compareEnable = compareEnable;
    createInfo.compareOp = getVkCompareOp(desc->compare, VK_COMPARE_OP_NEVER);
    createInfo.minLod = desc->lodMinClamp;
    createInfo.minLod = desc->lodMaxClamp == 0.0f ? 3.402823466e+38F : desc->lodMaxClamp;
    createInfo.borderColor = getVkBorderColor(desc->border_color);
    createInfo.unnormalizedCoordinates = VK_FALSE;

    VkSampler handle;
    if (vk.api.vkCreateSampler(vk.device, &createInfo, NULL, &handle) != VK_SUCCESS)
    {
        return vgpu::kInvalidSampler;
    }

    _vgpu_vk_set_name(VK_OBJECT_TYPE_SAMPLER, (uint64_t)handle, desc->label);

    if (vk.samplers.isFull()) {
        return vgpu::kInvalidSampler;
    }

    const int id = vk.samplers.alloc();
    VGPUSamplerVk& sampler = vk.samplers[id];
    sampler.handle = handle;
    return { (uint32_t)id };
}

static void vk_samplerDestroy(VGPUSampler handle)
{
    VGPUSamplerVk& sampler = vk.samplers[handle.id];
    vgpuVkDeferredDestroy(sampler.handle, nullptr, VK_OBJECT_TYPE_SAMPLER);
    vk.samplers.dealloc(handle.id);
}

/* RenderPass */
static uint32_t vk_GetRenderPassHash(
    const VGPURenderPassDescriptor* descriptor,
    vgpu::Hash* renderPassHash,
    vgpu::Hash* framebufferHash)
{
    vgpu::Hasher passHasher;
    vgpu::Hasher fboHasher;

    uint32_t colorAttachmentCount = 0;
    for (uint32_t i = 0; i < VGPU_MAX_COLOR_ATTACHMENTS; i++)
    {
        if (!vgpu::isValid(descriptor->colorAttachments[i].texture)) {
            continue;
        }

        VGPUTextureVk& texture = vk.textures[descriptor->colorAttachments[i].texture.id];

        passHasher.u32(texture.format);
        passHasher.u32(descriptor->colorAttachments[i].loadOp);
        fboHasher.u64(texture.cookie);
        colorAttachmentCount++;
    }

    if (vgpu::isValid(descriptor->depthStencilAttachment.texture))
    {
        VGPUTextureVk& texture = vk.textures[descriptor->depthStencilAttachment.texture.id];
        //hash.depthStencilFormat = texture.format;
        passHasher.u32(texture.format);
        passHasher.u32(descriptor->depthStencilAttachment.depthLoadOp);
        passHasher.u32(descriptor->depthStencilAttachment.stencilLoadOp);
        fboHasher.u64(texture.cookie);
    }

    passHasher.u32(colorAttachmentCount);
    fboHasher.u64(passHasher.get());

    *renderPassHash = passHasher.get();
    *framebufferHash = fboHasher.get();
    return colorAttachmentCount;
}

static VkAttachmentLoadOp get_vk_load_op(VGPULoadOp op) {
    switch (op) {
    case VGPULoadOp_DontCare:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    case VGPULoadOp_Load:
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    case VGPULoadOp_Clear:
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
    default:
        _VGPU_UNREACHABLE();
    }
}

static VkRenderPass vk_GetRenderPass(const VGPURenderPassDescriptor* descriptor, uint32_t colorAttachmentCount, vgpu::Hash hash)
{
    /* Lookup hash first */
    if (stbds_hmgeti(vk.renderPassHashMap, hash) != -1)
    {
        return stbds_hmget(vk.renderPassHashMap, hash);
    }

    uint32_t attachmentCount = 0;
    VkAttachmentDescription attachments[VGPU_MAX_COLOR_ATTACHMENTS + 1];
    VkAttachmentReference references[VGPU_MAX_COLOR_ATTACHMENTS + 1];

    for (uint32_t i = 0; i < colorAttachmentCount; i++)
    {
        VGPUTextureVk& texture = vk.textures[descriptor->colorAttachments[i].texture.id];
        attachments[i] = {};
        attachments[i].format = texture.format;
        attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[i].loadOp = get_vk_load_op(descriptor->colorAttachments[i].loadOp);
        attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[i].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments[i].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        references[i] = { i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
        attachmentCount++;
    }

    bool hasDepthStencil = false;
    if (vgpu::isValid(descriptor->depthStencilAttachment.texture))
    {
        hasDepthStencil = true;
        VGPUTextureVk& texture = vk.textures[descriptor->depthStencilAttachment.texture.id];
        uint32_t i = attachmentCount++;
        attachments[i] = {};
        attachments[i].format = texture.format;
        attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[i].loadOp = get_vk_load_op(descriptor->depthStencilAttachment.depthLoadOp);
        attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[i].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments[i].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        references[i] = { i, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
    }

    VkSubpassDescription subpass = {};
    subpass.flags = 0u;
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.inputAttachmentCount = 0u;
    subpass.pInputAttachments = nullptr;
    subpass.colorAttachmentCount = colorAttachmentCount;
    subpass.pColorAttachments = references;
    subpass.pResolveAttachments = nullptr;
    subpass.pDepthStencilAttachment = (hasDepthStencil) ? &references[attachmentCount - 1] : nullptr;
    subpass.preserveAttachmentCount = 0u;
    subpass.pPreserveAttachments = nullptr;

    VkRenderPassCreateInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    renderPassInfo.pNext = nullptr;
    renderPassInfo.flags = 0u;
    renderPassInfo.attachmentCount = attachmentCount;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 0u;
    renderPassInfo.pDependencies = nullptr;

    VkRenderPass renderPass;
    if (vk.api.vkCreateRenderPass(vk.device, &renderPassInfo, NULL, &renderPass) != VK_SUCCESS)
    {
        return nullptr;
    }

    hmput(vk.renderPassHashMap, hash, renderPass);
    return renderPass;
}

static VGPUFramebufferVk vk_GetFramebuffer(const VGPURenderPassDescriptor* descriptor, uint32_t colorAttachmentCount, VkRenderPass renderPass, vgpu::Hash hash)
{
    /* Lookup hash first */
    if (stbds_hmgeti(vk.framebufferHashMap, hash) != -1)
    {
        return stbds_hmget(vk.framebufferHashMap, hash);
    }

    uint32_t width = UINT32_MAX;
    uint32_t height = UINT32_MAX;

    VkImageView attachments[VGPU_MAX_COLOR_ATTACHMENTS + 1];
    VGPUFramebufferVk result = {};

    for (uint32_t i = 0; i < colorAttachmentCount; i++)
    {
        VGPUTextureVk& texture = vk.textures[descriptor->colorAttachments[i].texture.id];

        uint32_t mipLevel = descriptor->colorAttachments[i].mipLevel;
        width = _vgpu_min(width, _vgpu_max(1u, texture.desc.size.width >> mipLevel));
        height = _vgpu_min(height, _vgpu_max(1u, texture.desc.size.height >> mipLevel));

        attachments[result.attachmentCount] = texture.view;
        result.attachments[result.attachmentCount] = descriptor->colorAttachments[i].texture;
        result.attachmentCount++;
    }

    if (vgpu::isValid(descriptor->depthStencilAttachment.texture))
    {
        VGPUTextureVk& texture = vk.textures[descriptor->depthStencilAttachment.texture.id];

        uint32_t mipLevel = 0; // descriptor->depthStencilAttachment.mipLevel;
        uint32_t slice = 0;
        width = _vgpu_min(width, _vgpu_max(1u, texture.desc.size.width >> mipLevel));
        height = _vgpu_min(height, _vgpu_max(1u, texture.desc.size.height >> mipLevel));

        attachments[result.attachmentCount] = texture.view;
        result.attachments[result.attachmentCount] = descriptor->depthStencilAttachment.texture;
        result.attachmentCount++;
    }

    VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    createInfo.renderPass = renderPass;
    createInfo.attachmentCount = result.attachmentCount;
    createInfo.pAttachments = attachments;
    createInfo.width = width;
    createInfo.height = height;
    createInfo.layers = 1;

    if (vk.api.vkCreateFramebuffer(vk.device, &createInfo, NULL, &result.handle) != VK_SUCCESS)
    {
        return result;
    }

    result.width = width;
    result.height = height;
    stbds_hmput(vk.framebufferHashMap, hash, result);
    return result;
}

/* Commands */
static void vk_cmdBeginRenderPass(const VGPURenderPassDescriptor* descriptor)
{
    vgpu::Hash renderPassHash, fboHash;
    uint32_t colorAttachmentCount = vk_GetRenderPassHash(descriptor, &renderPassHash, &fboHash);

    VkRenderPass renderPass = vk_GetRenderPass(descriptor, colorAttachmentCount, renderPassHash);
    VGPUFramebufferVk framebuffer = vk_GetFramebuffer(descriptor, colorAttachmentCount, renderPass, fboHash);

    uint32_t clearValueCount = 0;
    VkClearValue clearValues[VGPU_MAX_COLOR_ATTACHMENTS + 1];

    for (uint32_t i = 0; i < framebuffer.attachmentCount - 1; i++) {
        vgpuVkTextureBarrier(vk.frame->commandBuffer, framebuffer.attachments[i], VGPUTextureLayout_RenderTarget);

        clearValues[clearValueCount].color.float32[0] = descriptor->colorAttachments[i].clearColor.r;
        clearValues[clearValueCount].color.float32[1] = descriptor->colorAttachments[i].clearColor.g;
        clearValues[clearValueCount].color.float32[2] = descriptor->colorAttachments[i].clearColor.b;
        clearValues[clearValueCount].color.float32[3] = descriptor->colorAttachments[i].clearColor.a;
        clearValueCount++;
    }

    if (vgpu::isValid(descriptor->depthStencilAttachment.texture))
    {
        vgpuVkTextureBarrier(vk.frame->commandBuffer, descriptor->depthStencilAttachment.texture, VGPUTextureLayout_RenderTarget);

        clearValues[clearValueCount].depthStencil.depth = descriptor->depthStencilAttachment.clearDepth;
        clearValues[clearValueCount].depthStencil.stencil = descriptor->depthStencilAttachment.clearStencil;
        clearValueCount++;
    }

    VkRenderPassBeginInfo beginInfo;
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.pNext = nullptr;
    beginInfo.renderPass = renderPass;
    beginInfo.framebuffer = framebuffer.handle;
    beginInfo.renderArea.offset.x = 0;
    beginInfo.renderArea.offset.y = 0;
    beginInfo.renderArea.extent.width = framebuffer.width;
    beginInfo.renderArea.extent.height = framebuffer.height;
    beginInfo.clearValueCount = clearValueCount;
    beginInfo.pClearValues = clearValues;
    vk.api.vkCmdBeginRenderPass(vk.frame->commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

static void vk_cmdEndRenderPass(void)
{
    vk.api.vkCmdEndRenderPass(vk.frame->commandBuffer);
}

bool vgpu_vk_supported(void) {
    if (vk.available_initialized) {
        return vk.available;
    }

    vk.available_initialized = true;

    VkResult result = vkbInit(&vk.api);
    if (result != VK_SUCCESS) {
        vgpu_log(VGPU_LOG_LEVEL_DEBUG, "Failed to initialize vkbind.");
        return false;
    }

    vk.available = true;
    return true;
}

vgpu_renderer* vgpu_vk_create_device(void) {
    static vgpu_renderer renderer = { nullptr };
    ASSIGN_DRIVER(vk);
    return &renderer;
}

#else

#include "vgpu/vgpu.h"

bool vgpu_vk_supported(void) {
    return false;
}

VGPUDevice vgpu_vk_create_device(void) {
    return nullptr;
}

#endif /* defined(VGPU_BACKEND_VULKAN) */
