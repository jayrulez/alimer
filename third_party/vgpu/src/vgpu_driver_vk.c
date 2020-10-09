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

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

/* Needed by vma */
PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;

#include "vma/vk_mem_alloc.h"

// Functions that don't require an instance
#define VGPU_FOREACH_ANONYMOUS(X)\
    X(vkCreateInstance)

// Functions that require an instance but don't require a device
#define VGPU_FOREACH_INSTANCE(X)\
    X(vkDestroyInstance)\
    X(vkCreateDebugUtilsMessengerEXT)\
    X(vkDestroyDebugUtilsMessengerEXT)\
    X(vkEnumeratePhysicalDevices)\
    X(vkGetPhysicalDeviceProperties)\
    X(vkGetPhysicalDeviceFeatures)\
    X(vkGetPhysicalDeviceMemoryProperties)\
    X(vkGetPhysicalDeviceQueueFamilyProperties)\
    X(vkCreateDevice)\
    X(vkDestroyDevice)\
    X(vkGetDeviceQueue)


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
VGPU_FOREACH_DEVICE(VGPU_DECLARE);

#define VOIDP_TO_U64(x) (((union { uint64_t u; void* p; }) { .p = x }).u)

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

/* Global data */
static struct {
    bool available_initialized;
    bool available;

#if defined(_WIN32)
    HMODULE library;
#else
    void* library;
#endif

    agpu_config config;
    agpu_caps caps;

    VkInstance instance;
    VkDebugUtilsMessengerEXT messenger;

    VkPhysicalDevice physical_device;

    VkDevice device;
    VkQueue queue;

    VmaAllocator memory_allocator;
} vk;

static VkBool32 _vgpu_vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT flags, const VkDebugUtilsMessengerCallbackDataEXT* data, void* context);

static bool vk_init(const char* app_name, const agpu_config* config)
{
    memcpy(&vk.config, config, sizeof(vk.config));

    VkResult result = VK_SUCCESS;

    const char* layers[] = {
        "VK_LAYER_KHRONOS_validation",
    };

    const char* extensions[] = {
        "VK_EXT_debug_utils"
    };

    VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &(VkApplicationInfo) {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = app_name,
            .apiVersion = VK_MAKE_VERSION(1, 1, 0),
        },
        .enabledLayerCount = config->debug ? VGPU_COUNT_OF(layers) : 0,
        .ppEnabledLayerNames = layers,
        .enabledExtensionCount = config->debug ? VGPU_COUNT_OF(extensions) : 0,
        .ppEnabledExtensionNames = extensions
    };

    if (vkCreateInstance(&instance_info, NULL, &vk.instance) != VK_SUCCESS) {
        return false;
    }

    VGPU_FOREACH_INSTANCE(VGPU_LOAD_INSTANCE);
    vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(vk.instance, "vkGetDeviceProcAddr");

    if (config->debug)
    {
        VkDebugUtilsMessengerCreateInfoEXT messengerInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = _vgpu_vk_debug_callback
        };

        if (vkCreateDebugUtilsMessengerEXT(vk.instance, &messengerInfo, NULL, &vk.messenger)) {
            vgpu_shutdown();
            return false;
        }
    }

    // Device
    {
        uint32_t deviceCount = 1;
        if (vkEnumeratePhysicalDevices(vk.instance, &deviceCount, &vk.physical_device) < 0) {
            vgpu_shutdown();
            return false;
        }

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(vk.physical_device, &deviceProperties);

        uint32_t queueFamilyIndex = ~0u;
        VkQueueFamilyProperties queueFamilies[4];
        uint32_t queueFamilyCount = VGPU_COUNT_OF(queueFamilies);
        vkGetPhysicalDeviceQueueFamilyProperties(vk.physical_device, &queueFamilyCount, queueFamilies);

        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            uint32_t flags = queueFamilies[i].queueFlags;
            if ((flags & VK_QUEUE_GRAPHICS_BIT) && (flags & VK_QUEUE_COMPUTE_BIT)) {
                queueFamilyIndex = i;
                break;
            }
        }

        if (queueFamilyIndex == ~0u) {
            vgpu_shutdown();
            return false;
        }

        VkDeviceQueueCreateInfo queueInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = &(float) { 1.0f }
        };

        VkDeviceCreateInfo deviceInfo = {
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
          .queueCreateInfoCount = 1,
          .pQueueCreateInfos = &queueInfo
        };

        if (vkCreateDevice(vk.physical_device, &deviceInfo, NULL, &vk.device) != VK_SUCCESS) {
            vgpu_shutdown();
            return false;
        }

        vkGetDeviceQueue(vk.device, queueFamilyIndex, 0, &vk.queue);

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

static void vk_query_caps(agpu_caps* caps) {
    *caps = vk.caps;
}

/* Buffer */
static vgpu_buffer vk_buffer_create(const agpu_buffer_info* info)
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

static void vk_begin_render_pass(const agpu_render_pass_info* info)
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

static agpu_renderer* vk_create_renderer(void)
{
    static agpu_renderer renderer = { 0 };
    ASSIGN_DRIVER(vk);
    return &renderer;
}

agpu_driver vulkan_driver = {
    VGPU_BACKEND_TYPE_VULKAN,
    vk_is_supported,
    vk_create_renderer
};

#endif /* defined(VGPU_DRIVER_VULKAN)  */
