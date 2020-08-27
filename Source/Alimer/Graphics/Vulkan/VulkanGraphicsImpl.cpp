//
// Copyright (c) 2020 Amer Koleci and contributors.
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

#include "config.h"
#include "VulkanGraphicsImpl.h"
#include "VulkanGPUTexture.h"
#include "VulkanGPUBuffer.h"
#include "Application/Application.h"
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

using namespace std;

namespace Alimer
{
    namespace
    {
#if defined(GPU_DEBUG) || defined(VULKAN_VALIDATION_LAYERS)
        VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsMessengerCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageTypes,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData)
        {
            // Log debug messge
            if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            {
                LOGW("{} - {}: {}", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
            }
            else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            {
                LOGE("{} - {}: {}", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
            }

            return VK_FALSE;
        }

        bool ValidateLayers(const vector<const char*>& required, const vector<VkLayerProperties>& available)
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
                    LOGE("Validation Layer '%s' not found", layer);
                    return false;
                }
            }

            return true;
        }

        vector<const char*> GetOptimalValidationLayers(const vector<VkLayerProperties>& supportedInstanceLayers)
        {
            vector<vector<const char*>> validation_layer_priority_list =
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
                {"VK_LAYER_LUNARG_core_validation"} };

            for (auto& validation_layers : validation_layer_priority_list)
            {
                if (ValidateLayers(validation_layers, supportedInstanceLayers))
                {
                    return validation_layers;
                }

                LOGW("Couldn't enable validation layers (see log for error) - falling back");
            }

            // Else return nothing
            return {};
        }
#endif

        QueueFamilyIndices QueryQueueFamilies(VkInstance instance, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
        {
            uint32_t queueCount = 0u;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);

            vector<VkQueueFamilyProperties> queue_families(queueCount);
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queue_families.data());

            QueueFamilyIndices result;
            result.graphicsQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            result.computeQueueFamily = VK_QUEUE_FAMILY_IGNORED;
            result.copyQueueFamily = VK_QUEUE_FAMILY_IGNORED;

            for (uint32_t i = 0; i < queueCount; i++)
            {
                VkBool32 presentSupport = VK_TRUE;
                if (surface != VK_NULL_HANDLE) {
                    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
                }

                static const VkQueueFlags required = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT;
                if (presentSupport && ((queue_families[i].queueFlags & required) == required))
                {
                    result.graphicsQueueFamilyIndex = i;
                    break;
                }
            }

            /* Dedicated compute queue. */
            for (uint32_t i = 0; i < queueCount; i++)
            {
                static const VkQueueFlags required = VK_QUEUE_COMPUTE_BIT;
                if (i != result.graphicsQueueFamilyIndex &&
                    (queue_families[i].queueFlags & required) == required)
                {
                    result.computeQueueFamily = i;
                    break;
                }
            }

            /* Dedicated transfer queue. */
            for (uint32_t i = 0; i < queueCount; i++)
            {
                static const VkQueueFlags required = VK_QUEUE_TRANSFER_BIT;
                if (i != result.graphicsQueueFamilyIndex &&
                    i != result.computeQueueFamily &&
                    (queue_families[i].queueFlags & required) == required)
                {
                    result.copyQueueFamily = i;
                    break;
                }
            }

            if (result.copyQueueFamily == VK_QUEUE_FAMILY_IGNORED)
            {
                for (uint32_t i = 0; i < queueCount; i++)
                {
                    static const VkQueueFlags required = VK_QUEUE_TRANSFER_BIT;
                    if (i != result.graphicsQueueFamilyIndex &&
                        (queue_families[i].queueFlags & required) == required)
                    {
                        result.copyQueueFamily = i;
                        break;
                    }
                }
            }

            return result;
        }

        PhysicalDeviceExtensions QueryPhysicalDeviceExtensions(const VulkanInstanceExtensions& instanceExts, VkPhysicalDevice physicalDevice)
        {
            uint32_t count = 0;
            VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, nullptr));

            vector<VkExtensionProperties> extensions(count);
            VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, extensions.data()));

            PhysicalDeviceExtensions result = {};
            for (uint32_t i = 0; i < count; ++i)
            {
                if (strcmp(extensions[i].extensionName, VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME) == 0) {
                    result.depth_clip_enable = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_KHR_maintenance1") == 0) {
                    result.maintenance_1 = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_KHR_maintenance2") == 0) {
                    result.maintenance_2 = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_KHR_maintenance3") == 0) {
                    result.maintenance_3 = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_KHR_get_memory_requirements2") == 0) {
                    result.get_memory_requirements2 = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_KHR_dedicated_allocation") == 0) {
                    result.dedicated_allocation = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_KHR_bind_memory2") == 0) {
                    result.bind_memory2 = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_EXT_memory_budget") == 0) {
                    result.memory_budget = true;
                }
                else if (strcmp(extensions[i].extensionName, VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME) == 0) {
                    result.image_format_list = true;
                }
                else if (strcmp(extensions[i].extensionName, VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME) == 0) {
                    result.sampler_mirror_clamp_to_edge = true;
                }
                else if (strcmp(extensions[i].extensionName, VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME) == 0) {
                    result.win32_full_screen_exclusive = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_KHR_ray_tracing") == 0) {
                    result.raytracing = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_KHR_buffer_device_address") == 0) {
                    result.buffer_device_address = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_KHR_deferred_host_operations") == 0) {
                    result.deferred_host_operations = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_EXT_descriptor_indexing") == 0) {
                    result.descriptor_indexing = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_KHR_pipeline_library") == 0) {
                    result.pipeline_library = true;
                }
                else if (strcmp(extensions[i].extensionName, "VK_KHR_multiview") == 0) {
                    result.multiview = true;
                }
            }

            // Return promoted to version 1.1
            VkPhysicalDeviceProperties2 gpuProps = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
            vkGetPhysicalDeviceProperties2(physicalDevice, &gpuProps);

            /* We run on vulkan 1.1 or higher. */
            if (gpuProps.properties.apiVersion >= VK_API_VERSION_1_1)
            {
                result.maintenance_1 = true;
                result.maintenance_2 = true;
                result.maintenance_3 = true;
                result.get_memory_requirements2 = true;
                result.bind_memory2 = true;
                result.multiview = true;
            }

            return result;
        }

        bool IsDeviceSuitable(VkInstance instance, const VulkanInstanceExtensions& instanceExts, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
        {
            QueueFamilyIndices indices = QueryQueueFamilies(instance, physicalDevice, surface);

            if (indices.graphicsQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED)
                return false;

            PhysicalDeviceExtensions features = QueryPhysicalDeviceExtensions(instanceExts, physicalDevice);

            /* We required maintenance_1 to support viewport flipping to match DX style. */
            if (!features.maintenance_1) {
                return false;
            }

            return true;
        }


        VkAttachmentLoadOp VulkanAttachmentLoadOp(LoadAction action)
        {
            switch (action) {
            case LoadAction::DontCare:
                return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            case LoadAction::Load:
                return VK_ATTACHMENT_LOAD_OP_LOAD;
            case LoadAction::Clear:
                return VK_ATTACHMENT_LOAD_OP_CLEAR;
            default:
                ALIMER_UNREACHABLE();
            }
        }
    }

    bool VulkanGraphicsImpl::IsAvailable()
    {
        static bool available_initialized = false;
        static bool available = false;
        if (available_initialized) {
            return available;
        }

        available_initialized = true;
        VkResult result = volkInitialize();
        if (result != VK_SUCCESS)
        {
            return false;
        }

        // We require Vulkan 1.1 at least
        uint32_t apiVersion = volkGetInstanceVersion();
        if (apiVersion <= VK_API_VERSION_1_1) {
            return false;
        }

        VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        appInfo.apiVersion = volkGetInstanceVersion();

        VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        createInfo.pApplicationInfo = &appInfo;

        VkInstance instance;
        result = vkCreateInstance(&createInfo, nullptr, &instance);
        if (result != VK_SUCCESS) {
            return false;
        }

        vkDestroyInstance = (PFN_vkDestroyInstance)vkGetInstanceProcAddr(instance, "vkDestroyInstance");
        vkDestroyInstance(instance, nullptr);
        available = true;
        return true;
    }

    VulkanGraphicsImpl::VulkanGraphicsImpl()
        : GraphicsImpl(GPUBackendType::Vulkan)
    {
        ALIMER_VERIFY(IsAvailable());
    }

    VulkanGraphicsImpl::~VulkanGraphicsImpl()
    {
        Shutdown();
    }

    void VulkanGraphicsImpl::Shutdown()
    {
        vkDeviceWaitIdle(device);

        for (auto semaphore : recycledSemaphores)
        {
            vkDestroySemaphore(device, semaphore, nullptr);
        }

        // Clear caches
        ClearRenderPassCache();
        ClearFramebufferCache();

        if (allocator != VK_NULL_HANDLE)
        {
            VmaStats stats;
            vmaCalculateStats(allocator, &stats);

            if (stats.total.usedBytes > 0) {
                LOGE("Total device memory leaked: {} bytes.", stats.total.usedBytes);
            }

            vmaDestroyAllocator(allocator);
        }

        vkDestroyDevice(device, nullptr);

#if defined(GPU_DEBUG) || defined(VULKAN_VALIDATION_LAYERS)
        if (debugUtilsMessenger != VK_NULL_HANDLE)
        {
            vkDestroyDebugUtilsMessengerEXT(instance, debugUtilsMessenger, nullptr);
        }
#endif

        if (instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(instance, nullptr);
        }
    }

    bool VulkanGraphicsImpl::Initialize(Window* window, const GraphicsDeviceSettings* settings)
    {
        // Create instance.
        {
            vector<const char*> enabledExtensions;
            vector<const char*> enabledLayers;

            uint32_t instanceExtensionCount;
            VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr));

            vector<VkExtensionProperties> availableInstanceExtensions(instanceExtensionCount);
            VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, availableInstanceExtensions.data()));
            for (auto& availableExtension : availableInstanceExtensions)
            {
                if (strcmp(availableExtension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
                {
                    instanceExts.debugUtils = true;
#if defined(GPU_DEBUG) || defined(VULKAN_VALIDATION_LAYERS)
                    enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
                }
                else if (strcmp(availableExtension.extensionName, VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME) == 0)
                {
                    instanceExts.headless = true;
                }
                else if (strcmp(availableExtension.extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0)
                {
                    // VK_KHR_get_physical_device_properties2 is a prerequisite of VK_KHR_performance_query
                    // which will be used for stats gathering where available.
                    instanceExts.get_physical_device_properties2 = true;
                    enabledExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
                }
                else if (strcmp(availableExtension.extensionName, VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME) == 0)
                {
                    instanceExts.get_surface_capabilities2 = true;
                }
            }


#if defined(GPU_DEBUG) || defined(VULKAN_VALIDATION_LAYERS)
            uint32_t instanceLayerCount;
            VK_CHECK(vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr));

            vector<VkLayerProperties> supportedInstanceLayers(instanceLayerCount);
            VK_CHECK(vkEnumerateInstanceLayerProperties(&instanceLayerCount, supportedInstanceLayers.data()));

            vector<const char*> optimalValidationLayers = GetOptimalValidationLayers(supportedInstanceLayers);
            enabledLayers.insert(enabledLayers.end(), optimalValidationLayers.begin(), optimalValidationLayers.end());
#endif

            const bool headless = false;
            if (headless)
            {
                enabledExtensions.push_back(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
            }
            else
            {
                enabledExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
                enabledExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
                enabledExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
                enabledExtensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
                enabledExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)
                enabledExtensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
                enabledExtensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#else
#	pragma error Platform not supported
#endif

                if (instanceExts.get_surface_capabilities2)
                {
                    enabledExtensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
                }
            }

            VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
            if (Application::GetCurrent() != nullptr)
            {
                auto name = Application::GetCurrent()->GetName();
                appInfo.pApplicationName = name.c_str();
            }

            appInfo.applicationVersion = 0;
            appInfo.pEngineName = "Alimer";
            appInfo.engineVersion = VK_MAKE_VERSION(ALIMER_VERSION_MAJOR, ALIMER_VERSION_MINOR, ALIMER_VERSION_PATCH);
            appInfo.apiVersion = volkGetInstanceVersion();

            VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };

#if defined(GPU_DEBUG) || defined(VULKAN_VALIDATION_LAYERS)
            VkDebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };

            if (instanceExts.debugUtils)
            {
                debugUtilsCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
                debugUtilsCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
                debugUtilsCreateInfo.pfnUserCallback = DebugUtilsMessengerCallback;
                debugUtilsCreateInfo.pUserData = this;

                createInfo.pNext = &debugUtilsCreateInfo;
            }
#endif

            createInfo.pApplicationInfo = &appInfo;
            createInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
            createInfo.ppEnabledLayerNames = enabledLayers.data();
            createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
            createInfo.ppEnabledExtensionNames = enabledExtensions.data();

            // Create the Vulkan instance.
            VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

            if (result != VK_SUCCESS)
            {
                VK_LOG_ERROR(result, "Could not create Vulkan instance");
            }

            volkLoadInstance(instance);

#if defined(GPU_DEBUG) || defined(VULKAN_VALIDATION_LAYERS)
            if (instanceExts.debugUtils)
            {
                result = vkCreateDebugUtilsMessengerEXT(instance, &debugUtilsCreateInfo, nullptr, &debugUtilsMessenger);
                if (result != VK_SUCCESS)
                {
                    VK_LOG_ERROR(result, "Could not create debug utils messenger");
                }
            }
#endif

            LOGI("Created VkInstance with version: {}.{}.{}", VK_VERSION_MAJOR(appInfo.apiVersion), VK_VERSION_MINOR(appInfo.apiVersion), VK_VERSION_PATCH(appInfo.apiVersion));
            if (createInfo.enabledLayerCount)
            {
                for (uint32_t i = 0; i < createInfo.enabledLayerCount; ++i)
                    LOGI("Instance layer '{}'", createInfo.ppEnabledLayerNames[i]);
            }

            for (uint32_t i = 0; i < createInfo.enabledExtensionCount; ++i)
            {
                LOGI("Instance extension '{}'", createInfo.ppEnabledExtensionNames[i]);
            }
        }

        VkSurfaceKHR surface = CreateSurface(window);

        if (!surface) {
            return false;
        }

        if (!InitPhysicalDevice(surface)) {
            return false;
        }

        if (!InitLogicalDevice()) {
            return false;
        }

        InitCapabilities();

        return true;
    }

    GPUAdapter* VulkanGraphicsImpl::GetAdapter() const
    {
        return adapter;
    }

    VulkanGPUContext* VulkanGraphicsImpl::GetVulkanMainContext() const
    {
        return mainContext;
    }

    void VulkanGraphicsImpl::InitCapabilities()
    {
        // Init features
        const VkPhysicalDeviceFeatures& vkFeatures = adapter->GetFeatures();

        _features.independentBlend = vkFeatures.independentBlend;
        _features.computeShader = true;
        _features.geometryShader = vkFeatures.geometryShader;
        _features.tessellationShader = vkFeatures.tessellationShader;
        _features.multiViewport = vkFeatures.multiViewport;
        _features.fullDrawIndexUint32 = vkFeatures.fullDrawIndexUint32;
        _features.multiDrawIndirect = vkFeatures.multiDrawIndirect;
        _features.fillModeNonSolid = vkFeatures.fillModeNonSolid;
        _features.samplerAnisotropy = vkFeatures.samplerAnisotropy;
        _features.textureCompressionETC2 = vkFeatures.textureCompressionETC2;
        _features.textureCompressionASTC_LDR = vkFeatures.textureCompressionASTC_LDR;
        _features.textureCompressionBC = vkFeatures.textureCompressionBC;
        _features.textureCubeArray = vkFeatures.imageCubeArray;
        //renderer->features.raytracing = vk.KHR_get_physical_device_properties2
        //    && renderer->device_features.get_memory_requirements2
        //    || HasExtension(VK_NV_RAY_TRACING_EXTENSION_NAME);

        // Limits
        const VkPhysicalDeviceProperties& gpuProps = adapter->GetProperties();

        _limits.maxVertexAttributes = gpuProps.limits.maxVertexInputAttributes;
        _limits.maxVertexBindings = gpuProps.limits.maxVertexInputBindings;
        _limits.maxVertexAttributeOffset = gpuProps.limits.maxVertexInputAttributeOffset;
        _limits.maxVertexBindingStride = gpuProps.limits.maxVertexInputBindingStride;

        //limits.maxTextureDimension1D = gpuProps.limits.maxImageDimension1D;
        _limits.maxTextureDimension2D = gpuProps.limits.maxImageDimension2D;
        _limits.maxTextureDimension3D = gpuProps.limits.maxImageDimension3D;
        _limits.maxTextureDimensionCube = gpuProps.limits.maxImageDimensionCube;
        _limits.maxTextureArrayLayers = gpuProps.limits.maxImageArrayLayers;
        _limits.maxColorAttachments = gpuProps.limits.maxColorAttachments;
        _limits.maxUniformBufferSize = gpuProps.limits.maxUniformBufferRange;
        _limits.minUniformBufferOffsetAlignment = (uint32_t)gpuProps.limits.minUniformBufferOffsetAlignment;
        _limits.maxStorageBufferSize = gpuProps.limits.maxStorageBufferRange;
        _limits.minStorageBufferOffsetAlignment = (uint32_t)gpuProps.limits.minStorageBufferOffsetAlignment;
        _limits.maxSamplerAnisotropy = (uint32_t)gpuProps.limits.maxSamplerAnisotropy;
        _limits.maxViewports = gpuProps.limits.maxViewports;
        _limits.maxViewportWidth = gpuProps.limits.maxViewportDimensions[0];
        _limits.maxViewportHeight = gpuProps.limits.maxViewportDimensions[1];
        _limits.maxTessellationPatchSize = gpuProps.limits.maxTessellationPatchSize;
        _limits.pointSizeRangeMin = gpuProps.limits.pointSizeRange[0];
        _limits.pointSizeRangeMax = gpuProps.limits.pointSizeRange[1];
        _limits.lineWidthRangeMin = gpuProps.limits.lineWidthRange[0];
        _limits.lineWidthRangeMax = gpuProps.limits.lineWidthRange[1];
        _limits.maxComputeSharedMemorySize = gpuProps.limits.maxComputeSharedMemorySize;
        _limits.maxComputeWorkGroupCountX = gpuProps.limits.maxComputeWorkGroupCount[0];
        _limits.maxComputeWorkGroupCountY = gpuProps.limits.maxComputeWorkGroupCount[1];
        _limits.maxComputeWorkGroupCountZ = gpuProps.limits.maxComputeWorkGroupCount[2];
        _limits.maxComputeWorkGroupInvocations = gpuProps.limits.maxComputeWorkGroupInvocations;
        _limits.maxComputeWorkGroupSizeX = gpuProps.limits.maxComputeWorkGroupSize[0];
        _limits.maxComputeWorkGroupSizeY = gpuProps.limits.maxComputeWorkGroupSize[1];
        _limits.maxComputeWorkGroupSizeZ = gpuProps.limits.maxComputeWorkGroupSize[2];
    }

    VkSurfaceKHR VulkanGraphicsImpl::CreateSurface(Window* window)
    {
        VkResult result = VK_SUCCESS;
        VkSurfaceKHR surface = VK_NULL_HANDLE;

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
        VkWin32SurfaceCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
        createInfo.hinstance = GetModuleHandle(NULL);
        createInfo.hwnd = (HWND)window->GetNativeHandle();
        result = vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
#elif defined(VK_USE_PLATFORM_XCB_KHR)
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
#else
#	pragma error Platform not supported
#endif
        if (result != VK_SUCCESS)
        {
            VK_LOG_ERROR(result, "Vulkan: Failed to create surface");
            return VK_NULL_HANDLE;
        }

        return surface;
    }

    bool VulkanGraphicsImpl::InitPhysicalDevice(VkSurfaceKHR surface)
    {
        // Enumerating and creating devices:
        uint32_t physicalDevicesCount = 0;
        VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, nullptr));

        if (physicalDevicesCount == 0) {
            LOGE("failed to find GPUs with Vulkan support!");
            assert(0);
        }

        vector<VkPhysicalDevice> physicalDevices(physicalDevicesCount);
        vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, physicalDevices.data());

        uint32_t bestDeviceScore = 0u;
        uint32_t bestDeviceIndex = VK_QUEUE_FAMILY_IGNORED;
        for (uint32_t i = 0; i < physicalDevicesCount; ++i)
        {
            if (!IsDeviceSuitable(instance, instanceExts, physicalDevices[i], surface)) {
                continue;
            }

            VkPhysicalDeviceProperties physical_device_props;
            vkGetPhysicalDeviceProperties(physicalDevices[i], &physical_device_props);

            uint32_t score = 0u;

            if (physical_device_props.apiVersion >= VK_API_VERSION_1_2) {
                score += 10000u;
            }

            switch (physical_device_props.deviceType)
            {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                score += 1000u;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                score += 100u;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                score += 80u;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                score += 70u;
                break;
            default:
                score += 10U;
                break;
            }

            if (score > bestDeviceScore) {
                bestDeviceIndex = i;
                bestDeviceScore = score;
            }
        }

        if (bestDeviceIndex == VK_QUEUE_FAMILY_IGNORED) {
            LOGE("Vulkan: Cannot find suitable physical device.");
            return false;
        }

        adapter = new VulkanGPUAdapter(physicalDevices[bestDeviceIndex]);
        queueFamilies = QueryQueueFamilies(instance, adapter->GetHandle(), surface);
        physicalDeviceExts = QueryPhysicalDeviceExtensions(instanceExts, adapter->GetHandle());
        return true;
    }

    bool VulkanGraphicsImpl::InitLogicalDevice()
    {
        VkResult result = VK_SUCCESS;

        /* Setup device queue's. */
        vector<VkQueueFamilyProperties> queue_families = adapter->GetQueueFamilyProperties();

        uint32_t universal_queue_index = 1;
        uint32_t compute_queue_index = 0;
        uint32_t copy_queue_index = 0;

        if (queueFamilies.computeQueueFamily == VK_QUEUE_FAMILY_IGNORED)
        {
            queueFamilies.computeQueueFamily = queueFamilies.graphicsQueueFamilyIndex;
            compute_queue_index = Min(queue_families[queueFamilies.graphicsQueueFamilyIndex].queueCount - 1, universal_queue_index);
            universal_queue_index++;
        }

        if (queueFamilies.copyQueueFamily == VK_QUEUE_FAMILY_IGNORED)
        {
            queueFamilies.copyQueueFamily = queueFamilies.graphicsQueueFamilyIndex;
            copy_queue_index = Min(queue_families[queueFamilies.graphicsQueueFamilyIndex].queueCount - 1, universal_queue_index);
            universal_queue_index++;
        }
        else if (queueFamilies.copyQueueFamily == queueFamilies.computeQueueFamily)
        {
            copy_queue_index = Min(queue_families[queueFamilies.computeQueueFamily].queueCount - 1, 1u);
        }

        static const float graphics_queue_prio = 0.5f;
        static const float compute_queue_prio = 1.0f;
        static const float transfer_queue_prio = 1.0f;
        float prio[3] = { graphics_queue_prio, compute_queue_prio, transfer_queue_prio };

        uint32_t queueCreateCount = 0;
        VkDeviceQueueCreateInfo queue_info[3] = { };
        queue_info[queueCreateCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[queueCreateCount].queueFamilyIndex = queueFamilies.graphicsQueueFamilyIndex;
        queue_info[queueCreateCount].queueCount = Min(universal_queue_index, queue_families[queueFamilies.graphicsQueueFamilyIndex].queueCount);
        queue_info[queueCreateCount].pQueuePriorities = prio;
        queueCreateCount++;

        if (queueFamilies.computeQueueFamily != queueFamilies.graphicsQueueFamilyIndex)
        {
            queue_info[queueCreateCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_info[queueCreateCount].queueFamilyIndex = queueFamilies.computeQueueFamily;
            queue_info[queueCreateCount].queueCount = Min(queueFamilies.copyQueueFamily == queueFamilies.computeQueueFamily ? 2u : 1u,
                queue_families[queueFamilies.computeQueueFamily].queueCount);
            queue_info[queueCreateCount].pQueuePriorities = prio + 1;
            queueCreateCount++;
        }

        // Dedicated copy queue
        if (queueFamilies.copyQueueFamily != queueFamilies.graphicsQueueFamilyIndex
            && queueFamilies.copyQueueFamily != queueFamilies.computeQueueFamily)
        {
            queue_info[queueCreateCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_info[queueCreateCount].queueFamilyIndex = queueFamilies.copyQueueFamily;
            queue_info[queueCreateCount].queueCount = 1;
            queue_info[queueCreateCount].pQueuePriorities = prio + 2;
            queueCreateCount++;
        }

        /* Setup device extensions now. */
        vector<const char*> enabledExtensions;
        const bool deviceApiVersion11 = adapter->GetProperties().apiVersion >= VK_API_VERSION_1_1;

        if (!headless) {
            enabledExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }

        if (physicalDeviceExts.get_memory_requirements2 && physicalDeviceExts.dedicated_allocation)
        {
            enabledExtensions.push_back("VK_KHR_get_memory_requirements2");
            enabledExtensions.push_back("VK_KHR_dedicated_allocation");
        }

        if (!deviceApiVersion11)
        {
            if (physicalDeviceExts.maintenance_1)
                enabledExtensions.push_back("VK_KHR_maintenance1");

            if (physicalDeviceExts.maintenance_2)
                enabledExtensions.push_back("VK_KHR_maintenance2");

            if (physicalDeviceExts.maintenance_3)
                enabledExtensions.push_back("VK_KHR_maintenance3");
        }

        if (physicalDeviceExts.image_format_list)
        {
            enabledExtensions.push_back(VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME);
        }

        if (physicalDeviceExts.sampler_mirror_clamp_to_edge)
        {
            enabledExtensions.push_back(VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME);
        }

        if (physicalDeviceExts.depth_clip_enable)
        {
            enabledExtensions.push_back(VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME);
        }

        /*if (vk.physical_device_features.buffer_device_address)
        {
            enabledExtensions[enabled_device_ext_count++] = VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME;
        }*/

#ifdef _WIN32
        if (instanceExts.get_surface_capabilities2 && physicalDeviceExts.win32_full_screen_exclusive)
        {
            enabledExtensions.push_back(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME);
        }
#endif

        VkPhysicalDeviceFeatures2 features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        VkPhysicalDeviceMultiviewFeatures multiview_features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES };
        void** ppNext = &features.pNext;

        if (physicalDeviceExts.multiview)
        {
            if (!deviceApiVersion11) {
                enabledExtensions.push_back("VK_KHR_multiview");
            }

            *ppNext = &multiview_features;
            ppNext = &multiview_features.pNext;
        }

        vkGetPhysicalDeviceFeatures2(adapter->GetHandle(), &features);

        // Enable device features we might care about.
        {
            VkPhysicalDeviceFeatures enabled_features = {};

            if (features.features.textureCompressionBC)
                enabled_features.textureCompressionBC = VK_TRUE;
            else if (features.features.textureCompressionASTC_LDR)
                enabled_features.textureCompressionASTC_LDR = VK_TRUE;
            else if (features.features.textureCompressionETC2)
                enabled_features.textureCompressionETC2 = VK_TRUE;

            if (features.features.fullDrawIndexUint32)
                enabled_features.fullDrawIndexUint32 = VK_TRUE;
            if (features.features.multiDrawIndirect)
                enabled_features.multiDrawIndirect = VK_TRUE;
            if (features.features.imageCubeArray)
                enabled_features.imageCubeArray = VK_TRUE;
            if (features.features.fillModeNonSolid)
                enabled_features.fillModeNonSolid = VK_TRUE;
            if (features.features.independentBlend)
                enabled_features.independentBlend = VK_TRUE;
            if (features.features.shaderSampledImageArrayDynamicIndexing)
                enabled_features.shaderSampledImageArrayDynamicIndexing = VK_TRUE;

            features.features = enabled_features;
        }

        VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        createInfo.pNext = &features;
        createInfo.queueCreateInfoCount = queueCreateCount;
        createInfo.pQueueCreateInfos = queue_info;
        createInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
        createInfo.ppEnabledExtensionNames = enabledExtensions.data();

        result = vkCreateDevice(adapter->GetHandle(), &createInfo, nullptr, &device);
        if (result != VK_SUCCESS)
        {
            VK_LOG_ERROR(result, "Failed to create device");
            return false;
        }

        vkGetDeviceQueue(device, queueFamilies.graphicsQueueFamilyIndex, 0, &graphicsQueue);
        vkGetDeviceQueue(device, queueFamilies.computeQueueFamily, compute_queue_index, &computeQueue);
        vkGetDeviceQueue(device, queueFamilies.copyQueueFamily, copy_queue_index, &copyQueue);

        LOGI("Created VkDevice using '{}' adapter with API version: {}.{}.{}",
            adapter->GetProperties().deviceName,
            VK_VERSION_MAJOR(adapter->GetProperties().apiVersion),
            VK_VERSION_MINOR(adapter->GetProperties().apiVersion),
            VK_VERSION_PATCH(adapter->GetProperties().apiVersion));
        for (uint32 i = 0; i < createInfo.enabledExtensionCount; ++i)
        {
            LOGI("Device extension '{}'", createInfo.ppEnabledExtensionNames[i]);
        }

        // Create vma allocator.
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = adapter->GetHandle();
        allocatorInfo.device = device;
        allocatorInfo.instance = instance;

        if (physicalDeviceExts.get_memory_requirements2 && physicalDeviceExts.dedicated_allocation)
        {
            allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
        }

        result = vmaCreateAllocator(&allocatorInfo, &allocator);
        if (result != VK_SUCCESS)
        {
            VK_LOG_ERROR(result, "Cannot create allocator");
            return false;
        }

        return true;
    }

    VkSemaphore VulkanGraphicsImpl::RequestSemaphore()
    {
        VkSemaphore semaphore;
        if (recycledSemaphores.empty())
        {
            VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
            VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore));
        }
        else
        {
            semaphore = recycledSemaphores.back();
            recycledSemaphores.pop_back();
        }

        return semaphore;
    }

    void VulkanGraphicsImpl::ReturnSemaphore(VkSemaphore semaphore)
    {
        recycledSemaphores.push_back(semaphore);
    }

    void VulkanGraphicsImpl::SetObjectName(VkObjectType type, uint64_t handle, const String& name)
    {
        if (!instanceExts.debugUtils)
            return;

        VkDebugUtilsObjectNameInfoEXT info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        info.objectType = type;
        info.objectHandle = handle;
        info.pObjectName = name.c_str();
        VK_CHECK(vkSetDebugUtilsObjectNameEXT(device, &info));
    }

    void VulkanGraphicsImpl::WaitForGPU()
    {
        VK_CHECK(vkDeviceWaitIdle(device));
    }

    bool VulkanGraphicsImpl::BeginFrame()
    {
        return true;
    }

    void VulkanGraphicsImpl::EndFrame()
    {

    }

    /* Resource creation methods */
    BufferHandle VulkanGraphicsImpl::AllocBufferHandle()
    {
        std::lock_guard<std::mutex> LockGuard(handle_mutex);

        if (buffers.isFull()) {
            LOGE("Vulkan: Not enough free buffer slots.");
            return kInvalidBuffer;
        }
        const int id = buffers.Alloc();
        ALIMER_ASSERT(id >= 0);

        VulkanBuffer& buffer = buffers[id];
        buffer.handle = nullptr;
        buffer.allocation = nullptr;
        return { (uint32_t)id };
    }

    VkRenderPass VulkanGraphicsImpl::GetRenderPass(uint32_t numColorAttachments, const RenderPassColorAttachment* colorAttachments, const RenderPassDepthStencilAttachment* depthStencil)
    {
        Hasher h;

        VkFormat formats[kMaxColorAttachments];

        for (uint32_t i = 0; i < numColorAttachments; i++)
        {
            //ALIMER_ASSERT(colorAttachments[i] != nullptr);
            formats[i] = VK_FORMAT_B8G8R8A8_UNORM;
        }

        h.data(formats, numColorAttachments * sizeof(VkFormat));
        h.u32(numColorAttachments);
        //h.u32(depth_stencil);

        auto hash = h.get();

        auto it = renderPasses.find(hash);
        if (it == renderPasses.end())
        {
            uint32_t attachmentCount = 0;
            VkAttachmentDescription attachments[kMaxColorAttachments + 1];
            VkAttachmentReference references[kMaxColorAttachments + 1];

            for (uint32_t i = 0; i < numColorAttachments; i++)
            {
                attachments[i] = {};
                attachments[i].format = VK_FORMAT_B8G8R8A8_UNORM; // texture.format;
                attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
                attachments[i].loadOp = VulkanAttachmentLoadOp(colorAttachments[i].loadAction);
                attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachments[i].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                attachments[i].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                references[i] = { i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
                attachmentCount++;
            }

            VkSubpassDescription subpass = {};
            subpass.flags = 0u;
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.inputAttachmentCount = 0u;
            subpass.pInputAttachments = nullptr;
            subpass.colorAttachmentCount = numColorAttachments;
            subpass.pColorAttachments = references;
            subpass.pResolveAttachments = nullptr;
            subpass.pDepthStencilAttachment = (depthStencil != nullptr) ? &references[attachmentCount - 1] : nullptr;

            VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
            createInfo.attachmentCount = attachmentCount;
            createInfo.pAttachments = attachments;
            createInfo.subpassCount = 1;
            createInfo.pSubpasses = &subpass;
            createInfo.dependencyCount = 0;
            createInfo.pDependencies = nullptr;

            VkRenderPass handle;
            VK_CHECK(vkCreateRenderPass(device, &createInfo, nullptr, &handle));
            renderPasses[hash] = handle;
            return handle;
        }

        return it->second;
    }

    VkFramebuffer VulkanGraphicsImpl::GetFramebuffer(VkRenderPass renderPass, uint32_t numColorAttachments, const RenderPassColorAttachment* colorAttachments, const RenderPassDepthStencilAttachment* depthStencil)
    {
        Hasher h;
        h.u64((uint64)renderPass);

        uint32 width = 0; // backbufferSize.x; // UINT32_MAX;
        uint32 height = 0; //  = backbufferSize.y; //UINT32_MAX;
        uint32 attachmentCount = 0;
        VkImageView attachments[kMaxColorAttachments + 1];

        for (uint32_t i = 0; i < numColorAttachments; i++)
        {
            //ALIMER_ASSERT(colorAttachments[i] != nullptr);
            //attachments[attachmentCount] = swapchainImageViews[backbufferIndex];
            h.u64((uint64_t)attachments[attachmentCount]);
            attachmentCount++;
        }

        auto hash = h.get();
        auto it = framebuffers.find(hash);
        if (it == framebuffers.end())
        {
            VkFramebufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
            createInfo.renderPass = renderPass;
            createInfo.attachmentCount = attachmentCount;
            createInfo.pAttachments = attachments;
            createInfo.width = width;
            createInfo.height = height;
            createInfo.layers = 1;

            VkFramebuffer handle;
            if (vkCreateFramebuffer(device, &createInfo, NULL, &handle) != VK_SUCCESS)
            {
                LOGE("Vulkan: Failed to create framebuffer.");
                return VK_NULL_HANDLE;
            }

            framebuffers[hash] = handle;
            return handle;
        }

        return it->second;
    }

    void VulkanGraphicsImpl::ClearRenderPassCache()
    {
        for (auto it : renderPasses) {
            vkDestroyRenderPass(device, it.second, nullptr);
        }
        renderPasses.clear();
    }

    void VulkanGraphicsImpl::ClearFramebufferCache()
    {
        for (auto it : framebuffers) {
            vkDestroyFramebuffer(device, it.second, nullptr);
        }
        framebuffers.clear();
    }
}
