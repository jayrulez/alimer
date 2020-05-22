#if defined(GPU_DRIVER_VULKAN)
#include "gpu_backend.h"
#include "vk/vk.h"
#include "vk/vk_mem_alloc.h"

#define GPU_CHECK(c, s) if (!(c)) { gpuLog(GPULogLevel_Error, s); GPU_ASSERT(false); }
#define VK_CHECK(f) do { VkResult r = (f); GPU_CHECK(r >= 0, gpukGetErrorString(r)); } while (0)

static const char* gpukGetErrorString(VkResult result) {
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

typedef struct VulkanRenderer {
    VkInstance instance;
    VkDebugUtilsMessengerEXT messenger;

    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceMemoryProperties memoryProperties;

    uint32_t queueFamilyIndex;
    VkDevice device;

    gpu_features features;
    gpu_limits limits;
} VulkanRenderer;

/* Driver functions */
static bool Vulkan_IsSupported(void) {
    static bool available_initialized;
    static bool available;

    if (available_initialized) {
        return available;
    }

    available_initialized = true;

    if (!vk_init_loader()) {
        return false;
    }

    VkInstanceCreateInfo instanceInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &(VkApplicationInfo) {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .apiVersion = VK_API_VERSION_1_1
        }
    };

    VkInstance instance;
    if (vkCreateInstance(&instanceInfo, NULL, &instance) != VK_SUCCESS) {
        return false;
    }

    vk_init_instance(instance);
    vkDestroyInstance(instance, NULL);
    available = true;
    return available;
};

static void Vulkan_GetDrawableSize(void* window, uint32_t* width, uint32_t* height)
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    RECT rect;
    GetClientRect((HWND)window, &rect);

    if (width) {
        *width = (uint32_t)(rect.right - rect.left);
    }

    if (height) {
        *height = (uint32_t)(rect.bottom - rect.top);
    }
#else
#endif
}

static VkBool32 Vulkan_DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT flags, const VkDebugUtilsMessengerCallbackDataEXT* data, void* context) {
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        gpuLog(GPULogLevel_Warn, data->pMessage);
    }
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        gpuLog(GPULogLevel_Error, data->pMessage);
    }

    return VK_FALSE;
}

static GPUDevice* Vulkan_CreateDevice(bool debug, const GPUSwapChainDescriptor* descriptor)
{
    GPUDevice* device;
    VulkanRenderer* renderer;

    if (!Vulkan_IsSupported()) {
        return NULL;
    }

    /* Allocate and zero out the renderer */
    renderer = (VulkanRenderer*)GPU_MALLOC(sizeof(VulkanRenderer));
    memset(renderer, 0, sizeof(VulkanRenderer));

    // Instance
    {
        uint32_t enabledLayersCount = 0;
        const char* enabled_layers[8];

        if (debug)
        {
            uint32_t layer_count;
            VK_CHECK(vkEnumerateInstanceLayerProperties(&layer_count, NULL));

            VkLayerProperties* queried_layers = GPU_STACK_ALLOC(VkLayerProperties, layer_count);
            VK_CHECK(vkEnumerateInstanceLayerProperties(&layer_count, queried_layers));

            bool foundValidationLayer = false;
            for (uint32_t i = 0; i < layer_count; i++)
            {
                if (strcmp(queried_layers[i].layerName, "VK_LAYER_KHRONOS_validation") == 0)
                {
                    enabled_layers[enabledLayersCount++] = "VK_LAYER_KHRONOS_validation";
                    foundValidationLayer = true;
                    break;
                }
            }

            if (!foundValidationLayer) {
                for (uint32_t i = 0; i < layer_count; i++)
                {
                    if (strcmp(queried_layers[i].layerName, "VK_LAYER_LUNARG_standard_validation") == 0)
                    {
                        enabled_layers[enabledLayersCount++] = "VK_LAYER_LUNARG_standard_validation";
                        foundValidationLayer = true;
                        break;
                    }
                }
            }

            if (!foundValidationLayer) {
                enabled_layers[enabledLayersCount++] = "VK_LAYER_LUNARG_object_tracker";
                enabled_layers[enabledLayersCount++] = "VK_LAYER_LUNARG_core_validation";
                enabled_layers[enabledLayersCount++] = "VK_LAYER_LUNARG_parameter_validation";
            }
        }

        const char* extensions[] = {
            "VK_KHR_surface",
#if defined(VK_USE_PLATFORM_WIN32_KHR)
            "VK_KHR_win32_surface",
#endif
            "VK_EXT_debug_utils"
        };

        /* We require version 1.1 or higher */
        uint32_t apiVersion = VK_API_VERSION_1_1;
        if (vkEnumerateInstanceVersion &&
            vkEnumerateInstanceVersion(&apiVersion) != VK_SUCCESS)
        {
            apiVersion = VK_API_VERSION_1_1;
        }

        VkInstanceCreateInfo instanceInfo = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &(VkApplicationInfo) {
                .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .apiVersion = apiVersion
            },
            .enabledLayerCount = enabledLayersCount,
            .ppEnabledLayerNames = enabled_layers,
            .enabledExtensionCount = debug ? GPU_COUNTOF(extensions) : 0,
            .ppEnabledExtensionNames = extensions
        };

        /*VkDebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
        if (config->debug && state.debugUtils)
        {
            debugUtilsCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
            debugUtilsCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
            debugUtilsCreateInfo.pfnUserCallback = DebugUtilsMessengerCallback;

            instanceInfo.pNext = &debugUtilsCreateInfo;
        }*/

        if (vkCreateInstance(&instanceInfo, NULL, &renderer->instance)) {
            return NULL;
        }

        vk_init_instance(renderer->instance);

        if (debug) {
            VkDebugUtilsMessengerCreateInfoEXT messengerInfo = {
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                .pfnUserCallback = Vulkan_DebugCallback,
                .pUserData = renderer
            };

            if (vkCreateDebugUtilsMessengerEXT(renderer->instance, &messengerInfo, NULL, &renderer->messenger)) {
                // gpu_shutdown();
                return false;
            }
        }

        gpuLog(GPULogLevel_Info, "Created VkInstance with version: %u.%u.%u",
            VK_VERSION_MAJOR(instanceInfo.pApplicationInfo->apiVersion),
            VK_VERSION_MINOR(instanceInfo.pApplicationInfo->apiVersion),
            VK_VERSION_PATCH(instanceInfo.pApplicationInfo->apiVersion));

        if (instanceInfo.enabledLayerCount > 0) {
            for (uint32_t i = 0; i < instanceInfo.enabledLayerCount; i++) {
                gpuLog(GPULogLevel_Info, "Instance layer '%s'", instanceInfo.ppEnabledLayerNames[i]);
            }
        }

        for (uint32_t i = 0; i < instanceInfo.enabledExtensionCount; i++) {
            gpuLog(GPULogLevel_Info, "Instance extension '%s'", instanceInfo.ppEnabledExtensionNames[i]);
        }
    }

    // Device
    {
        uint32_t deviceCount = 1;
        if (vkEnumeratePhysicalDevices(renderer->instance, &deviceCount, &renderer->physicalDevice) < 0) {
            //gpu_shutdown();
            return false;
        }

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(renderer->physicalDevice, &deviceProperties);

        renderer->queueFamilyIndex = ~0u;

        uint32_t queueFamilyCount = 0u;
        vkGetPhysicalDeviceQueueFamilyProperties(renderer->physicalDevice, &queueFamilyCount, NULL);
        VkQueueFamilyProperties* queueFamilies = GPU_STACK_ALLOC(VkQueueFamilyProperties, queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(renderer->physicalDevice, &queueFamilyCount, queueFamilies);

        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            uint32_t flags = queueFamilies[i].queueFlags;
            if ((flags & VK_QUEUE_GRAPHICS_BIT) && (flags & VK_QUEUE_COMPUTE_BIT)) {
                renderer->queueFamilyIndex = i;
                break;
            }
        }

        if (renderer->queueFamilyIndex == ~0u) {
            //gpu_shutdown();
            return false;
        }

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(renderer->physicalDevice, &features);
        //state.features.anisotropy = features.samplerAnisotropy;
        //state.features.astc = features.textureCompressionASTC_LDR;
        //state.features.dxt = features.textureCompressionBC;

        vkGetPhysicalDeviceMemoryProperties(renderer->physicalDevice, &renderer->memoryProperties);

        VkDeviceQueueCreateInfo queueInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = renderer->queueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = &(float) { 1.0f }
        };

        const char* extensions[] = {
            "VK_KHR_swapchain",
            "VK_KHR_maintenance1",
        };

        const VkDeviceCreateInfo deviceInfo = {
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
            .pQueueCreateInfos = &queueInfo,
            .enabledExtensionCount = GPU_COUNTOF(extensions),
            .ppEnabledExtensionNames = extensions
        };

        if (vkCreateDevice(renderer->physicalDevice, &deviceInfo, NULL, &renderer->device)) {
            //gpu_shutdown();
            return false;
        }

        vk_init_device(renderer->device);

        /*vkGetDeviceQueue(renderer->device, renderer->queueFamilyIndex, 0, &renderer->queue);

        VkCommandPoolCreateInfo commandPoolInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = state.queueFamilyIndex
        };

        if (vkCreateCommandPool(state.device, &commandPoolInfo, NULL, &state.commandPool)) {
            //gpu_shutdown();
            return false;
        }*/
    }

    // Frame data
    /*{
        state.frame = &state.frames[0];

        for (uint32_t i = 0; i < GPU_COUNTOF(state.frames); i++) {
            state.frames[i].index = i;

            const VkCommandBufferAllocateInfo commandBufferInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = state.commandPool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1
            };

            if (vkAllocateCommandBuffers(state.device, &commandBufferInfo, &state.frames[i].commandBuffer)) {
                //gpu_shutdown();
                return false;
            }

            const VkFenceCreateInfo fenceInfo = {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT
            };

            if (vkCreateFence(state.device, &fenceInfo, NULL, &state.frames[i].fence)) {
                //gpu_shutdown();
                return false;
            }
        }
    }*/

    /* Create and return the Device */
    device = (GPUDevice*)GPU_MALLOC(sizeof(GPUDevice));
    device->driverData = (GPU_Renderer*)renderer;
    //ASSIGN_DRIVER(Vulkan);
    return device;
}

GPUDriver VulkanDriver = {
    GPUBackendType_Vulkan,
    Vulkan_IsSupported,
    Vulkan_GetDrawableSize,
    Vulkan_CreateDevice
};

#endif /* defined(GPU_DRIVER_VULKAN) */
