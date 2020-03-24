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

#include "agpu_internal.h"

#if defined(GPU_VK_BACKEND)
#ifndef VK_NO_PROTOTYPES
#   define VK_NO_PROTOTYPES
#endif

#if defined(_WIN32)

#ifndef NOMINMAX
#   define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN
#endif

#ifndef VK_USE_PLATFORM_WIN32_KHR
#   define VK_USE_PLATFORM_WIN32_KHR
#endif
#define VK_CREATE_SURFACE_FN vkCreateWin32SurfaceKHR
#else
#include <dlfcn.h>
#endif

#define GPU_THROW(s) if (vk_state.config.callback) { vk_state.config.callback(vk_state.config.context, s, true); }
#define GPU_CHECK(c, s) if (!(c)) { GPU_THROW(s); }

#include <vector>
#include <vulkan/vulkan.h>
static PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
static PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#define VK_CHECK(f) do { VkResult r = (f); GPU_CHECK(r >= 0, vk_get_error_string(r)); } while (0)

// Functions that don't require an instance
#define VULKAN_FOREACH_ANONYMOUS(X)\
    X(vkCreateInstance);\
    X(vkEnumerateInstanceExtensionProperties);\
    X(vkEnumerateInstanceLayerProperties);\
    X(vkEnumerateInstanceVersion)

// Functions that require an instance but don't require a device
#define VULKAN_FOREACH_INSTANCE(X)\
    X(vkDestroyInstance);\
    X(vkCreateDebugUtilsMessengerEXT);\
    X(vkDestroyDebugUtilsMessengerEXT);\
    X(vkEnumeratePhysicalDevices);\
    X(vkGetPhysicalDeviceProperties);\
    X(vkGetPhysicalDeviceMemoryProperties);\
    X(vkGetPhysicalDeviceQueueFamilyProperties);\
    X(vkCreateDevice);\
    X(vkDestroyDevice);\
    X(vkDestroySurfaceKHR);\
    X(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);\
    X(vkGetPhysicalDeviceSurfaceFormatsKHR);\
    X(vkGetPhysicalDeviceSurfacePresentModesKHR);\
    X(vkGetPhysicalDeviceSurfaceSupportKHR)


// Functions that require a device
#define VULKAN_FOREACH_DEVICE(X)\
    X(vkGetDeviceQueue);\
    X(vkSetDebugUtilsObjectNameEXT);\
    X(vkQueueSubmit);\
    X(vkDeviceWaitIdle);\
    X(vkCreateCommandPool);\
    X(vkDestroyCommandPool);\
    X(vkAllocateCommandBuffers);\
    X(vkFreeCommandBuffers);\
    X(vkBeginCommandBuffer);\
    X(vkEndCommandBuffer);\
    X(vkCreateFence);\
    X(vkDestroyFence);\
    X(vkWaitForFences);\
    X(vkResetFences);\
    X(vkCmdPipelineBarrier);\
    X(vkCreateBuffer);\
    X(vkDestroyBuffer);\
    X(vkGetBufferMemoryRequirements);\
    X(vkBindBufferMemory);\
    X(vkCmdCopyBuffer);\
    X(vkCreateImage);\
    X(vkDestroyImage);\
    X(vkGetImageMemoryRequirements);\
    X(vkBindImageMemory);\
    X(vkCmdCopyBufferToImage);\
    X(vkAllocateMemory);\
    X(vkFreeMemory);\
    X(vkMapMemory);\
    X(vkUnmapMemory);\
    X(vkFlushMappedMemoryRanges);\
    X(vkInvalidateMappedMemoryRanges);\
    X(vkCreateSampler);\
    X(vkDestroySampler);\
    X(vkCreateRenderPass);\
    X(vkDestroyRenderPass);\
    X(vkCmdBeginRenderPass);\
    X(vkCmdEndRenderPass);\
    X(vkCreateImageView);\
    X(vkDestroyImageView);\
    X(vkCreateFramebuffer);\
    X(vkDestroyFramebuffer);\
    X(vkCreateShaderModule);\
    X(vkDestroyShaderModule);\
    X(vkCreateSwapchainKHR);\
    X(vkDestroySwapchainKHR);\
    X(vkGetSwapchainImagesKHR);\
    X(vkAcquireNextImageKHR);\
    X(vkQueuePresentKHR)

#define GPU_DECLARE(fn) static PFN_##fn fn
#define GPU_LOAD_ANONYMOUS(fn) fn = (PFN_##fn) vkGetInstanceProcAddr(NULL, #fn)
#define GPU_LOAD_INSTANCE(fn) fn = (PFN_##fn) vkGetInstanceProcAddr(vk_state.instance, #fn)
#define GPU_LOAD_DEVICE(fn) fn = (PFN_##fn) vkGetDeviceProcAddr(vk_state.device, #fn)

// Declare function pointers
VULKAN_FOREACH_ANONYMOUS(GPU_DECLARE);
VULKAN_FOREACH_INSTANCE(GPU_DECLARE);
VULKAN_FOREACH_DEVICE(GPU_DECLARE);

#if defined(VK_USE_PLATFORM_WIN32_KHR)
static PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
#endif

typedef struct gpu_ref {
    VkObjectType type;
    void* handle;
    void* handle2;
} gpu_ref;

typedef struct gpu_destroy_vector {
    gpu_ref* data;
    size_t capacity;
    size_t length;
} gpu_destroy_vector;

typedef struct {
    VkFence fence;
    VkCommandBuffer command_buffer;
    gpu_destroy_vector destroy;
} gpu_frame;

typedef struct gpu_buffer_t {
    VkBuffer handle;
    VmaAllocation allocation;
} gpu_buffer_t;

typedef struct gpu_swapchain_t {
    VkSurfaceKHR  surface;
    VkSwapchainKHR handle;
} gpu_swapchain_t;

typedef struct gpu_swapchain_t* gpu_swapchain;

static struct {
    agpu_config config;
    bool headless;
    uint32_t max_inflight_frames;

#if defined(_WIN32)
    HMODULE library;
#else
    void* library;
#endif

    uint32_t api_version;
    VkInstance instance;
    VkDebugUtilsMessengerEXT messenger;
    VkSurfaceKHR surface;

    VkPhysicalDevice physical_device;
    uint32_t graphics_queue_family;
    uint32_t compute_queue_family;
    uint32_t transfer_queue_family;
    VkDevice device;
    VkQueue graphics_queue;
    VkQueue compute_queue;
    VkQueue transfer_queue;
    VmaAllocator memory_allocator;
    VkCommandPool command_pool;

    gpu_swapchain swapchains[16];

    gpu_frame frames[3];
    uint32_t frame;
} vk_state;

static VkBool32 vulkan_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT flags, const VkDebugUtilsMessengerCallbackDataEXT* data, void* context);
static const char* vk_get_error_string(VkResult result);
static void vk_set_name(void* object, VkObjectType type, const char* name);
static void vk_queue_destroy(void* handle, void* handle2, VkObjectType type);
static void vk_destroy_frame(gpu_frame* frame);
static void vk_backend_shutdown(void);

/* Swapchain functions */
static bool gpu_create_swapchain(VkSurfaceKHR surface, const agpu_swapchain_desc* desc, gpu_swapchain* result);
static void gpu_destroy_swapchain(gpu_swapchain swapchain);

static agpu_backend vk_get_backend(void) {
    return AGPU_BACKEND_VULKAN;
}

static bool vk_backend_initialize(const agpu_config* config) {
    // Copy settings
    memcpy(&vk_state.config, config, sizeof(*config));
    if (vk_state.config.flags & AGPU_CONFIG_FLAGS_HEADLESS) {
        vk_state.headless = true;
    }

    vk_state.max_inflight_frames = _gpu_min(config->max_inflight_frames, 3);

    vk_state.graphics_queue_family = VK_QUEUE_FAMILY_IGNORED;
    vk_state.compute_queue_family = VK_QUEUE_FAMILY_IGNORED;
    vk_state.transfer_queue_family = VK_QUEUE_FAMILY_IGNORED;

#if defined(_WIN32)
    vk_state.library = LoadLibraryA("vulkan-1.dll");
    if (!vk_state.library)
        return false;

    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(vk_state.library, "vkGetInstanceProcAddr");
#elif defined(__APPLE__)
    vk_state.library = dlopen("libvulkan.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!vk_state.library)
        vk_state.library = dlopen("libvulkan.1.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!vk_state.library)
        vk_state.library = dlopen("libMoltenVK.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!vk_state.library)
        return false;

    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(vk_state.library, "vkGetInstanceProcAddr");
#else
    vk_state.library = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    if (!vk_state.library)
        vk_state.library = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!vk_state.library)
        return false;

    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(vk_state.library, "vkGetInstanceProcAddr");
#endif

    // Load all the global functions like vkCreateInstance
    VULKAN_FOREACH_ANONYMOUS(GPU_LOAD_ANONYMOUS);

    /* Detect API version */
    vk_state.api_version = VK_MAKE_VERSION(1, 0, 0);
    if (vkEnumerateInstanceVersion && vkEnumerateInstanceVersion(&vk_state.api_version) != VK_SUCCESS) {
        vk_state.api_version = VK_MAKE_VERSION(1, 0, 0);
    }

    /* Setup extensions and layers */
    uint32_t layersCount = 0;
    const char* layers[8] = { 0 };

    if (vk_state.config.flags & AGPU_CONFIG_FLAGS_VALIDATION) {
        uint32_t num_layer_props;
        VkLayerProperties queried_layers[64];
        vkEnumerateInstanceLayerProperties(&num_layer_props, queried_layers);
        /* Prefer VK_LAYER_KHRONOS_validation over VK_LAYER_LUNARG_standard_validation */
        bool layer_found = false;
        uint32_t i;
        for (i = 0; i < num_layer_props; i++) {
            if (strcmp(queried_layers[i].layerName, "VK_LAYER_KHRONOS_validation") == 0) {
                layers[layersCount++] = "VK_LAYER_KHRONOS_validation";
                layer_found = true;
                break;
            }
        }

        if (!layer_found) {
            for (i = 0; i < num_layer_props; i++) {
                if (strcmp(queried_layers[i].layerName, "VK_LAYER_LUNARG_standard_validation") == 0) {
                    layers[layersCount++] = "VK_LAYER_LUNARG_standard_validation";
                    break;
                }
            }
        }
    }

    /* Query extensions and setup instance extensions */
    uint32_t num_ext_props;
    VkExtensionProperties queried_exts[64];
    vkEnumerateInstanceExtensionProperties(NULL, &num_ext_props, queried_exts);
    uint32_t extensionCount = 0;
    const char* extensions[64] = { 0 };
    uint32_t i;
    bool supports_debug_utils = false;

    for (i = 0; i < num_ext_props; i++) {
        if (!vk_state.headless) {
            if (strcmp(queried_exts[i].extensionName, "VK_KHR_surface") == 0)
                extensions[extensionCount++] = "VK_KHR_surface";
#if defined(_WIN32)
            else if (strcmp(queried_exts[i].extensionName, "VK_KHR_win32_surface") == 0)
                extensions[extensionCount++] = "VK_KHR_win32_surface";
#elif defined(__APPLE__)
            else if (strcmp(queried_exts[i].extensionName, "VK_MVK_macos_surface") == 0)
                extensions[extensionCount++] = "VK_MVK_macos_surface";
            else if (strcmp(queried_exts[i].extensionName, "VK_EXT_metal_surface") == 0)
                extensions[extensionCount++] = "VK_EXT_metal_surface";
#elif defined(__ANDROID__)
            else if (strcmp(queried_exts[i].extensionName, "VK_KHR_android_surface") == 0)
                extensions[extensionCount++] = "VK_KHR_android_surface";
#elif defined(__linux__)
            else if (strcmp(queried_exts[i].extensionName, "VK_KHR_xlib_surface") == 0)
                extensions[extensionCount++] = "VK_KHR_xlib_surface";
            else if (strcmp(queried_exts[i].extensionName, "VK_KHR_xcb_surface") == 0)
                extensions[extensionCount++] = "VK_EXT_metal_surface";
#elif defined(_AGPU_WAYLAND)
            else if (strcmp(queried_exts[i].extensionName, "VK_KHR_wayland_surface") == 0)
                extensions[extensionCount++] = "VK_KHR_wayland_surface";
#endif
        }

        if (strcmp(queried_exts[i].extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0)
            extensions[extensionCount++] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
        else if (strcmp(queried_exts[i].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
            extensions[extensionCount++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
            supports_debug_utils = true;
        }
    }

    VkApplicationInfo applicationInfo = {};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.apiVersion = vk_state.api_version;
    VkInstanceCreateInfo instanceInfo = {};

    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &applicationInfo;
    instanceInfo.enabledLayerCount = layersCount;
    instanceInfo.ppEnabledLayerNames = layers;
    instanceInfo.enabledExtensionCount = extensionCount;
    instanceInfo.ppEnabledExtensionNames = extensions;

    if (vkCreateInstance(&instanceInfo, NULL, &vk_state.instance)) {
        vk_backend_shutdown();
        return false;
    }

    // Load the instance functions
    VULKAN_FOREACH_INSTANCE(GPU_LOAD_INSTANCE);

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(vk_state.instance, "vkCreateWin32SurfaceKHR");
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
#endif

    // Debug callbacks
    if (vk_state.config.flags & AGPU_CONFIG_FLAGS_VALIDATION && supports_debug_utils) {
        VkDebugUtilsMessengerCreateInfoEXT messengerInfo = {};

        messengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        messengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        messengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        messengerInfo.pfnUserCallback = vulkan_messenger_callback;

        if (vkCreateDebugUtilsMessengerEXT(vk_state.instance, &messengerInfo, NULL, &vk_state.messenger)) {
            vk_backend_shutdown();
            return false;
        }
    }

    /* Create main surface */
    if (!vk_state.headless) {
        VkResult result;
#if defined(_WIN32)
        VkWin32SurfaceCreateInfoKHR surface_create_info = {};
        surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surface_create_info.pNext = NULL;
        surface_create_info.flags = 0;
        surface_create_info.hinstance = GetModuleHandle(NULL);
        surface_create_info.hwnd = (HWND)vk_state.config.swapchain->native_handle;
#elif defined(__APPLE__)
#elif defined(__ANDROID__)
        const VkAndroidSurfaceCreateInfoKHR surface_create_info = {
            .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
            .pNext = NULL,
            .flags = 0,
        };

        result = vkCreateAndroidSurfaceKHR(vk_state.instance, &surface_create_info, NULL, &state.surface);
#elif defined(__linux__)
#elif defined(_AGPU_WAYLAND)
#endif
        result = VK_CREATE_SURFACE_FN(vk_state.instance, &surface_create_info, NULL, &vk_state.surface);
        if (result != VK_SUCCESS) {
            vk_backend_shutdown();
            return false;
        }
    }

    // Enumerate physical devices and create it
    uint32_t deviceCount = 16;
    VkPhysicalDevice physicalDevices[16];
    if (vkEnumeratePhysicalDevices(vk_state.instance, &deviceCount, physicalDevices) != VK_SUCCESS) {
        vk_backend_shutdown();
        return false;
    }

    // Pick a suitable physical device based on user's preference.
    VkPhysicalDeviceProperties device_props;
    uint32_t best_device_score = 0U;
    uint32_t best_device_index = -1;
    const bool highPerformance = true;
    for (uint32_t i = 0; i < deviceCount; ++i) {
        vkGetPhysicalDeviceProperties(physicalDevices[i], &device_props);
        uint32_t score = 0U;

        if (device_props.apiVersion >= VK_API_VERSION_1_2) {
            score += 10000u;
        }
        else if (device_props.apiVersion >= VK_API_VERSION_1_1) {
            score += 5000u;
        }

        switch (device_props.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            score += 100U;
            if (highPerformance) { score += 1000u; }
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            score += 90U;
            if (!highPerformance) { score += 1000u; }
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            score += 80u;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            score += 70u;
            break;
        default: score += 10u;
        }
        if (score > best_device_score) {
            best_device_index = i;
            best_device_score = score;
        }
    }
    if (best_device_index == -1) {
        vk_backend_shutdown();
        return false;
    }

    vk_state.physical_device = physicalDevices[best_device_index];
    vkGetPhysicalDeviceProperties(vk_state.physical_device, &device_props);

    uint32_t queue_count;
    vkGetPhysicalDeviceQueueFamilyProperties(vk_state.physical_device, &queue_count, NULL);
    std::vector<VkQueueFamilyProperties> queue_props(queue_count);
    vkGetPhysicalDeviceQueueFamilyProperties(vk_state.physical_device, &queue_count, queue_props.data());

    for (uint32_t i = 0; i < queue_count; i++)
    {
        VkBool32 supported = vk_state.surface == VK_NULL_HANDLE;
        if (vk_state.surface != VK_NULL_HANDLE)
            vkGetPhysicalDeviceSurfaceSupportKHR(vk_state.physical_device, i, vk_state.surface, &supported);

        static const VkQueueFlags required = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT;
        if (supported && ((queue_props[i].queueFlags & required) == required))
        {
            vk_state.graphics_queue_family = i;
            break;
        }
    }

    for (uint32_t i = 0; i < queue_count; i++)
    {
        static const VkQueueFlags required = VK_QUEUE_COMPUTE_BIT;
        if (i != vk_state.graphics_queue_family && (queue_props[i].queueFlags & required) == required)
        {
            vk_state.compute_queue_family = i;
            break;
        }
    }

    for (uint32_t i = 0; i < queue_count; i++)
    {
        static const VkQueueFlags required = VK_QUEUE_TRANSFER_BIT;
        if (i != vk_state.graphics_queue_family
            && i != vk_state.compute_queue_family
            && (queue_props[i].queueFlags & required) == required)
        {
            vk_state.transfer_queue_family = i;
            break;
        }
    }

    /* Dedicated transfer queue. */
    if (vk_state.transfer_queue_family == VK_QUEUE_FAMILY_IGNORED)
    {
        for (uint32_t i = 0; i < queue_count; i++)
        {
            static const VkQueueFlags required = VK_QUEUE_TRANSFER_BIT;
            if (i != vk_state.graphics_queue_family && (queue_props[i].queueFlags & required) == required)
            {
                vk_state.transfer_queue_family = i;
                break;
            }
        }
    }

    if (vk_state.graphics_queue_family == VK_QUEUE_FAMILY_IGNORED) {
        vk_backend_shutdown();
        return false;
    }

    uint32_t universal_queue_index = 1;
    uint32_t compute_queue_index = 0;
    uint32_t transfer_queue_index = 0;

    if (vk_state.compute_queue_family == VK_QUEUE_FAMILY_IGNORED) {
        vk_state.compute_queue_family = vk_state.graphics_queue_family;
        compute_queue_index = VMA_MIN(queue_props[vk_state.graphics_queue_family].queueCount - 1, universal_queue_index);
        universal_queue_index++;
    }

    if (vk_state.transfer_queue_family == VK_QUEUE_FAMILY_IGNORED) {
        vk_state.transfer_queue_family = vk_state.graphics_queue_family;
        transfer_queue_index = VMA_MIN(queue_props[vk_state.graphics_queue_family].queueCount - 1, universal_queue_index);
        universal_queue_index++;
    }
    else if (vk_state.transfer_queue_family == vk_state.compute_queue_family) {
        transfer_queue_index = VMA_MIN(queue_props[vk_state.compute_queue_family].queueCount - 1, 1u);
    }

    static const float graphics_queue_prio = 0.5f;
    static const float compute_queue_prio = 1.0f;
    static const float transfer_queue_prio = 1.0f;
    float prio[3] = { graphics_queue_prio, compute_queue_prio, transfer_queue_prio };

    unsigned queue_family_count = 0;
    VkDeviceQueueCreateInfo queue_info[3] = {};

    queue_info[queue_family_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info[queue_family_count].queueFamilyIndex = vk_state.graphics_queue_family;
    queue_info[queue_family_count].queueCount = VMA_MIN(universal_queue_index, queue_props[vk_state.graphics_queue_family].queueCount);
    queue_info[queue_family_count].pQueuePriorities = prio;
    queue_family_count++;

    if (vk_state.compute_queue_family != vk_state.graphics_queue_family)
    {
        queue_info[queue_family_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[queue_family_count].queueFamilyIndex = vk_state.compute_queue_family;
        queue_info[queue_family_count].queueCount = VMA_MIN(vk_state.transfer_queue_family == vk_state.compute_queue_family ? 2u : 1u,
            queue_props[vk_state.compute_queue_family].queueCount);
        queue_info[queue_family_count].pQueuePriorities = prio + 1;
        queue_family_count++;
    }

    if (vk_state.transfer_queue_family != vk_state.graphics_queue_family
        && vk_state.transfer_queue_family != vk_state.compute_queue_family)
    {
        queue_info[queue_family_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[queue_family_count].queueFamilyIndex = vk_state.transfer_queue_family;
        queue_info[queue_family_count].queueCount = 1;
        queue_info[queue_family_count].pQueuePriorities = prio + 2;
        queue_family_count++;
    }

    VkPhysicalDeviceFeatures device_features;
    memset(&device_features, 0, sizeof(device_features));

    VmaVector<const char*, VmaStlAllocator<const char*> > device_exts(VmaStlAllocator<char>(nullptr));
    if (!vk_state.headless) {
        device_exts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

    device_exts.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);


    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = queue_family_count;
    deviceCreateInfo.pQueueCreateInfos = queue_info;
    deviceCreateInfo.enabledExtensionCount = (uint32_t)device_exts.size();
    deviceCreateInfo.ppEnabledExtensionNames = device_exts.data();
    deviceCreateInfo.pEnabledFeatures = &device_features;

    if (vkCreateDevice(vk_state.physical_device, &deviceCreateInfo, NULL, &vk_state.device)) {
        vk_backend_shutdown();
        return false;
    }

    // Load device functions
    VULKAN_FOREACH_DEVICE(GPU_LOAD_DEVICE);

    vkGetDeviceQueue(vk_state.device, vk_state.graphics_queue_family, 0, &vk_state.graphics_queue);
    vkGetDeviceQueue(vk_state.device, vk_state.compute_queue_family, compute_queue_index, &vk_state.compute_queue);
    vkGetDeviceQueue(vk_state.device, vk_state.transfer_queue_family, transfer_queue_index, &vk_state.transfer_queue);

    /* Create vma allocator */
    VmaVulkanFunctions vma_funcs = {};
    vma_funcs.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
    vma_funcs.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
    vma_funcs.vkAllocateMemory = vkAllocateMemory;
    vma_funcs.vkFreeMemory = vkFreeMemory;
    vma_funcs.vkMapMemory = vkMapMemory;
    vma_funcs.vkUnmapMemory = vkUnmapMemory;
    vma_funcs.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
    vma_funcs.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
    vma_funcs.vkBindBufferMemory = vkBindBufferMemory;
    vma_funcs.vkBindImageMemory = vkBindImageMemory;
    vma_funcs.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
    vma_funcs.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
    vma_funcs.vkCreateBuffer = vkCreateBuffer;
    vma_funcs.vkDestroyBuffer = vkDestroyBuffer;
    vma_funcs.vkCreateImage = vkCreateImage;
    vma_funcs.vkDestroyImage = vkDestroyImage;
    vma_funcs.vkCmdCopyBuffer = vkCmdCopyBuffer;

    VmaAllocatorCreateInfo vma_info = {};
    vma_info.flags = 0;
    vma_info.physicalDevice = vk_state.physical_device;
    vma_info.device = vk_state.device;
    vma_info.pVulkanFunctions = &vma_funcs;
    vma_info.instance = vk_state.instance;

    if (vmaCreateAllocator(&vma_info, &vk_state.memory_allocator) != VK_SUCCESS) {
        return false;
    }

    VkCommandPoolCreateInfo commandPoolInfo = {};
    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolInfo.queueFamilyIndex = vk_state.graphics_queue_family;

    if (vkCreateCommandPool(vk_state.device, &commandPoolInfo, NULL, &vk_state.command_pool)) {
        vk_backend_shutdown();
        return false;
    }

    /* Create main Swapchain if required. */
    if (!vk_state.headless) {
        if (!gpu_create_swapchain(vk_state.surface, config->swapchain, &vk_state.swapchains[0])) {
            return false;
        }
    }

    const VkCommandBufferAllocateInfo command_buffer_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        vk_state.command_pool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        1u
    };

    const VkFenceCreateInfo fence_create_info = {
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        nullptr,
        VK_FENCE_CREATE_SIGNALED_BIT
    };

    for (uint32_t i = 0; i < COUNTOF(vk_state.frames); i++) {
        if (vkAllocateCommandBuffers(vk_state.device, &command_buffer_info, &vk_state.frames[i].command_buffer)) {
            vk_backend_shutdown();
            return false;
        }

        if (vkCreateFence(vk_state.device, &fence_create_info, NULL, &vk_state.frames[i].fence)) {
            vk_backend_shutdown();
            return false;
        }
    }

    return true;
}

static void vk_backend_shutdown(void) {
    if (vk_state.device) vkDeviceWaitIdle(vk_state.device);

    /* Destroy main swap chain */
    if (!vk_state.headless) {
        gpu_destroy_swapchain(vk_state.swapchains[0]);
    }

    /* Release frame data. */
    for (uint32_t i = 0; i < COUNTOF(vk_state.frames); i++) {
        gpu_frame* frame = &vk_state.frames[i];

        vk_destroy_frame(frame);
        free(frame->destroy.data);

        if (frame->fence) vkDestroyFence(vk_state.device, frame->fence, NULL);
        if (frame->command_buffer) vkFreeCommandBuffers(vk_state.device, vk_state.command_pool, 1, &frame->command_buffer);
    }

    if (vk_state.command_pool) {
        vkDestroyCommandPool(vk_state.device, vk_state.command_pool, NULL);
    }

    if (vk_state.memory_allocator) {
        VmaStats stats;
        vmaCalculateStats(vk_state.memory_allocator, &stats);
        if (stats.total.usedBytes > 0) {
            GPU_THROW("GPU memory allocated is leaked");
        }
        //LOGI("Total device memory leaked: {} bytes.", stats.total.usedBytes);

        vmaDestroyAllocator(vk_state.memory_allocator);
    }

    if (vk_state.device) vkDestroyDevice(vk_state.device, NULL);

    if (vk_state.surface) vkDestroySurfaceKHR(vk_state.instance, vk_state.surface, NULL);
    if (vk_state.messenger) vkDestroyDebugUtilsMessengerEXT(vk_state.instance, vk_state.messenger, NULL);
    if (vk_state.instance) vkDestroyInstance(vk_state.instance, NULL);

#if defined(_WIN32)
    FreeLibrary(vk_state.library);
#else
    dlclose(vk_state.library);
#endif
    memset(&vk_state, 0, sizeof(vk_state));
}

static void vk_backend_wait_idle(void) {
    VK_CHECK(vkDeviceWaitIdle(vk_state.device));
}

static void vk_backend_begin_frame(void) {
    gpu_frame* frame = &vk_state.frames[vk_state.frame];

    // Wait for GPU to process the frame, then reset its scratchpad pool and purge condemned resources
    VK_CHECK(vkWaitForFences(vk_state.device, 1, &frame->fence, VK_FALSE, ~0ull));
    VK_CHECK(vkResetFences(vk_state.device, 1, &frame->fence));
    vk_destroy_frame(frame);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(frame->command_buffer, &begin_info));
}

static void vk_backend_end_frame(void) {
    gpu_frame* frame = &vk_state.frames[vk_state.frame];
    VK_CHECK(vkEndCommandBuffer(frame->command_buffer));

    // Submit pending graphics commands.
    const VkPipelineStageFlags color_attachment_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.pCommandBuffers = &frame->command_buffer;
    submit_info.commandBufferCount = 1;

    VK_CHECK(vkQueueSubmit(vk_state.graphics_queue, 1, &submit_info, frame->fence));

    vk_state.frame = (vk_state.frame + 1) % COUNTOF(vk_state.frames);
}

/* Buffer */
/*bool gpu_create_buffer(const gpu_buffer_desc* desc, gpu_buffer* result) {
    gpu_buffer buffer = _GPU_ALLOC_HANDLE(gpu_buffer);
    *result = buffer;

    if (buffer == NULL) {
        return false;
    }

    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VkBufferCreateInfo buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size = desc->size;
    buffer_info.usage = usage;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VmaAllocationInfo allocation_info;
    VkResult vk_result = vmaCreateBuffer(vk_state.memory_allocator, &buffer_info, &alloc_info,
        &buffer->handle, &buffer->allocation,
        &allocation_info);
    if (vk_result != VK_SUCCESS) {
        return false;
    }

    vk_set_name(buffer->handle, VK_OBJECT_TYPE_BUFFER, desc->name);
    return true;
}

void gpu_destroy_buffer(gpu_buffer buffer) {
    if (buffer->handle && buffer->allocation) {
        vk_queue_destroy(buffer->handle, buffer->allocation, VK_OBJECT_TYPE_BUFFER);
    }

    free(buffer);
}*/

/* Swapchain */
bool gpu_create_swapchain(VkSurfaceKHR surface, const agpu_swapchain_desc* desc, gpu_swapchain* result) {
    VkResult vk_result = VK_SUCCESS;
    gpu_swapchain swapchain = new gpu_swapchain_t();
    *result = swapchain;

    if (swapchain == NULL) {
        return false;
    }

    // Get physical device surface properties and formats
    VkSurfaceCapabilitiesKHR surface_caps;
    vk_result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_state.physical_device, surface, &surface_caps);
    if (vk_result != VK_SUCCESS) {
        return false;
    }

    uint32_t format_count;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(vk_state.physical_device, surface, &format_count, NULL) != VK_SUCCESS) {
        return false;
    }

    std::vector<VkSurfaceFormatKHR> formats(format_count);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(vk_state.physical_device, surface, &format_count, formats.data()) != VK_SUCCESS) {
        return false;
    }

    const bool srgb_backbuffer_enable = true;
    VkSurfaceFormatKHR format;
    if (format_count == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
    {
        format = formats[0];
        format.format = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else
    {
        if (format_count == 0)
        {
            GPU_THROW("Vulkan: Surface has no formats.");
            return false;
        }

        bool found = false;
        for (uint32_t i = 0; i < format_count; i++)
        {
            if (srgb_backbuffer_enable)
            {
                if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB
                    || formats[i].format == VK_FORMAT_B8G8R8A8_SRGB
                    || formats[i].format == VK_FORMAT_A8B8G8R8_SRGB_PACK32)
                {
                    format = formats[i];
                    found = true;
                }
            }
            else
            {
                if (formats[i].format == VK_FORMAT_R8G8B8A8_UNORM
                    || formats[i].format == VK_FORMAT_B8G8R8A8_UNORM
                    || formats[i].format == VK_FORMAT_A8B8G8R8_UNORM_PACK32)
                {
                    format = formats[i];
                    found = true;
                }
            }
        }

        if (!found)
            format = formats[0];
    }

    // Determine the number of images
    uint32_t desired_swapchain_images = 3;
    if (desired_swapchain_images < surface_caps.minImageCount)
        desired_swapchain_images = surface_caps.minImageCount;

    if ((surface_caps.maxImageCount > 0) && (desired_swapchain_images > surface_caps.maxImageCount))
        desired_swapchain_images = surface_caps.maxImageCount;

    // Clamp the target width, height to boundaries.
    VkExtent2D swapchain_size;
    swapchain_size.width = VMA_MAX(VMA_MIN(desc->width, surface_caps.maxImageExtent.width), surface_caps.minImageExtent.width);
    swapchain_size.height = VMA_MAX(VMA_MIN(desc->height, surface_caps.maxImageExtent.height), surface_caps.minImageExtent.height);

    // Enable transfer destination on swap chain images if supported
    VkImageUsageFlags image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (surface_caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
        image_usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    VkSurfaceTransformFlagBitsKHR pre_transform;
    if ((surface_caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) != 0) {
        // We prefer a non-rotated transform
        pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else {
        pre_transform = surface_caps.currentTransform;
    }

    VkCompositeAlphaFlagBitsKHR composite_mode = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if (surface_caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
        composite_mode = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    if (surface_caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
        composite_mode = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if (surface_caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
        composite_mode = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
    if (surface_caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
        composite_mode = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;

    VkSwapchainKHR old_swapchain = swapchain->handle;

    VkSwapchainCreateInfoKHR swapchain_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapchain_info.pNext = NULL;
    swapchain_info.flags = 0;
    swapchain_info.surface = surface;
    swapchain_info.minImageCount = desired_swapchain_images;
    swapchain_info.imageFormat = format.format;
    swapchain_info.imageColorSpace = format.colorSpace;
    swapchain_info.imageExtent = swapchain_size;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = image_usage;
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_info.queueFamilyIndexCount = 0;
    swapchain_info.pQueueFamilyIndices = NULL;
    swapchain_info.preTransform = pre_transform;
    swapchain_info.compositeAlpha = composite_mode;
    swapchain_info.presentMode = present_mode;
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.oldSwapchain = old_swapchain;

    vk_result = vkCreateSwapchainKHR(vk_state.device, &swapchain_info, NULL, &swapchain->handle);
    if (vk_result != VK_SUCCESS) {
        gpu_destroy_swapchain(swapchain);
        return false;
    }

    if (old_swapchain) {
        vkDestroySwapchainKHR(vk_state.device, old_swapchain, NULL);
    }

    return true;
}

static void gpu_destroy_swapchain(gpu_swapchain swapchain) {
    if (swapchain->handle != VK_NULL_HANDLE) {
        vk_queue_destroy(swapchain->handle, NULL, VK_OBJECT_TYPE_SWAPCHAIN_KHR);
    }

    free(swapchain);
}

static VkBool32 vulkan_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT flags, const VkDebugUtilsMessengerCallbackDataEXT* data, void* context) {

    bool error = severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    if (vk_state.config.callback) {
        vk_state.config.callback(vk_state.config.context, data->pMessage, error);
    }
    else {
        if (error)
        {
            GPU_THROW(data->pMessage);
        }
    }

    return false;
}

static const char* vk_get_error_string(VkResult result) {
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

static void vk_set_name(void* handle, VkObjectType type, const char* name) {
    if (name && vk_state.config.flags & AGPU_CONFIG_FLAGS_VALIDATION) {
        VkDebugUtilsObjectNameInfoEXT info = {
            VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            nullptr,
            type,
            (uint64_t)handle,
            name
        };

        VK_CHECK(vkSetDebugUtilsObjectNameEXT(vk_state.device, &info));
    }
}

static void vk_queue_destroy(void* handle, void* handle2, VkObjectType type) {
    gpu_destroy_vector* destroy_list = &vk_state.frames[vk_state.frame].destroy;

    if (destroy_list->length >= destroy_list->capacity) {
        destroy_list->capacity = destroy_list->capacity ? (destroy_list->capacity * 2) : 1;
        destroy_list->data = (gpu_ref*)realloc(destroy_list->data, destroy_list->capacity * sizeof(*destroy_list->data));
        GPU_CHECK(destroy_list->data, "Out of memory");
    }

    destroy_list->data[destroy_list->length++] = { type, handle, handle2 };
}

static void vk_destroy_frame(gpu_frame* frame) {
    for (size_t i = 0; i < frame->destroy.length; i++) {
        gpu_ref* ref = &frame->destroy.data[i];
        switch (ref->type) {
        case VK_OBJECT_TYPE_BUFFER: vmaDestroyBuffer(vk_state.memory_allocator, (VkBuffer)ref->handle, (VmaAllocation)ref->handle2); break;
        case VK_OBJECT_TYPE_IMAGE: vmaDestroyImage(vk_state.memory_allocator, (VkImage)ref->handle, (VmaAllocation)ref->handle2); break;
        case VK_OBJECT_TYPE_IMAGE_VIEW: vkDestroyImageView(vk_state.device, (VkImageView)ref->handle, nullptr); break;
        case VK_OBJECT_TYPE_SAMPLER: vkDestroySampler(vk_state.device, (VkSampler)ref->handle, nullptr); break;
        case VK_OBJECT_TYPE_RENDER_PASS: vkDestroyRenderPass(vk_state.device, (VkRenderPass)ref->handle, nullptr); break;
        case VK_OBJECT_TYPE_FRAMEBUFFER: vkDestroyFramebuffer(vk_state.device, (VkFramebuffer)ref->handle, nullptr); break;
        case VK_OBJECT_TYPE_SWAPCHAIN_KHR: vkDestroySwapchainKHR(vk_state.device, (VkSwapchainKHR)ref->handle, nullptr); break;
        default: GPU_THROW("Unreachable"); break;
        }
    }
    frame->destroy.length = 0;
}

bool agpu_vk_supported(void) {
    return true;
}

agpu_renderer* agpu_create_vk_backend(void) {
    static agpu_renderer renderer = { nullptr };
    renderer.get_backend = vk_get_backend;
    renderer.initialize = vk_backend_initialize;
    renderer.shutdown = vk_backend_shutdown;
    renderer.wait_idle = vk_backend_wait_idle;
    renderer.begin_frame = vk_backend_begin_frame;
    renderer.end_frame = vk_backend_end_frame;
    return &renderer;
}

#else

bool agpu_vk_supported(void) {
    return false;
}

agpu_renderer* agpu_create_vk_backend(void) {
    return nullptr;
}


#endif /* defined(GPU_VK_BACKEND) */
