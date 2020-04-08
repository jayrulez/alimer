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

#include "vgpu_backend.h"

#include "vk/vk.h"
#include "vk/vk_mem_alloc.h"

#define VK_CHECK(res) do { VkResult r = (res); GPU_CHECK(r >= 0, vk_get_error_string(r)); } while (0)

#if !defined(NDEBUG) || defined(DEBUG) || defined(_DEBUG)
#   define VULKAN_DEBUG
#endif

#define _GPU_MAX_PHYSICAL_DEVICES (32u)
#define _VGPU_VK_MAX_SURFACE_FORMATS (32u)
#define _VGPU_VK_MAX_PRESENT_MODES (16u)

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

typedef struct {
    uint32_t graphics_queue_family;
    uint32_t compute_queue_family;
    uint32_t copy_queue_family;
} vk_queue_family_indices;

typedef struct vgpu_vk_surface_caps {
    bool success;
    VkSurfaceCapabilitiesKHR capabilities;
    uint32_t format_count;
    uint32_t present_mode_count;
    VkSurfaceFormatKHR formats[_VGPU_VK_MAX_SURFACE_FORMATS];
    VkPresentModeKHR present_modes[_VGPU_VK_MAX_PRESENT_MODES];
} vgpu_vk_surface_caps;

typedef struct vgpu_vk_frame {
    uint32_t index;
    bool active;

    VkCommandPool command_pool;
    VkFence fence;
    VkCommandBuffer command_buffer;
    VkSemaphore cmd_buffer_semaphore;

    uint32_t submitted_command_buffer_count;
    VkCommandBuffer submitted_command_buffers[VGPU_MAX_SUBMITTED_COMMAND_BUFFERS];
    VkSemaphore signal_semaphores[VGPU_MAX_SUBMITTED_COMMAND_BUFFERS];
} vgpu_vk_frame;

typedef struct vgpu_vk_context {
    VkSurfaceKHR surface;
    uint32_t width;
    uint32_t height;
    uint32_t preferred_image_count;
    bool srgb;
    VkPresentModeKHR present_mode;
    VkSwapchainKHR handle;
    uint32_t image_index;
    uint32_t image_count;
    VkImage* images;
    VkSemaphore* image_acquired_semaphore;

    uint32_t max_inflight_frames;
    vgpu_vk_frame* frames; /* frames[max_inflight_frames] */
    vgpu_vk_frame* frame; /* active frame */
} vgpu_vk_context;

typedef struct vgpu_vk_buffer {
    VkBuffer handle;
    VmaAllocation allocation;
} vgpu_vk_buffer;

typedef struct vgpu_vk_texture {
    VkImage handle;
    VmaAllocation allocation;
} vgpu_vk_texture;

typedef struct agpu_vk_renderer {
    /* Associated device */
    agpu_device* gpu_device;

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

    /* Main context. */
    vgpu_vk_context* main_context;

    /* Current active context. */
    vgpu_vk_context* context;

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
    bool full_screen_exclusive;

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
static vgpu_vk_surface_caps _vgpu_query_swapchain_support(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
static VkSurfaceKHR vk_create_surface(uintptr_t native_handle, uint32_t* width, uint32_t* height);
static void _vgpu_vk_fill_buffer_sharing_indices(vk_queue_family_indices indices, VkBufferCreateInfo* info, uint32_t* sharing_indices);

static VkCommandBuffer _vgpu_request_command_buffer(agpu_vk_renderer* driver_data, VkCommandPool command_pool);
static void _vgpu_commit_command_buffer(agpu_vk_renderer* driver_data, VkCommandBuffer command_buffer, VkCommandPool command_pool);

/* Conversion functions */
static VkFormat get_vk_image_format(vgpu_pixel_format value) {
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
        VK_FORMAT_R16G16_UNORM,
        VK_FORMAT_R16G16_SNORM,
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
        VK_FORMAT_R16G16B16A16_UNORM,
        VK_FORMAT_R16G16B16A16_SNORM,
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
        VK_FORMAT_D24_UNORM_S8_UINT,
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

        // Compressed PVRTC Pixel Formats
        VK_FORMAT_UNDEFINED,
        VK_FORMAT_UNDEFINED,
        VK_FORMAT_UNDEFINED,
        VK_FORMAT_UNDEFINED,

        // Compressed ETC Pixel Formats
        VK_FORMAT_UNDEFINED,
        VK_FORMAT_UNDEFINED,
        VK_FORMAT_UNDEFINED,
        VK_FORMAT_UNDEFINED,

        // Compressed ASTC Pixel Formats
        VK_FORMAT_UNDEFINED,
        VK_FORMAT_UNDEFINED,
        VK_FORMAT_UNDEFINED,
        VK_FORMAT_UNDEFINED,
        VK_FORMAT_UNDEFINED,
        VK_FORMAT_UNDEFINED,
        VK_FORMAT_UNDEFINED,
        VK_FORMAT_UNDEFINED
    };
    return formats[value];
}

static VkPresentModeKHR get_vk_present_mode(vgpu_present_mode value) {
    static const VkPresentModeKHR types[_VGPU_PRESENT_MODE_COUNT] = {
      VK_PRESENT_MODE_IMMEDIATE_KHR,
      VK_PRESENT_MODE_MAILBOX_KHR,
      VK_PRESENT_MODE_FIFO_KHR
    };
    return types[value];
}

static VkImageType get_vk_image_type(vgpu_texture_type value) {
    static const VkImageType types[VGPU_TEXTURE_TYPE_COUNT] = {
      VK_IMAGE_TYPE_2D,
      VK_IMAGE_TYPE_3D,
      VK_IMAGE_TYPE_2D
    };
    return types[value];
}

static VkSampleCountFlagBits get_vk_sample_count(uint32_t sample_count) {
    switch (sample_count) {
    case 0u:
    case 1u:  return VK_SAMPLE_COUNT_1_BIT;
    case 2u:  return VK_SAMPLE_COUNT_2_BIT;
    case 4u:  return VK_SAMPLE_COUNT_4_BIT;
    case 8u:  return VK_SAMPLE_COUNT_8_BIT;
    case 16u: return VK_SAMPLE_COUNT_16_BIT;
    case 32u: return VK_SAMPLE_COUNT_32_BIT;
    case 64u: return VK_SAMPLE_COUNT_64_BIT;
    default:
        /* TODO: warn user */
        return  VK_SAMPLE_COUNT_1_BIT;
    }
}

#if defined(VULKAN_DEBUG)
VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
{
    // Log debug messge
    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        vgpu_log_format(VGPU_LOG_LEVEL_WARN, "%u - %s: %s", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
    }
    else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        vgpu_log_format(VGPU_LOG_LEVEL_ERROR, "%u - %s: %s", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
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
        vgpu_log_format(VGPU_LOG_LEVEL_ERROR, "%s: %s", layer_prefix, message);
    }
    else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
    {
        vgpu_log_format(VGPU_LOG_LEVEL_WARN, "%s: %s", layer_prefix, message);
    }
    else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
    {
        vgpu_log_format(VGPU_LOG_LEVEL_WARN, "%s: %s", layer_prefix, message);
    }
    else
    {
        vgpu_log_format(VGPU_LOG_LEVEL_INFO, "%s: %s", layer_prefix, message);
    }
    return VK_FALSE;
}
#endif

static void vk_destroy_device(agpu_device* device) {
    agpu_vk_renderer* renderer = (agpu_vk_renderer*)device->renderer;

    if (renderer->device != VK_NULL_HANDLE) {
        vgpu_wait_idle(device);
    }

    /* TODO: Release data. */
    if (renderer->main_context) {
        vgpu_destroy_context(device, (vgpu_context*)renderer->main_context);
    }

    if (renderer->memory_allocator != VK_NULL_HANDLE)
    {
        VmaStats stats;
        vmaCalculateStats(renderer->memory_allocator, &stats);

        if (stats.total.usedBytes > 0) {
            vgpu_log_format(VGPU_LOG_LEVEL_ERROR, "Total device memory leaked: %llx bytes.", stats.total.usedBytes);
        }

        vmaDestroyAllocator(renderer->memory_allocator);
    }

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

    VGPU_FREE(renderer);
    VGPU_FREE(device);
}

static void vk_wait_idle(agpu_renderer* renderer) {
    agpu_vk_renderer* vk_renderer = (agpu_vk_renderer*)renderer;
    vkDeviceWaitIdle(vk_renderer->device);
}


static void vk_begin_frame(agpu_renderer* driver_data) {
    agpu_vk_renderer* renderer = (agpu_vk_renderer*)driver_data;
    vgpu_vk_context* context = renderer->context;
    VK_CHECK(vkWaitForFences(renderer->device, 1, &context->frame->fence, VK_FALSE, UINT64_MAX));
    VK_CHECK(vkResetFences(renderer->device, 1, &context->frame->fence));

    /* Release destroy data */

    /* TODO: We need transient flat probably */
    VK_CHECK(vkResetCommandPool(renderer->device, context->frame->command_pool, 0));
    if (context->frame->cmd_buffer_semaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(renderer->device, context->frame->cmd_buffer_semaphore, NULL);
    }

    context->frame->command_buffer = _vgpu_request_command_buffer(renderer, context->frame->command_pool);
    context->frame->submitted_command_buffer_count = 0;

    // Create semaphore.
    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0u
    };
    VK_CHECK(vkCreateSemaphore(renderer->device, &semaphore_info, NULL, &context->frame->cmd_buffer_semaphore));
}

static void vk_end_frame(agpu_renderer* driver_data) {
    agpu_vk_renderer* renderer = (agpu_vk_renderer*)driver_data;
    vgpu_vk_context* context = renderer->context;

    uint32_t frame_index = context->frame->index;

    /* TODO: Submit compute and copy commands here. */
    VkResult result = VK_SUCCESS;
    const bool has_swapchain = context->handle != VK_NULL_HANDLE;
    if (has_swapchain)
    {
        result = vkAcquireNextImageKHR(renderer->device,
            context->handle,
            UINT64_MAX,
            context->image_acquired_semaphore[frame_index],
            VK_NULL_HANDLE,
            &context->image_index);

        if (result == VK_ERROR_OUT_OF_DATE_KHR ||
            result == VK_SUBOPTIMAL_KHR)
        {
            // windowResize();
        }
        else {
            VK_CHECK(result);
        }
    }

    VK_CHECK(vkEndCommandBuffer(context->frame->command_buffer));
    context->frame->submitted_command_buffers[context->frame->submitted_command_buffer_count] = context->frame->command_buffer;
    context->frame->signal_semaphores[context->frame->submitted_command_buffer_count] = context->frame->cmd_buffer_semaphore;
    context->frame->submitted_command_buffer_count++;


    // Submit pending graphics commands.
    const VkPipelineStageFlags color_attachment_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    uint32_t wait_semaphore_count = 0u;
    VkSemaphore* wait_semaphores = NULL;
    const VkPipelineStageFlags* wait_stage_masks = NULL;
    if (has_swapchain)
    {
        wait_semaphore_count = 1u;
        wait_semaphores = &context->image_acquired_semaphore[frame_index];
        wait_stage_masks = &color_attachment_stage;
    }

    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,
        .waitSemaphoreCount = wait_semaphore_count,
        .pWaitSemaphores = wait_semaphores,
        .pWaitDstStageMask = wait_stage_masks,
        .commandBufferCount = context->frame->submitted_command_buffer_count,
        .pCommandBuffers = context->frame->submitted_command_buffers,
        .signalSemaphoreCount = context->frame->submitted_command_buffer_count,
        .pSignalSemaphores = context->frame->signal_semaphores
    };
    VK_CHECK(vkQueueSubmit(renderer->graphics_queue, 1, &submit_info, context->frame->fence));

    // Present if necessary.
    if (has_swapchain) {
        const VkPresentInfoKHR present_info = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = NULL,
            .waitSemaphoreCount = context->frame->submitted_command_buffer_count,
            .pWaitSemaphores = context->frame->signal_semaphores,
            .swapchainCount = 1,
            .pSwapchains = &context->handle,
            .pImageIndices = &context->image_index,
            .pResults = NULL
        };
        const VkResult present_result = vkQueuePresentKHR(renderer->graphics_queue, &present_info);
        VK_CHECK(present_result);
    }

    /* Advance to next frame.*/
    context->frame = &context->frames[(context->frame->index + 1) % context->max_inflight_frames];
}

static void vk_set_context(agpu_renderer* renderer, vgpu_context* context) {
    agpu_vk_renderer* vk_renderer = (agpu_vk_renderer*)renderer;
    vk_renderer->context = (vgpu_vk_context*)context;
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

static bool vk_init_or_update_context(agpu_vk_renderer* renderer, vgpu_context* context) {
    vgpu_vk_context* vk_context = (vgpu_vk_context*)context;

    vgpu_vk_surface_caps surface_caps = _vgpu_query_swapchain_support(renderer->physical_device, vk_context->surface);

    VkSwapchainKHR old_swapchain = vk_context->handle;

    /* Detect image count. */
    uint32_t image_count = vk_context->preferred_image_count;
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

    vk_context->max_inflight_frames = _vgpu_max(vk_context->max_inflight_frames, image_count);

    /* Extent */
    VkExtent2D swapchain_size = { vk_context->width, vk_context->height };
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
    if (surface_caps.format_count == 1 &&
        surface_caps.formats[0].format == VK_FORMAT_UNDEFINED)
    {
        format = surface_caps.formats[0];
        format.format = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else
    {
        if (surface_caps.format_count == 0)
        {
            //LOGE("Surface has no formats.\n");
            return false;
        }

        bool found = false;
        for (uint32_t i = 0; i < surface_caps.format_count; i++)
        {
            if (vk_context->srgb)
            {
                if (surface_caps.formats[i].format == VK_FORMAT_R8G8B8A8_SRGB ||
                    surface_caps.formats[i].format == VK_FORMAT_B8G8R8A8_SRGB ||
                    surface_caps.formats[i].format == VK_FORMAT_A8B8G8R8_SRGB_PACK32)
                {
                    format = surface_caps.formats[i];
                    found = true;
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
                }
            }
        }

        if (!found)
            format = surface_caps.formats[0];
    }

    // Enable transfer destination on swap chain images if supported
    VkImageUsageFlags image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Enable transfer source on swap chain images if supported
    if (surface_caps.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
        image_usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    // Enable transfer destination on swap chain images if supported
    if (surface_caps.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
        image_usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
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
    VkPresentModeKHR present_mode = vk_context->present_mode;
    for (uint32_t i = 0; i < surface_caps.present_mode_count; i++) {
        if (surface_caps.present_modes[i] == present_mode) {
            present_mode_found = true;
            break;
        }
    }

    if (!present_mode_found) {
        present_mode = VK_PRESENT_MODE_FIFO_KHR;
    }

    /* We use same family for graphics and present so no sharing is necessary. */
    const VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .surface = vk_context->surface,
        .minImageCount = image_count,
        .imageFormat = format.format,
        .imageColorSpace = format.colorSpace,
        .imageExtent = swapchain_size,
        .imageArrayLayers = 1,
        .imageUsage = image_usage,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .preTransform = pre_transform,
        .compositeAlpha = composite_mode,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = old_swapchain
    };

    VkResult result = vkCreateSwapchainKHR(renderer->device, &create_info, NULL, &vk_context->handle);
    if (result != VK_SUCCESS) {
        vgpu_destroy_context(renderer->gpu_device, context);
        return false;
    }

    if (old_swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(renderer->device, old_swapchain, NULL);
    }

    // Obtain swapchain images.
    result = vkGetSwapchainImagesKHR(renderer->device, vk_context->handle, &vk_context->image_count, NULL);
    if (result != VK_SUCCESS) {
        vgpu_destroy_context(renderer->gpu_device, context);
        return false;
    }

    vk_context->images = VGPU_ALLOCN(VkImage, vk_context->image_count);
    if (vk_context->images == NULL) {
        vgpu_destroy_context(renderer->gpu_device, context);
        return false;
    }

    result = vkGetSwapchainImagesKHR(renderer->device, vk_context->handle, &vk_context->image_count, vk_context->images);
    if (result != VK_SUCCESS) {
        vgpu_destroy_context(renderer->gpu_device, context);
        return false;
    }

    vk_context->image_acquired_semaphore = VGPU_ALLOCN(VkSemaphore, vk_context->image_count);
    if (vk_context->image_acquired_semaphore == NULL) {
        vgpu_destroy_context(renderer->gpu_device, context);
        return false;
    }

    /* Allocate and init frame data. */
    {
        vk_context->frames = VGPU_ALLOCN(vgpu_vk_frame, vk_context->max_inflight_frames);
        if (vk_context->frames == NULL) {
            vgpu_destroy_context(renderer->gpu_device, context);
            return false;
        }

        const VkCommandPoolCreateInfo graphics_command_pool_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = NULL,
            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = renderer->queue_families.graphics_queue_family
        };

        const VkFenceCreateInfo fence_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = NULL,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        for (uint32_t i = 0; i < vk_context->max_inflight_frames; ++i) {
            vk_context->frames[i].index = i;
            vk_context->frames[i].active = false;
            vk_context->frames[i].submitted_command_buffer_count = 0;
            vk_context->frames[i].command_buffer = VK_NULL_HANDLE;
            vk_context->frames[i].cmd_buffer_semaphore = VK_NULL_HANDLE;

            if (vkCreateCommandPool(renderer->device, &graphics_command_pool_info, NULL, &vk_context->frames[i].command_pool)) {
                vgpu_destroy_context(renderer->gpu_device, context);
                return false;
            }

            if (vkCreateFence(renderer->device, &fence_info, NULL, &vk_context->frames[i].fence)) {
                vgpu_destroy_context(renderer->gpu_device, context);
                return false;
            }
        }

        vk_context->frame = &vk_context->frames[0];
        vk_context->frame->active = true;
    }

    /* Setup image data. */
    {
        const VkSemaphoreCreateInfo semaphore_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0
        };

        VkCommandBuffer setup_cmd_buffer = _vgpu_request_command_buffer(renderer, vk_context->frame->command_pool);
        for (uint32_t i = 0; i < vk_context->image_count; ++i)
        {
            result = vkCreateSemaphore(renderer->device, &semaphore_info, NULL, &vk_context->image_acquired_semaphore[i]);
            if (result != VK_SUCCESS) {
                vgpu_destroy_context(renderer->gpu_device, context);
                return false;
            }

            VkImageMemoryBarrier barrier;
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.pNext = nullptr;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = 0;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier.srcQueueFamilyIndex = 0;
            barrier.dstQueueFamilyIndex = 0;
            barrier.image = vk_context->images[i];
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            vkCmdPipelineBarrier(setup_cmd_buffer,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);
        }

        _vgpu_commit_command_buffer(renderer, setup_cmd_buffer, vk_context->frame->command_pool);
    }

    vk_context->image_index = 0u;
    vk_context->width = swapchain_size.width;
    vk_context->height = swapchain_size.height;

    return true;
}

static vgpu_context* vk_create_context(agpu_renderer* driver_data, const vgpu_context_desc* desc) {
    agpu_vk_renderer* renderer = (agpu_vk_renderer*)driver_data;
    vgpu_vk_context* context = VGPU_ALLOC_HANDLE(vgpu_vk_context);

    context->surface = vk_create_surface(desc->native_handle, &context->width, &context->height);
    context->handle = VK_NULL_HANDLE;
    context->max_inflight_frames = desc->max_inflight_frames;
    context->preferred_image_count = desc->image_count;
    context->srgb = desc->srgb;
    context->present_mode = get_vk_present_mode(desc->present_mode);

    if (!vk_init_or_update_context(renderer, (vgpu_context*)context))
    {
        VGPU_FREE(context);
        return NULL;
    }

    return (vgpu_context*)context;
}

static void vk_destroy_context(agpu_renderer* driver_data, vgpu_context* context) {
    agpu_vk_renderer* renderer = (agpu_vk_renderer*)driver_data;
    vgpu_vk_context* vk_context = (vgpu_vk_context*)context;

    /* Destroy swap chain data. */
    for (uint32_t i = 0; i < vk_context->image_count; i++) {
        vkDestroySemaphore(renderer->device, vk_context->image_acquired_semaphore[i], NULL);
    }

    /* Destroy frame data. */
    for (uint32_t i = 0; i < vk_context->max_inflight_frames; i++) {
        vgpu_vk_frame* frame = &vk_context->frames[i];

        vkDestroyCommandPool(renderer->device, frame->command_pool, NULL);
        vkDestroySemaphore(renderer->device, frame->cmd_buffer_semaphore, NULL);
        vkDestroyFence(renderer->device, frame->fence, NULL);
    }

    if (vk_context->handle != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(renderer->device, vk_context->handle, NULL);
    }

    if (vk_context->surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(vk.instance, vk_context->surface, NULL);
    }

    VGPU_FREE(vk_context->images);
    VGPU_FREE(vk_context->image_acquired_semaphore);
    VGPU_FREE(vk_context->frames);
    VGPU_FREE(vk_context);
}

/* Buffer */
static vgpu_buffer* vk_create_buffer(agpu_renderer* renderer, const vgpu_buffer_desc* desc) {
    agpu_vk_renderer* vk_renderer = (agpu_vk_renderer*)renderer;

    VkBufferUsageFlags usage = 0;
    if (desc->usage & VPU_BUFFER_USAGE_COPY_SRC) {
        usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }
    if (desc->usage & VPU_BUFFER_USAGE_COPY_DEST) {
        usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    if (desc->usage & VPU_BUFFER_USAGE_INDEX) {
        usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }
    if (desc->usage & VPU_BUFFER_USAGE_VERTEX) {
        usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if (desc->usage & VPU_BUFFER_USAGE_UNIFORM) {
        usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }
    if (desc->usage & VPU_BUFFER_USAGE_STORAGE) {
        usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }
    if (desc->usage & VPU_BUFFER_USAGE_INDIRECT) {
        usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    }

    VkBufferCreateInfo buffer_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = NULL,
      .flags = 0u,
      .size = desc->size,
      .usage = usage
    };

    uint32_t sharing_indices[3];
    _vgpu_vk_fill_buffer_sharing_indices(vk_renderer->queue_families, &buffer_info, sharing_indices);

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

    vgpu_vk_buffer* buffer = VGPU_ALLOC_HANDLE(vgpu_vk_buffer);
    buffer->handle = handle;
    buffer->allocation = allocation;
    return (vgpu_buffer*)result;
}

static void vk_destroy_buffer(agpu_renderer* driver_data, vgpu_buffer* driver_buffer) {
    agpu_vk_renderer* renderer = (agpu_vk_renderer*)driver_data;
    vgpu_vk_buffer* buffer = (vgpu_vk_buffer*)driver_buffer;
}

/* Texture */
static vgpu_texture* vk_create_texture(agpu_renderer* driver_data, const vgpu_texture_desc* desc) {
    agpu_vk_renderer* renderer = (agpu_vk_renderer*)driver_data;

    const VkSharingMode sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    VkImageUsageFlags image_usage = 0;
    if (desc->usage & VGPU_TEXTURE_USAGE_COPY_SRC) {
        image_usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (desc->usage & VGPU_TEXTURE_USAGE_COPY_SRC) {
        image_usage |= VGPU_TEXTURE_USAGE_COPY_DEST;
    }
    if (desc->usage & VGPU_TEXTURE_USAGE_SAMPLED) {
        image_usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (desc->usage & VGPU_TEXTURE_USAGE_STORAGE) {
        image_usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if (desc->usage & VGPU_TEXTURE_USAGE_OUTPUT_ATTACHMENT) {
        if (vgpu_is_depth_stencil_format(desc->format)) {
            image_usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        else {
            image_usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
    }

    const VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0u,
        .imageType = get_vk_image_type(desc->type),
        .extent = {
            .width = desc->extent.width,
            .height = desc->extent.height,
            .depth = desc->extent.depth
        },
        .format = get_vk_image_format(desc->format),
        .mipLevels = 1,
        .arrayLayers = 1u,
        .samples = get_vk_sample_count(desc->sample_count),
        .usage = image_usage,
        .sharingMode = sharing_mode,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    const VmaAllocationCreateInfo memory_info = {
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    VkImage handle;
    VmaAllocation allocation;
    VmaAllocationInfo allocation_info;
    VkResult result = vmaCreateImage(renderer->memory_allocator,
        &image_info,
        &memory_info,
        &handle, &allocation, &allocation_info);
    if (result != VK_SUCCESS) {
        GPU_THROW("[Vulkan]: Failed to create texture");
        return NULL;
    }

    vgpu_vk_texture* texture = VGPU_ALLOC_HANDLE(vgpu_vk_texture);
    texture->handle = handle;
    texture->allocation = allocation;
    return (vgpu_texture*)texture;
}

static void vk_destroy_texture(agpu_renderer* driver_data, vgpu_texture* driver_texture) {
    agpu_vk_renderer* _renderer = (agpu_vk_renderer*)driver_data;
    vgpu_vk_texture* texture = (vgpu_vk_texture*)driver_texture;
}

static vgpu_backend vk_query_backend() {
    return VGPU_BACKEND_VULKAN;
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

static void _vgpu_vk_fill_buffer_sharing_indices(
    vk_queue_family_indices indices, VkBufferCreateInfo* info, uint32_t* sharing_indices) {
    info->sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (indices.graphics_queue_family != indices.compute_queue_family ||
        indices.graphics_queue_family != indices.compute_queue_family)
    {
        info->sharingMode = VK_SHARING_MODE_CONCURRENT;

        sharing_indices[info->queueFamilyIndexCount++] = indices.graphics_queue_family;

        if (indices.graphics_queue_family != indices.compute_queue_family)
            sharing_indices[info->queueFamilyIndexCount++] = indices.compute_queue_family;

        if (indices.graphics_queue_family != indices.copy_queue_family &&
            indices.compute_queue_family != indices.copy_queue_family)
        {
            sharing_indices[info->queueFamilyIndexCount++] = indices.copy_queue_family;
        }

        info->pQueueFamilyIndices = sharing_indices;
    }
}

static VkCommandBuffer _vgpu_request_command_buffer(agpu_vk_renderer* renderer, VkCommandPool command_pool)
{
    vgpu_vk_context* context = renderer->context;

    VkCommandBufferAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1u
    };

    VkCommandBuffer command_buffer;
    VK_CHECK(vkAllocateCommandBuffers(renderer->device, &allocate_info, &command_buffer));

    VkCommandBufferBeginInfo beginfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    VK_CHECK(vkBeginCommandBuffer(command_buffer, &beginfo));
    return command_buffer;
}

static void _vgpu_commit_command_buffer(agpu_vk_renderer* renderer, VkCommandBuffer command_buffer, VkCommandPool command_pool)
{
    VK_CHECK(vkEndCommandBuffer(command_buffer));

    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1u,
        .pCommandBuffers = &command_buffer
    };

    // Create fence to ensure that the command buffer has finished executing
    const VkFenceCreateInfo fence_info =
    {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0
    };

    VkFence fence;
    VK_CHECK(vkCreateFence(renderer->device, &fence_info, nullptr, &fence));

    // Submit to the queue
    VK_CHECK(vkQueueSubmit(renderer->graphics_queue, 1, &submit_info, fence));
    // Wait for the fence to signal that command buffer has finished executing
    VK_CHECK(vkWaitForFences(renderer->device, 1, &fence, VK_TRUE, 100000000000));
    vkDestroyFence(renderer->device, fence, nullptr);

    vkFreeCommandBuffers(renderer->device, command_pool, 1, &command_buffer);
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
    VkQueueFamilyProperties* queue_families = _vgpu_alloca(VkQueueFamilyProperties, queue_count);
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

    VkExtensionProperties* available_extensions = _vgpu_alloca(VkExtensionProperties, ext_count);
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
            vk.full_screen_exclusive = true;
        }
    }

    return result;
}

static vgpu_vk_surface_caps _vgpu_query_swapchain_support(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    vgpu_vk_surface_caps caps;
    memset(&caps, 0, sizeof(vgpu_vk_surface_caps));
    caps.format_count = _VGPU_VK_MAX_SURFACE_FORMATS;
    caps.present_mode_count = _VGPU_VK_MAX_PRESENT_MODES;

    const VkPhysicalDeviceSurfaceInfo2KHR surface_info = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
        .pNext = NULL,
        .surface = surface
    };

    if (vk.surface_capabilities2)
    {
        VkSurfaceCapabilities2KHR surface_caps2 = {
            .sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR,
            .pNext = NULL
        };

        if (vkGetPhysicalDeviceSurfaceCapabilities2KHR(physical_device, &surface_info, &surface_caps2) != VK_SUCCESS)
        {
            return caps;
        }
        caps.capabilities = surface_caps2.surfaceCapabilities;

        if (vkGetPhysicalDeviceSurfaceFormats2KHR(physical_device, &surface_info, &caps.format_count, NULL) != VK_SUCCESS)
        {
            return caps;
        }

        VkSurfaceFormat2KHR* formats2 = _vgpu_alloca(VkSurfaceFormat2KHR, caps.format_count);
        VGPU_ASSERT(formats2);

        for (uint32_t i = 0; i < caps.format_count; ++i)
        {
            memset(&formats2[i], 0, sizeof(VkSurfaceFormat2KHR));
            formats2[i].sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
        }

        if (vkGetPhysicalDeviceSurfaceFormats2KHR(physical_device, &surface_info, &caps.format_count, formats2) != VK_SUCCESS)
        {
            return caps;
        }

        for (uint32_t i = 0; i < caps.format_count; ++i)
        {
            caps.formats[i] = formats2[i].surfaceFormat;
        }
    }
    else
    {
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &caps.capabilities) != VK_SUCCESS)
        {
            return caps;
        }

        if (vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &caps.format_count, caps.formats) != VK_SUCCESS)
        {
            return caps;
        }
    }

#ifdef _WIN32
    if (vk.surface_capabilities2 && vk.full_screen_exclusive)
    {
        if (vkGetPhysicalDeviceSurfacePresentModes2EXT(physical_device, &surface_info, &caps.present_mode_count, caps.present_modes) != VK_SUCCESS)
        {
            return caps;
        }
    }
    else
#endif
    {
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &caps.present_mode_count, caps.present_modes) != VK_SUCCESS)
        {
            return caps;
        }
    }

    caps.success = true;
    return caps;
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
    device = VGPU_ALLOC_HANDLE(agpu_device);
    ASSIGN_DRIVER(vk);

    /* Init the renderer */
    renderer = VGPU_ALLOC_HANDLE(agpu_vk_renderer);
    memset(renderer, 0, sizeof(agpu_vk_renderer));

    /* Reference gpu_device and renderer each other */
    renderer->gpu_device = device;
    device->renderer = (agpu_renderer*)renderer;

    bool headless = true;
    if (desc->main_context_desc) {
        headless = false;
    }

    /* Setup instance only once. */
    if (!vk.instance) {
        bool validation = false;
#if defined(VULKAN_DEBUG)
        if (desc->flags & VGPU_CONFIG_FLAGS_VALIDATION) {
            validation = true;
        }
#endif
        if (!agpu_vk_init_loader()) {
            return false;
        }

        uint32_t instance_extension_count;
        VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, NULL));

        VkExtensionProperties* available_instance_extensions = _vgpu_alloca(VkExtensionProperties, instance_extension_count);
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

            VkLayerProperties* supported_validation_layers = _vgpu_alloca(VkLayerProperties, instance_layer_count);
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
            vgpu_destroy_device(device);
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
                vgpu_destroy_device(device);
                return false;
            }
        }
        else
        {
            result = vkCreateDebugReportCallbackEXT(vk.instance, &debug_report_create_info, NULL, &vk.debug_report_callback);
            if (result != VK_SUCCESS) {
                GPU_THROW("Could not create debug report callback");
                vgpu_destroy_device(device);
                return false;
            }
        }
#endif

        /* Enumerate all physical device. */
        vk.physical_device_count = _GPU_MAX_PHYSICAL_DEVICES;
        result = vkEnumeratePhysicalDevices(vk.instance, &vk.physical_device_count, vk.physical_devices);
        if (result != VK_SUCCESS) {
            GPU_THROW("Cannot enumerate physical devices.");
            vgpu_destroy_device(device);
            return false;
        }
    }

    /* Create surface if required. */
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    uint32_t backbuffer_width;
    uint32_t backbuffer_height;
    if (desc->main_context_desc)
    {
        surface = vk_create_surface(desc->main_context_desc->native_handle, &backbuffer_width, &backbuffer_height);
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
            if (desc->preferred_adapter == VGPU_ADAPTER_TYPE_DISCRETE_GPU) {
                score += 1000u;
            }
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            score += 90U;
            if (desc->preferred_adapter == VGPU_ADAPTER_TYPE_INTEGRATED_GPU) {
                score += 1000u;
            }
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            score += 80U;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            score += 70U;
            if (desc->preferred_adapter == VGPU_ADAPTER_TYPE_CPU) {
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
        vgpu_destroy_device(device);
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
    VkQueueFamilyProperties* queue_families = _vgpu_alloca(VkQueueFamilyProperties, queue_count);
    vkGetPhysicalDeviceQueueFamilyProperties(renderer->physical_device, &queue_count, queue_families);

    uint32_t universal_queue_index = 1;
    uint32_t graphics_queue_index = 0;
    uint32_t compute_queue_index = 0;
    uint32_t copy_queue_index = 0;

    if (renderer->queue_families.compute_queue_family == VK_QUEUE_FAMILY_IGNORED)
    {
        renderer->queue_families.compute_queue_family = renderer->queue_families.graphics_queue_family;
        compute_queue_index = _vgpu_min(queue_families[renderer->queue_families.graphics_queue_family].queueCount - 1, universal_queue_index);
        universal_queue_index++;
    }

    if (renderer->queue_families.copy_queue_family == VK_QUEUE_FAMILY_IGNORED)
    {
        renderer->queue_families.copy_queue_family = renderer->queue_families.graphics_queue_family;
        copy_queue_index = _vgpu_min(queue_families[renderer->queue_families.graphics_queue_family].queueCount - 1, universal_queue_index);
        universal_queue_index++;
    }
    else if (renderer->queue_families.copy_queue_family == renderer->queue_families.compute_queue_family)
    {
        copy_queue_index = _vgpu_min(queue_families[renderer->queue_families.compute_queue_family].queueCount - 1, 1u);
    }

    static const float graphics_queue_prio = 0.5f;
    static const float compute_queue_prio = 1.0f;
    static const float transfer_queue_prio = 1.0f;
    float prio[3] = { graphics_queue_prio, compute_queue_prio, transfer_queue_prio };

    uint32_t queue_family_count = 0;
    VkDeviceQueueCreateInfo queue_info[3] = { 0 };
    queue_info[queue_family_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info[queue_family_count].queueFamilyIndex = renderer->queue_families.graphics_queue_family;
    queue_info[queue_family_count].queueCount = _vgpu_min(universal_queue_index, queue_families[renderer->queue_families.graphics_queue_family].queueCount);
    queue_info[queue_family_count].pQueuePriorities = prio;
    queue_family_count++;

    if (renderer->queue_families.compute_queue_family != renderer->queue_families.graphics_queue_family)
    {
        queue_info[queue_family_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[queue_family_count].queueFamilyIndex = renderer->queue_families.compute_queue_family;
        queue_info[queue_family_count].queueCount = _vgpu_min(renderer->queue_families.copy_queue_family == renderer->queue_families.compute_queue_family ? 2u : 1u,
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
        vk.full_screen_exclusive)
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
        vgpu_destroy_device(device);
        return false;
    }
    agpu_vk_init_device(renderer->device);
    vkGetDeviceQueue(renderer->device, renderer->queue_families.graphics_queue_family, graphics_queue_index, &renderer->graphics_queue);
    vkGetDeviceQueue(renderer->device, renderer->queue_families.compute_queue_family, compute_queue_index, &renderer->compute_queue);
    vkGetDeviceQueue(renderer->device, renderer->queue_families.copy_queue_family, copy_queue_index, &renderer->copy_queue);

    /* Create memory allocator. */
    {
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
            .instance = vk.instance,
            .vulkanApiVersion = agpu_vk_get_instance_version()
        };
        result = vmaCreateAllocator(&allocator_info, &renderer->memory_allocator);
        if (result != VK_SUCCESS) {
            GPU_THROW("Cannot create memory allocator.");
            vgpu_destroy_device(device);
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
    renderer->limits.max_compute_work_group_size_x = gpu_props.limits.maxComputeWorkGroupSize[0];
    renderer->limits.max_compute_work_group_size_y = gpu_props.limits.maxComputeWorkGroupSize[1];
    renderer->limits.max_compute_work_group_size_z = gpu_props.limits.maxComputeWorkGroupSize[2];

    /* Create main context and set it as active. */
    if (surface != VK_NULL_HANDLE)
    {
        vgpu_vk_context* context = VGPU_ALLOC_HANDLE(vgpu_vk_context);
        context->surface = surface;
        context->width = backbuffer_width;
        context->height = backbuffer_height;
        context->handle = VK_NULL_HANDLE;
        context->max_inflight_frames = desc->main_context_desc->max_inflight_frames;
        context->preferred_image_count = desc->main_context_desc->image_count;
        context->srgb = desc->main_context_desc->srgb;
        context->present_mode = get_vk_present_mode(desc->main_context_desc->present_mode);

        if (!vk_init_or_update_context(renderer, (vgpu_context*)context))
        {
            GPU_THROW("Cannot create main context.");
            vgpu_destroy_device(device);
            return false;
        }

        renderer->main_context = context;
        renderer->context = context;
    }

    /* Increase device being created. */
    vk.device_count++;

    return device;
}

agpu_driver vulkan_driver = {
    VGPU_BACKEND_VULKAN,
    vk_create_device
};
