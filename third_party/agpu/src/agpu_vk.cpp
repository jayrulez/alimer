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
#define GPU_THROW(s) if (vk.config.callback) { vk.config.callback(vk.config.context, s, true); }
#define GPU_CHECK(c, s) if (!(c)) { GPU_THROW(s); }

#include <volk.h>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include <vector>

#define VK_CHECK(f) do { VkResult r = (f); GPU_CHECK(r >= 0, vk_get_error_string(r)); } while (0)

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
    bool validation;
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
} vk;

bool validate_layers(const std::vector<const char*>& required, const std::vector<VkLayerProperties>& available)
{
    for (auto layer : required)
    {
        bool found = false;
        for (auto& available_layer : available)
        {
            if (strcmp(available_layer.layerName, layer) == 0)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            //LOGE("Validation Layer {} not found", layer);
            return false;
        }
    }

    return true;
}

#if defined(AGPU_DEBUG) || defined(AGPU_VALIDATION_LAYERS)
std::vector<const char*> get_optimal_validation_layers(const std::vector<VkLayerProperties>& supported_instance_layers)
{
    std::vector<std::vector<const char*>> validation_layer_priority_list =
    {
        // The preferred validation layer is "VK_LAYER_KHRONOS_validation"
        {"VK_LAYER_KHRONOS_validation"},

        // Otherwise we fallback to using the LunarG meta layer
        {"VK_LAYER_LUNARG_standard_validation"},

        // Otherwise we attempt to enable the individual layers that compose the LunarG meta layer since it doesn't exist
        {
            "VK_LAYER_GOOGLE_threading",
            "VK_LAYER_LUNARG_parameter_validation",
            "VK_LAYER_LUNARG_object_tracker",
            "VK_LAYER_LUNARG_core_validation",
            "VK_LAYER_GOOGLE_unique_objects",
        },

        // Otherwise as a last resort we fallback to attempting to enable the LunarG core layer
        {"VK_LAYER_LUNARG_core_validation"}
    };

    for (auto& validation_layers : validation_layer_priority_list)
    {
        if (validate_layers(validation_layers, supported_instance_layers))
        {
            return validation_layers;
        }

        //LOGW("Couldn't enable validation layers (see log for error) - falling back");
    }

    // Else return nothing
    return {};
}

#endif /* defined(AGPU_DEBUG) || defined(AGPU_VALIDATION_LAYERS) */

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
    if (!agpu_vk_supported()) {
        return false;
    }

    // Copy settings
    memcpy(&vk.config, config, sizeof(*config));
    if (vk.config.flags & AGPU_CONFIG_FLAGS_HEADLESS) {
        vk.headless = true;
    }

#if defined(AGPU_DEBUG) || defined(AGPU_VALIDATION_LAYERS)
    if (config->flags & AGPU_CONFIG_FLAGS_VALIDATION | AGPU_CONFIG_FLAGS_GPU_BASED_VALIDATION) {
        vk.validation = true;
    }
#endif

    vk.max_inflight_frames = _gpu_min(_gpu_def(config->max_inflight_frames, 3), 3);

    vk.graphics_queue_family = VK_QUEUE_FAMILY_IGNORED;
    vk.compute_queue_family = VK_QUEUE_FAMILY_IGNORED;
    vk.transfer_queue_family = VK_QUEUE_FAMILY_IGNORED;

    /* Detect API version */
    vk.api_version = volkGetInstanceVersion();

    /* Setup extensions and layers */
    std::vector<const char*> instance_layers;

#if defined(AGPU_DEBUG) || defined(AGPU_VALIDATION_LAYERS)
    if (vk.validation) {

        uint32_t instance_layer_count;
        VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr));

        std::vector<VkLayerProperties> supported_validation_layers(instance_layer_count);
        VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, supported_validation_layers.data()));

        std::vector<const char*> optimal_validation_layers = get_optimal_validation_layers(supported_validation_layers);
        instance_layers.insert(instance_layers.end(), optimal_validation_layers.begin(), optimal_validation_layers.end());
    }
#endif

    /* Query extensions and setup instance extensions */
    uint32_t num_ext_props;
    VkExtensionProperties queried_exts[64];
    vkEnumerateInstanceExtensionProperties(NULL, &num_ext_props, queried_exts);
    uint32_t extensionCount = 0;
    const char* extensions[64] = { 0 };
    uint32_t i;
    bool supports_debug_utils = false;

    for (i = 0; i < num_ext_props; i++) {
        if (!vk.headless) {
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
    applicationInfo.apiVersion = vk.api_version;
    VkInstanceCreateInfo instanceInfo = {};

    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &applicationInfo;
    instanceInfo.enabledLayerCount = static_cast<uint32_t>(instance_layers.size());
    instanceInfo.ppEnabledLayerNames = instance_layers.data();
    instanceInfo.enabledExtensionCount = extensionCount;
    instanceInfo.ppEnabledExtensionNames = extensions;

    if (vkCreateInstance(&instanceInfo, NULL, &vk.instance)) {
        vk_backend_shutdown();
        return false;
    }

    // Load the instance functions
    volkLoadInstance(vk.instance);

    // Debug callbacks
    if (vk.config.flags & AGPU_CONFIG_FLAGS_VALIDATION && supports_debug_utils) {
        VkDebugUtilsMessengerCreateInfoEXT messengerInfo = {};

        messengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        messengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        messengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        messengerInfo.pfnUserCallback = vulkan_messenger_callback;
        messengerInfo.pUserData = vk.config.context;

        if (vkCreateDebugUtilsMessengerEXT(vk.instance, &messengerInfo, nullptr, &vk.messenger)) {
            vk_backend_shutdown();
            return false;
        }
    }

    /* Create main surface */
    if (!vk.headless) {
        VkResult result;
#if defined(_WIN32)
        VkWin32SurfaceCreateInfoKHR surface_create_info;
        surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surface_create_info.pNext = NULL;
        surface_create_info.flags = 0;
        surface_create_info.hinstance = GetModuleHandle(NULL);
        surface_create_info.hwnd = (HWND)vk.config.swapchain->native_handle;
        result = vkCreateWin32SurfaceKHR(vk.instance, &surface_create_info, nullptr, &vk.surface);
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
        if (result != VK_SUCCESS) {
            vk_backend_shutdown();
            return false;
        }
    }

    // Enumerate physical devices and create it
    uint32_t deviceCount = 16;
    VkPhysicalDevice physicalDevices[16];
    if (vkEnumeratePhysicalDevices(vk.instance, &deviceCount, physicalDevices) != VK_SUCCESS) {
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

    vk.physical_device = physicalDevices[best_device_index];
    vkGetPhysicalDeviceProperties(vk.physical_device, &device_props);

    uint32_t queue_count;
    vkGetPhysicalDeviceQueueFamilyProperties(vk.physical_device, &queue_count, NULL);
    VkQueueFamilyProperties* queue_props = _AGPU_ALLOCAN(VkQueueFamilyProperties, queue_count);
    vkGetPhysicalDeviceQueueFamilyProperties(vk.physical_device, &queue_count, queue_props);

    for (uint32_t i = 0; i < queue_count; i++)
    {
        VkBool32 supported = vk.surface == VK_NULL_HANDLE;
        if (vk.surface != VK_NULL_HANDLE)
            vkGetPhysicalDeviceSurfaceSupportKHR(vk.physical_device, i, vk.surface, &supported);

        static const VkQueueFlags required = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT;
        if (supported && ((queue_props[i].queueFlags & required) == required))
        {
            vk.graphics_queue_family = i;
            break;
        }
    }

    for (uint32_t i = 0; i < queue_count; i++)
    {
        static const VkQueueFlags required = VK_QUEUE_COMPUTE_BIT;
        if (i != vk.graphics_queue_family && (queue_props[i].queueFlags & required) == required)
        {
            vk.compute_queue_family = i;
            break;
        }
    }

    for (uint32_t i = 0; i < queue_count; i++)
    {
        static const VkQueueFlags required = VK_QUEUE_TRANSFER_BIT;
        if (i != vk.graphics_queue_family
            && i != vk.compute_queue_family
            && (queue_props[i].queueFlags & required) == required)
        {
            vk.transfer_queue_family = i;
            break;
        }
    }

    /* Dedicated transfer queue. */
    if (vk.transfer_queue_family == VK_QUEUE_FAMILY_IGNORED)
    {
        for (uint32_t i = 0; i < queue_count; i++)
        {
            static const VkQueueFlags required = VK_QUEUE_TRANSFER_BIT;
            if (i != vk.graphics_queue_family && (queue_props[i].queueFlags & required) == required)
            {
                vk.transfer_queue_family = i;
                break;
            }
        }
    }

    if (vk.graphics_queue_family == VK_QUEUE_FAMILY_IGNORED) {
        vk_backend_shutdown();
        return false;
    }

    uint32_t universal_queue_index = 1;
    uint32_t compute_queue_index = 0;
    uint32_t transfer_queue_index = 0;

    if (vk.compute_queue_family == VK_QUEUE_FAMILY_IGNORED) {
        vk.compute_queue_family = vk.graphics_queue_family;
        compute_queue_index = _gpu_min(queue_props[vk.graphics_queue_family].queueCount - 1, universal_queue_index);
        universal_queue_index++;
    }

    if (vk.transfer_queue_family == VK_QUEUE_FAMILY_IGNORED) {
        vk.transfer_queue_family = vk.graphics_queue_family;
        transfer_queue_index = _gpu_min(queue_props[vk.graphics_queue_family].queueCount - 1, universal_queue_index);
        universal_queue_index++;
    }
    else if (vk.transfer_queue_family == vk.compute_queue_family) {
        transfer_queue_index = _gpu_min(queue_props[vk.compute_queue_family].queueCount - 1, 1u);
    }

    static const float graphics_queue_prio = 0.5f;
    static const float compute_queue_prio = 1.0f;
    static const float transfer_queue_prio = 1.0f;
    float prio[3] = { graphics_queue_prio, compute_queue_prio, transfer_queue_prio };

    unsigned queue_family_count = 0;
    VkDeviceQueueCreateInfo queue_info[3] = {};

    queue_info[queue_family_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info[queue_family_count].queueFamilyIndex = vk.graphics_queue_family;
    queue_info[queue_family_count].queueCount = _gpu_min(universal_queue_index, queue_props[vk.graphics_queue_family].queueCount);
    queue_info[queue_family_count].pQueuePriorities = prio;
    queue_family_count++;

    if (vk.compute_queue_family != vk.graphics_queue_family)
    {
        queue_info[queue_family_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[queue_family_count].queueFamilyIndex = vk.compute_queue_family;
        queue_info[queue_family_count].queueCount = _gpu_min(vk.transfer_queue_family == vk.compute_queue_family ? 2u : 1u,
            queue_props[vk.compute_queue_family].queueCount);
        queue_info[queue_family_count].pQueuePriorities = prio + 1;
        queue_family_count++;
    }

    if (vk.transfer_queue_family != vk.graphics_queue_family
        && vk.transfer_queue_family != vk.compute_queue_family)
    {
        queue_info[queue_family_count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[queue_family_count].queueFamilyIndex = vk.transfer_queue_family;
        queue_info[queue_family_count].queueCount = 1;
        queue_info[queue_family_count].pQueuePriorities = prio + 2;
        queue_family_count++;
    }

    VkPhysicalDeviceFeatures device_features;
    memset(&device_features, 0, sizeof(device_features));

    std::vector<const char*> device_exts;
    if (!vk.headless) {
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

    if (vkCreateDevice(vk.physical_device, &deviceCreateInfo, nullptr, &vk.device)) {
        vk_backend_shutdown();
        return false;
    }

    // Load device functions
    volkLoadDevice(vk.device);

    vkGetDeviceQueue(vk.device, vk.graphics_queue_family, 0, &vk.graphics_queue);
    vkGetDeviceQueue(vk.device, vk.compute_queue_family, compute_queue_index, &vk.compute_queue);
    vkGetDeviceQueue(vk.device, vk.transfer_queue_family, transfer_queue_index, &vk.transfer_queue);

    /* Create vma allocator */
    VmaVulkanFunctions vma_funcs = { 0 };
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

    VmaAllocatorCreateInfo vma_info;
    memset(&vma_info, 0, sizeof(vma_info));
    vma_info.flags = 0;
    vma_info.physicalDevice = vk.physical_device;
    vma_info.device = vk.device;
    vma_info.pVulkanFunctions = &vma_funcs;
    vma_info.instance = vk.instance;

    if (vmaCreateAllocator(&vma_info, &vk.memory_allocator) != VK_SUCCESS) {
        return false;
    }

    VkCommandPoolCreateInfo commandPoolInfo = {};
    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolInfo.queueFamilyIndex = vk.graphics_queue_family;

    if (vkCreateCommandPool(vk.device, &commandPoolInfo, NULL, &vk.command_pool)) {
        vk_backend_shutdown();
        return false;
    }

    /* Create main Swapchain if required. */
    if (!vk.headless) {
        if (!gpu_create_swapchain(vk.surface, config->swapchain, &vk.swapchains[0])) {
            return false;
        }
    }

    const VkCommandBufferAllocateInfo command_buffer_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        vk.command_pool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        1u
    };

    const VkFenceCreateInfo fence_create_info = {
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        nullptr,
        VK_FENCE_CREATE_SIGNALED_BIT
    };

    for (uint32_t i = 0; i < COUNTOF(vk.frames); i++) {
        if (vkAllocateCommandBuffers(vk.device, &command_buffer_info, &vk.frames[i].command_buffer)) {
            vk_backend_shutdown();
            return false;
        }

        if (vkCreateFence(vk.device, &fence_create_info, NULL, &vk.frames[i].fence)) {
            vk_backend_shutdown();
            return false;
        }
    }

    return true;
}

static void vk_backend_shutdown(void) {
    if (vk.device) vkDeviceWaitIdle(vk.device);

    /* Destroy main swap chain */
    if (!vk.headless) {
        gpu_destroy_swapchain(vk.swapchains[0]);
    }

    /* Release frame data. */
    for (uint32_t i = 0; i < COUNTOF(vk.frames); i++) {
        gpu_frame* frame = &vk.frames[i];

        vk_destroy_frame(frame);
        free(frame->destroy.data);

        if (frame->fence) vkDestroyFence(vk.device, frame->fence, NULL);
        if (frame->command_buffer) vkFreeCommandBuffers(vk.device, vk.command_pool, 1, &frame->command_buffer);
    }

    if (vk.command_pool) {
        vkDestroyCommandPool(vk.device, vk.command_pool, NULL);
    }

    if (vk.memory_allocator) {
        VmaStats stats;
        vmaCalculateStats(vk.memory_allocator, &stats);
        if (stats.total.usedBytes > 0) {
            GPU_THROW("GPU memory allocated is leaked");
        }
        //LOGI("Total device memory leaked: {} bytes.", stats.total.usedBytes);

        vmaDestroyAllocator(vk.memory_allocator);
    }

    if (vk.device) vkDestroyDevice(vk.device, NULL);

    if (vk.surface) vkDestroySurfaceKHR(vk.instance, vk.surface, NULL);
    if (vk.messenger) vkDestroyDebugUtilsMessengerEXT(vk.instance, vk.messenger, NULL);
    if (vk.instance) vkDestroyInstance(vk.instance, NULL);

#if defined(_WIN32)
    FreeLibrary(vk.library);
#else
    dlclose(vk_state.library);
#endif
    memset(&vk, 0, sizeof(vk));
}

static void vk_backend_wait_idle(void) {
    VK_CHECK(vkDeviceWaitIdle(vk.device));
}

static void vk_backend_begin_frame(void) {
    gpu_frame* frame = &vk.frames[vk.frame];

    // Wait for GPU to process the frame, then reset its scratchpad pool and purge condemned resources
    VK_CHECK(vkWaitForFences(vk.device, 1, &frame->fence, VK_FALSE, ~0ull));
    VK_CHECK(vkResetFences(vk.device, 1, &frame->fence));
    vk_destroy_frame(frame);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(frame->command_buffer, &begin_info));
}

static void vk_backend_end_frame(void) {
    gpu_frame* frame = &vk.frames[vk.frame];
    VK_CHECK(vkEndCommandBuffer(frame->command_buffer));

    // Submit pending graphics commands.
    const VkPipelineStageFlags color_attachment_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.pCommandBuffers = &frame->command_buffer;
    submit_info.commandBufferCount = 1;

    VK_CHECK(vkQueueSubmit(vk.graphics_queue, 1, &submit_info, frame->fence));

    vk.frame = (vk.frame + 1) % COUNTOF(vk.frames);
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
    gpu_swapchain swapchain = (gpu_swapchain)malloc(sizeof(gpu_swapchain_t));
    *result = swapchain;

    if (swapchain == NULL) {
        return false;
    }

    // Get physical device surface properties and formats
    VkSurfaceCapabilitiesKHR surface_caps;
    vk_result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.physical_device, surface, &surface_caps);
    if (vk_result != VK_SUCCESS) {
        return false;
    }

    uint32_t format_count;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physical_device, surface, &format_count, NULL) != VK_SUCCESS) {
        return false;
    }

    VkSurfaceFormatKHR* formats = _AGPU_ALLOCAN(VkSurfaceFormatKHR, format_count);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physical_device, surface, &format_count, formats) != VK_SUCCESS) {
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
    swapchain_size.width = _gpu_max(_gpu_min(desc->width, surface_caps.maxImageExtent.width), surface_caps.minImageExtent.width);
    swapchain_size.height = _gpu_max(_gpu_min(desc->height, surface_caps.maxImageExtent.height), surface_caps.minImageExtent.height);

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

    vk_result = vkCreateSwapchainKHR(vk.device, &swapchain_info, NULL, &swapchain->handle);
    if (vk_result != VK_SUCCESS) {
        gpu_destroy_swapchain(swapchain);
        return false;
    }

    if (old_swapchain) {
        vkDestroySwapchainKHR(vk.device, old_swapchain, NULL);
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
    if (vk.config.callback) {
        vk.config.callback(vk.config.context, data->pMessage, error);
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
    if (name && vk.config.flags & AGPU_CONFIG_FLAGS_VALIDATION) {
        VkDebugUtilsObjectNameInfoEXT info = {
            VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            nullptr,
            type,
            (uint64_t)handle,
            name
        };

        VK_CHECK(vkSetDebugUtilsObjectNameEXT(vk.device, &info));
    }
}

static void vk_queue_destroy(void* handle, void* handle2, VkObjectType type) {
    gpu_destroy_vector* destroy_list = &vk.frames[vk.frame].destroy;

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
        case VK_OBJECT_TYPE_BUFFER: vmaDestroyBuffer(vk.memory_allocator, (VkBuffer)ref->handle, (VmaAllocation)ref->handle2); break;
        case VK_OBJECT_TYPE_IMAGE: vmaDestroyImage(vk.memory_allocator, (VkImage)ref->handle, (VmaAllocation)ref->handle2); break;
        case VK_OBJECT_TYPE_IMAGE_VIEW: vkDestroyImageView(vk.device, (VkImageView)ref->handle, nullptr); break;
        case VK_OBJECT_TYPE_SAMPLER: vkDestroySampler(vk.device, (VkSampler)ref->handle, nullptr); break;
        case VK_OBJECT_TYPE_RENDER_PASS: vkDestroyRenderPass(vk.device, (VkRenderPass)ref->handle, nullptr); break;
        case VK_OBJECT_TYPE_FRAMEBUFFER: vkDestroyFramebuffer(vk.device, (VkFramebuffer)ref->handle, nullptr); break;
        case VK_OBJECT_TYPE_SWAPCHAIN_KHR: vkDestroySwapchainKHR(vk.device, (VkSwapchainKHR)ref->handle, nullptr); break;
        default: GPU_THROW("Unreachable"); break;
        }
    }
    frame->destroy.length = 0;
}

bool agpu_vk_supported(void) {
    static bool available_initialized = false;
    static bool available = false;
    if (available_initialized) {
        return available;
    }

    VkResult result = volkInitialize();
    if (result != VK_SUCCESS)
    {
        return false;
    }

    available = true;
    return available;
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

AgpuBool32 agpu_vk_supported(void) {
    return AGPU_FALSE;
}

agpu_renderer* agpu_create_vk_backend(void) {
    return nullptr;
}


#endif /* defined(GPU_VK_BACKEND) */
