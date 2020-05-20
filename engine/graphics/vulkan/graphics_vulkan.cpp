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

#include "graphics_vulkan.h"
#include "volk.h"
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "containers/array.h"
#include <map>
#include <algorithm>

namespace alimer
{
    namespace graphics
    {
        namespace vulkan
        {
            static const char* GetErrorString(VkResult result) {
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

            VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
                const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                void* user_data)
            {
                // Log debug messge
                if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
                {
                    LogWarn("%u - %s: %s", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
                }
                else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
                {
                    LogError("%u - %s: %s", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
                }
                return VK_FALSE;
            }

            struct PhysicalDeviceExtensions
            {
                bool swapchain;
                bool depthClipEnable;
                bool maintenance1;
                bool maintenance2;
                bool maintenance3;
                bool getMemoryRequirements2;
                bool dedicatedAllocation;
                bool bindMemory2;
                bool memoryBudget;
                bool imageFormatList;
                bool debugMarker;
                bool raytracing;
                bool buffer_device_address;
                bool deferred_host_operations;
                bool descriptor_indexing;
                bool pipeline_library;
                bool externalSemaphore;
                bool externalMemory;
                struct {
                    bool fullScreenExclusive;
                    bool externalSemaphore;
                    bool externalMemory;
                } win32;
                struct {
                    bool externalSemaphore;
                    bool externalMemory;
                } fd;
            };

            struct QueueFamilyIndices
            {
                uint32_t graphicsFamily = VK_QUEUE_FAMILY_IGNORED;
                uint32_t computeFamily = VK_QUEUE_FAMILY_IGNORED;
                uint32_t transferFamily = VK_QUEUE_FAMILY_IGNORED;
                uint32_t timestampValidBits = 0;

                bool IsComplete()
                {
                    return (graphicsFamily != VK_QUEUE_FAMILY_IGNORED);
                }
            };

#define GPU_VK_THROW(str) LogError("%s", str);
#define GPU_VK_CHECK(c, str) if (!(c)) { GPU_VK_THROW(str); }
#define VK_CHECK(res) do { VkResult r = (res); GPU_VK_CHECK(r >= 0, GetErrorString(r)); } while (0)

            struct Frame
            {
                uint32_t index;
                VkCommandPool commandPool;
                VkFence fence;
                VkSemaphore imageAcquiredSemaphore;
                VkSemaphore renderCompleteSemaphore;
                VkCommandBuffer commandBuffer;
            };

            struct Context
            {
                enum { MAX_COUNT = 16 };

                VkSurfaceKHR surface;
                VkSurfaceFormatKHR surfaceFormat;
                VkSwapchainKHR handle;

                VkExtent2D surfaceExtent;
                uint32_t frameIndex;
                uint32_t maxInflightFrames;
                uint32_t imageIndex;
                uint32_t imageCount;
                Array<TextureHandle> backbuffers;
                Frame* frames;
            };

            struct Texture
            {
                enum { MAX_COUNT = 4096 };

                VkFormat format;
                VkImage handle;
                TextureState state;
                VkImageView view;
            };

            struct Buffer
            {
                enum { MAX_COUNT = 4096 };
            };

            static struct {
                bool availableInitialized = false;
                bool available = false;

                /// VK_KHR_get_physical_device_properties2
                bool physicalDeviceProperties2 = false;
                /// VK_KHR_external_memory_capabilities
                bool externalMemoryCapabilities = false;
                /// VK_KHR_external_semaphore_capabilities
                bool externalSemaphoreCapabilities = false;
                /// VK_EXT_debug_utils
                bool debugUtils = false;
                /// VK_EXT_headless_surface
                bool headless = false;
                /// VK_KHR_surface
                bool surface = false;
                /// VK_KHR_get_surface_capabilities2
                bool surfaceCapabilities2 = false;

                VkInstance instance;
                VkDebugUtilsMessengerEXT debugUtilsMessenger = VK_NULL_HANDLE;

                VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
                VkPhysicalDeviceProperties physicalDeviceProperties;
                PhysicalDeviceExtensions physicalDeviceExtensions;
                QueueFamilyIndices queueFamilyIndices;
                bool supportsExternal;

                VkDevice device = VK_NULL_HANDLE;
                VolkDeviceTable deviceTable;
                VkQueue graphicsQueue = VK_NULL_HANDLE;
                VkQueue computeQueue = VK_NULL_HANDLE;
                VkQueue copyQueue = VK_NULL_HANDLE;

                VmaAllocator memoryAllocator = VK_NULL_HANDLE;

                Pool<Context, Context::MAX_COUNT> contexts;
                Pool<Texture, Texture::MAX_COUNT> textures;
                Pool<Buffer, Buffer::MAX_COUNT> buffers;
            } state;

            static VkFormat GetVkFormat(PixelFormat format)
            {
                switch (format)
                {
                case PixelFormat::R8UNorm:          return VK_FORMAT_R8_UNORM;
                case PixelFormat::R8SNorm:          return VK_FORMAT_R8_SNORM;
                case PixelFormat::R8UInt:           return VK_FORMAT_R8_UINT;
                case PixelFormat::R8SInt:           return VK_FORMAT_R8_SINT;
                case PixelFormat::R16UNorm:         return VK_FORMAT_R16_UNORM;
                case PixelFormat::R16SNorm:         return VK_FORMAT_R16_SNORM;
                case PixelFormat::R16UInt:          return VK_FORMAT_R16_UINT;
                case PixelFormat::R16SInt:          return VK_FORMAT_R16_SINT;
                case PixelFormat::R16Float:         return VK_FORMAT_R16_SFLOAT;
                case PixelFormat::RG8UNorm:         return VK_FORMAT_R8G8_UNORM;
                case PixelFormat::RG8SNorm:         return VK_FORMAT_R8G8_SNORM;
                case PixelFormat::RG8UInt:          return VK_FORMAT_R8G8_UINT;
                case PixelFormat::RG8SInt:          return VK_FORMAT_R8G8_SINT;
                case PixelFormat::R32UInt:          return VK_FORMAT_R32_UINT;
                case PixelFormat::R32SInt:          return VK_FORMAT_R32_SINT;
                case PixelFormat::R32Float:         return VK_FORMAT_R32_SFLOAT;
                case PixelFormat::RG16UNorm:        return VK_FORMAT_R16G16_UNORM;
                case PixelFormat::RG16SNorm:        return VK_FORMAT_R16G16_SNORM;
                case PixelFormat::RG16UInt:         return VK_FORMAT_R16G16_UINT;
                case PixelFormat::RG16SInt:         return VK_FORMAT_R16G16_SINT;
                case PixelFormat::RG16Float:        return VK_FORMAT_R16G16_SFLOAT;
                case PixelFormat::RGBA8UNorm:
                    return VK_FORMAT_R8G8B8A8_UNORM;
                case PixelFormat::RGBA8UNormSrgb:
                    return VK_FORMAT_R8G8B8A8_SRGB;
                case PixelFormat::RGBA8SNorm:
                    return VK_FORMAT_R8G8B8A8_SNORM;
                case PixelFormat::RGBA8UInt:
                    return VK_FORMAT_R8G8B8A8_UINT;
                case PixelFormat::RGBA8SInt:
                    return VK_FORMAT_R8G8B8A8_SINT;
                case PixelFormat::BGRA8UNorm:
                    return VK_FORMAT_B8G8R8A8_UNORM;
                case PixelFormat::BGRA8UNormSrgb:
                    return VK_FORMAT_B8G8R8A8_SRGB;
                case PixelFormat::RGB10A2UNorm:
                    return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
                case PixelFormat::RG11B10Float:     return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
                case PixelFormat::RG32UInt:         return VK_FORMAT_R32G32_UINT;
                case PixelFormat::RG32SInt:         return VK_FORMAT_R32G32_SINT;
                case PixelFormat::RG32Float:        return VK_FORMAT_R32G32_SFLOAT;
                case PixelFormat::RGBA16UNorm:      return VK_FORMAT_R16G16B16A16_UNORM;
                case PixelFormat::RGBA16SNorm:      return VK_FORMAT_R16G16B16A16_SNORM;
                case PixelFormat::RGBA16UInt:       return VK_FORMAT_R16G16B16A16_UINT;
                case PixelFormat::RGBA16SInt:       return VK_FORMAT_R16G16B16A16_SINT;
                case PixelFormat::RGBA16Float:      return VK_FORMAT_R16G16B16A16_SFLOAT;
                case PixelFormat::RGBA32UInt:       return VK_FORMAT_R32G32B32A32_UINT;
                case PixelFormat::RGBA32SInt:       return VK_FORMAT_R32G32B32A32_SINT;
                case PixelFormat::RGBA32Float:      return VK_FORMAT_R32G32B32A32_SFLOAT;

                    // Depth-stencil formats
                case PixelFormat::Depth32Float:
                    return VK_FORMAT_D32_SFLOAT;
                case PixelFormat::Depth16UNorm:
                    return VK_FORMAT_D16_UNORM;
                case PixelFormat::Depth24Plus:
                    return VK_FORMAT_D24_UNORM_S8_UINT;
                case PixelFormat::Depth24PlusStencil8:
                    return VK_FORMAT_D32_SFLOAT_S8_UINT;

                    // Compressed BC formats
                case PixelFormat::BC1RGBAUNorm:
                    return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
                case PixelFormat::BC1RGBAUNormSrgb:
                    return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
                case PixelFormat::BC2RGBAUNorm:
                    return VK_FORMAT_BC2_UNORM_BLOCK;
                case PixelFormat::BC2RGBAUNormSrgb:
                    return VK_FORMAT_BC2_SRGB_BLOCK;
                case PixelFormat::BC3RGBAUNorm:
                    return VK_FORMAT_BC3_UNORM_BLOCK;
                case PixelFormat::BC3RGBAUNormSrgb:
                    return VK_FORMAT_BC3_SRGB_BLOCK;
                case PixelFormat::BC4RUNorm:
                    return VK_FORMAT_BC4_UNORM_BLOCK;
                case PixelFormat::BC4RSNorm:
                    return VK_FORMAT_BC4_SNORM_BLOCK;
                case PixelFormat::BC5RGUNorm:
                    return VK_FORMAT_BC5_UNORM_BLOCK;
                case PixelFormat::BC5RGSNorm:
                    return VK_FORMAT_BC5_SNORM_BLOCK;
                case PixelFormat::BC6HRGBUFloat:
                    return VK_FORMAT_BC6H_UFLOAT_BLOCK;
                case PixelFormat::BC6HRGBSFloat:
                    return VK_FORMAT_BC6H_SFLOAT_BLOCK;
                case PixelFormat::BC7RGBAUNorm:
                    return VK_FORMAT_BC7_UNORM_BLOCK;
                case PixelFormat::BC7RGBAUNormSrgb:
                    return VK_FORMAT_BC7_SRGB_BLOCK;
                default:
                    return VK_FORMAT_UNDEFINED;
                }

                return VK_FORMAT_UNDEFINED;
            }

            static bool VulkanIsSupported(void)
            {
                if (state.availableInitialized) {
                    return state.available;
                }

                state.availableInitialized = true;
                VkResult result = volkInitialize();
                if (result != VK_SUCCESS)
                {
                    return false;
                }

                // Create the Vulkan instance
                VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
                appInfo.pApplicationName = "Alimer";
                appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
                appInfo.pEngineName = "Alimer";
                appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
                appInfo.apiVersion = VK_API_VERSION_1_1;

                VkInstanceCreateInfo instanceInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
                instanceInfo.pApplicationInfo = &appInfo;

                VkInstance instance;
                result = vkCreateInstance(&instanceInfo, nullptr, &instance);
                if (result != VK_SUCCESS)
                {
                    return false;
                }

                volkLoadInstance(instance);
                vkDestroyInstance(instance, nullptr);
                state.available = true;
                return state.available;
            }

            static PhysicalDeviceExtensions CheckDeviceExtensionSupport(VkPhysicalDevice device)
            {
                uint32_t extensionCount;
                vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

                VkExtensionProperties* availableExtensions = StackAlloc<VkExtensionProperties>(extensionCount);
                vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions);

                PhysicalDeviceExtensions exts = {};
                for (uint32_t i = 0; i < extensionCount; i++) {
                    VkExtensionProperties extension = availableExtensions[i];
                    if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
                        exts.swapchain = true;
                    }
                    else if (strcmp(extension.extensionName, VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME) == 0) {
                        exts.depthClipEnable = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_MAINTENANCE1_EXTENSION_NAME) == 0) {
                        exts.maintenance1 = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_MAINTENANCE2_EXTENSION_NAME) == 0) {
                        exts.maintenance2 = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_MAINTENANCE3_EXTENSION_NAME) == 0) {
                        exts.maintenance3 = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME) == 0) {
                        exts.getMemoryRequirements2 = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME) == 0) {
                        exts.dedicatedAllocation = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_BIND_MEMORY_2_EXTENSION_NAME) == 0) {
                        exts.bindMemory2 = true;
                    }
                    else if (strcmp(extension.extensionName, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME) == 0) {
                        exts.memoryBudget = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME) == 0) {
                        exts.imageFormatList = true;
                    }
                    else if (strcmp(extension.extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0) {
                        exts.debugMarker = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_RAY_TRACING_EXTENSION_NAME) == 0) {
                        exts.raytracing = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) == 0) {
                        exts.buffer_device_address = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME) == 0) {
                        exts.deferred_host_operations = true;
                    }
                    else if (strcmp(extension.extensionName, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME) == 0) {
                        exts.descriptor_indexing = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME) == 0) {
                        exts.pipeline_library = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME) == 0) {
                        exts.externalSemaphore = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME) == 0) {
                        exts.externalMemory = true;
                    }
                    else if (strcmp(extension.extensionName, "VK_EXT_full_screen_exclusive") == 0) {
                        exts.win32.fullScreenExclusive = true;
                    }
                    else if (strcmp(extension.extensionName, "VK_KHR_external_semaphore_win32") == 0) {
                        exts.win32.externalSemaphore = true;
                    }
                    else if (strcmp(extension.extensionName, "VK_KHR_external_memory_win32") == 0) {
                        exts.win32.externalMemory = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME) == 0) {
                        exts.fd.externalSemaphore = true;
                    }
                    else if (strcmp(extension.extensionName, VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME) == 0) {
                        exts.fd.externalMemory = true;
                    }
                }

                return exts;
            }

            static bool GetPhysicalDevicePresentationSupport(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex) {
#if defined(_WIN32) || defined(_WIN64)
                return vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, queueFamilyIndex);
#elif defined(__ANDROID__)
                return true; // All Android queues surfaces support present.
#else
                // TODO:
                return true;
#endif
            }

            static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
            {
                uint32_t queueFamilyCount = 0;
                vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

                VkQueueFamilyProperties* queueFamilyProperties = StackAlloc<VkQueueFamilyProperties>(queueFamilyCount);
                vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties);

                QueueFamilyIndices indices = {};
                for (uint32_t i = 0; i < queueFamilyCount; i++)
                {
                    VkBool32 presentSupported = surface == VK_NULL_HANDLE;
                    if (surface != VK_NULL_HANDLE)
                    {
                        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupported);
                    }
                    else
                    {
                        presentSupported = GetPhysicalDevicePresentationSupport(physicalDevice, i);
                    }

                    static const VkQueueFlags required = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT;
                    if (presentSupported && ((queueFamilyProperties[i].queueFlags & required) == required))
                    {
                        indices.graphicsFamily = i;

                        // This assumes timestamp valid bits is the same for all queue types.
                        indices.timestampValidBits = queueFamilyProperties[i].timestampValidBits;
                        break;
                    }
                }

                for (uint32_t i = 0; i < queueFamilyCount; i++)
                {
                    static const VkQueueFlags required = VK_QUEUE_COMPUTE_BIT;
                    if (i != indices.graphicsFamily
                        && (queueFamilyProperties[i].queueFlags & required) == required)
                    {
                        indices.computeFamily = i;
                        break;
                    }
                }

                for (uint32_t i = 0; i < queueFamilyCount; i++)
                {
                    static const VkQueueFlags required = VK_QUEUE_TRANSFER_BIT;
                    if (i != indices.graphicsFamily
                        && i != indices.computeFamily
                        && (queueFamilyProperties[i].queueFlags & required) == required)
                    {
                        indices.transferFamily = i;
                        break;
                    }
                }

                /* Find dedicated transfer family. */
                if (indices.transferFamily == VK_QUEUE_FAMILY_IGNORED)
                {
                    for (uint32_t i = 0; i < queueFamilyCount; i++)
                    {
                        static const VkQueueFlags required = VK_QUEUE_TRANSFER_BIT;
                        if (i != indices.graphicsFamily && (queueFamilyProperties[i].queueFlags & required) == required)
                        {
                            indices.transferFamily = i;
                            break;
                        }
                    }
                }


                return indices;
            }

            static int32_t RatePhysicalDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
            {
                VkPhysicalDeviceProperties deviceProperties;
                vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

                VkPhysicalDeviceFeatures deviceFeatures;
                vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

                PhysicalDeviceExtensions exts = CheckDeviceExtensionSupport(physicalDevice);
                if (!exts.swapchain || !exts.maintenance1)
                {
                    return 0;
                }

                QueueFamilyIndices indices = FindQueueFamilies(physicalDevice, surface);

                if (!indices.IsComplete())
                {
                    return 0;
                }

                int32_t score = 0;
                if (deviceProperties.apiVersion >= VK_API_VERSION_1_2) {
                    score += 10000u;
                }
                else if (deviceProperties.apiVersion >= VK_API_VERSION_1_1) {
                    score += 5000u;
                }

                switch (deviceProperties.deviceType) {
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
                default: score += 10u;
                }

                return score;
            }

            static bool VulkanInit(const Configuration& config)
            {
                // Enumerate global supported extensions.
                uint32_t instanceExtensionCount;
                VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr));

                VkExtensionProperties* availableInstanceExtensions = StackAlloc<VkExtensionProperties>(instanceExtensionCount);
                VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, availableInstanceExtensions));

                for (uint32_t i = 0; i < instanceExtensionCount; ++i)
                {
                    if (strcmp(availableInstanceExtensions[i].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
                        state.debugUtils = true;
                    }
                    else if (strcmp(availableInstanceExtensions[i].extensionName, VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME) == 0)
                    {
                        state.headless = true;
                    }
                    else if (strcmp(availableInstanceExtensions[i].extensionName, VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME) == 0)
                    {
                        state.surfaceCapabilities2 = true;
                    }
                    else if (strcmp(availableInstanceExtensions[i].extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0)
                    {
                        state.physicalDeviceProperties2 = true;
                    }
                    else if (strcmp(availableInstanceExtensions[i].extensionName, VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME) == 0)
                    {
                        state.externalMemoryCapabilities = true;
                    }
                    else if (strcmp(availableInstanceExtensions[i].extensionName, VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME) == 0)
                    {
                        state.externalSemaphoreCapabilities = true;
                    }
                }

                Array<const char*> enabledInstanceExtensions;

                if (state.physicalDeviceProperties2)
                {
                    enabledInstanceExtensions.Push(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
                }

                if (state.physicalDeviceProperties2 &&
                    state.externalMemoryCapabilities &&
                    state.externalSemaphoreCapabilities)
                {
                    enabledInstanceExtensions.Push(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
                    enabledInstanceExtensions.Push(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
                }

                if (config.debug && state.debugUtils)
                {
                    enabledInstanceExtensions.Push(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                }

                // Try to enable headless surface extension if it exists.
                const bool headless = false;
                if (headless)
                {
                    if (!state.headless)
                    {
                        LogWarn("%s is not available, disabling swapchain creation", VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
                    }
                    else
                    {
                        LogInfo("%s is available, enabling it", VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
                        enabledInstanceExtensions.Push(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
                    }
                }
                else
                {
                    enabledInstanceExtensions.Push(VK_KHR_SURFACE_EXTENSION_NAME);
                    // Enable surface extensions depending on os
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
                    enabledInstanceExtensions.Push(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
                    enabledInstanceExtensions.Push(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
                    enabledInstanceExtensions.Push(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
                    enabledInstanceExtensions.Push(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_IOS_MVK)
                    enabledInstanceExtensions.Push(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
                // TODO: Support VK_EXT_metal_surface
                    enabledInstanceExtensions.Push(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#endif

                    if (state.surfaceCapabilities2)
                    {
                        enabledInstanceExtensions.Push(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
                    }
                }

                uint32_t enabledLayersCount = 0;
                const char* enabledLayers[8] = {};

                if (config.debug)
                {
                    uint32_t layerCount;
                    VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

                    VkLayerProperties queriedLayers[32] = {};
                    VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, queriedLayers));

                    bool foundValidationLayer = false;
                    for (uint32_t i = 0; i < layerCount; i++)
                    {
                        if (strcmp(queriedLayers[i].layerName, "VK_LAYER_KHRONOS_validation") == 0)
                        {
                            enabledLayers[enabledLayersCount++] = "VK_LAYER_KHRONOS_validation";
                            foundValidationLayer = true;
                            break;
                        }
                    }

                    if (!foundValidationLayer) {
                        for (uint32_t i = 0; i < layerCount; i++)
                        {
                            if (strcmp(queriedLayers[i].layerName, "VK_LAYER_LUNARG_standard_validation") == 0)
                            {
                                enabledLayers[enabledLayersCount++] = "VK_LAYER_LUNARG_standard_validation";
                                foundValidationLayer = true;
                                break;
                            }
                        }
                    }
                }

                // Create the Vulkan instance
                VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
                appInfo.pApplicationName = "Alimer";
                appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
                appInfo.pEngineName = "Alimer";
                appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
                appInfo.apiVersion = VK_API_VERSION_1_1;

                VkInstanceCreateInfo instanceInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
                instanceInfo.pApplicationInfo = &appInfo;
                instanceInfo.enabledLayerCount = enabledLayersCount;
                instanceInfo.ppEnabledLayerNames = enabledLayers;
                instanceInfo.enabledExtensionCount = enabledInstanceExtensions.Size();
                instanceInfo.ppEnabledExtensionNames = enabledInstanceExtensions.Data();

                VkDebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
                if (config.debug && state.debugUtils)
                {
                    debugUtilsCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
                    debugUtilsCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
                    debugUtilsCreateInfo.pfnUserCallback = DebugUtilsMessengerCallback;

                    instanceInfo.pNext = &debugUtilsCreateInfo;
                }

                VkResult result = vkCreateInstance(&instanceInfo, nullptr, &state.instance);
                if (result != VK_SUCCESS)
                {
                    return false;
                }

                volkLoadInstance(state.instance);

                LogInfo("Created VkInstance with version: %u.%u.%u", VK_VERSION_MAJOR(appInfo.apiVersion), VK_VERSION_MINOR(appInfo.apiVersion), VK_VERSION_PATCH(appInfo.apiVersion));
                if (enabledLayersCount > 0) {
                    for (uint32_t i = 0; i < enabledLayersCount; i++)
                    {
                        LogInfo("Instance layer '%s'", enabledLayers[i]);
                    }
                }

                for (auto* ext_name : enabledInstanceExtensions)
                {
                    LogInfo("Instance extension '%s'", ext_name);
                }

                if (config.debug && state.debugUtils)
                {
                    result = vkCreateDebugUtilsMessengerEXT(state.instance, &debugUtilsCreateInfo, nullptr, &state.debugUtilsMessenger);
                    if (result != VK_SUCCESS)
                    {
                        LogError("Could not create debug utils messenger");
                    }
                }

                uint32_t deviceCount;
                if (vkEnumeratePhysicalDevices(state.instance, &deviceCount, nullptr) != VK_SUCCESS ||
                    deviceCount == 0)
                {
                    return false;
                }

                VkPhysicalDevice* physicalDevices = StackAlloc<VkPhysicalDevice>(deviceCount);
                if (vkEnumeratePhysicalDevices(state.instance, &deviceCount, physicalDevices) != VK_SUCCESS) {
                    return false;
                }

                std::multimap<int32_t, VkPhysicalDevice> physicalDeviceCandidates;
                for (uint32_t i = 0; i < deviceCount; i++)
                {
                    int32_t score = RatePhysicalDevice(physicalDevices[i], VK_NULL_HANDLE);
                    physicalDeviceCandidates.insert(std::make_pair(score, physicalDevices[i]));
                }

                // Check if the best candidate is suitable at all
                if (physicalDeviceCandidates.rbegin()->first <= 0)
                {
                    LogError("[Vulkan]: Failed to find a suitable GPU!");
                    return false;
                }

                state.physicalDevice = physicalDeviceCandidates.rbegin()->second;
                vkGetPhysicalDeviceProperties(state.physicalDevice, &state.physicalDeviceProperties);
                // Store the properties of each queuefamily
                uint32_t queueFamilyCount = 0;
                vkGetPhysicalDeviceQueueFamilyProperties(state.physicalDevice, &queueFamilyCount, nullptr);

                VkQueueFamilyProperties* queueFamilyProperties = StackAlloc<VkQueueFamilyProperties>(queueFamilyCount);
                vkGetPhysicalDeviceQueueFamilyProperties(state.physicalDevice, &queueFamilyCount, queueFamilyProperties);

                state.physicalDeviceExtensions = CheckDeviceExtensionSupport(state.physicalDevice);
                state.queueFamilyIndices = FindQueueFamilies(state.physicalDevice, VK_NULL_HANDLE);

                // Setup queues first.
                uint32_t universal_queue_index = 1;
                const uint32_t graphicsQueueIndex = 0;
                uint32_t computeQueueIndex = 0;
                uint32_t copyQueueIndex = 0;

                if (state.queueFamilyIndices.computeFamily == VK_QUEUE_FAMILY_IGNORED)
                {
                    state.queueFamilyIndices.computeFamily = state.queueFamilyIndices.graphicsFamily;
                    computeQueueIndex = std::min(queueFamilyProperties[state.queueFamilyIndices.graphicsFamily].queueCount - 1, universal_queue_index);
                    universal_queue_index++;
                }

                if (state.queueFamilyIndices.transferFamily == VK_QUEUE_FAMILY_IGNORED)
                {
                    state.queueFamilyIndices.transferFamily = state.queueFamilyIndices.graphicsFamily;
                    copyQueueIndex = std::min(queueFamilyProperties[state.queueFamilyIndices.graphicsFamily].queueCount - 1, universal_queue_index);
                    universal_queue_index++;
                }
                else if (state.queueFamilyIndices.transferFamily == state.queueFamilyIndices.computeFamily)
                {
                    copyQueueIndex = std::min(queueFamilyProperties[state.queueFamilyIndices.computeFamily].queueCount - 1, 1u);
                }

                static const float graphics_queue_prio = 0.5f;
                static const float compute_queue_prio = 1.0f;
                static const float transfer_queue_prio = 1.0f;
                float prio[3] = { graphics_queue_prio, compute_queue_prio, transfer_queue_prio };

                uint32_t queueCreateInfoCount = 0;
                VkDeviceQueueCreateInfo queueCreateInfo[3] = {};

                queueCreateInfo[queueCreateInfoCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo[queueCreateInfoCount].queueFamilyIndex = state.queueFamilyIndices.graphicsFamily;
                queueCreateInfo[queueCreateInfoCount].queueCount = std::min(universal_queue_index, queueFamilyProperties[state.queueFamilyIndices.graphicsFamily].queueCount);
                queueCreateInfo[queueCreateInfoCount].pQueuePriorities = prio;
                queueCreateInfoCount++;

                if (state.queueFamilyIndices.computeFamily != state.queueFamilyIndices.graphicsFamily)
                {
                    queueCreateInfo[queueCreateInfoCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                    queueCreateInfo[queueCreateInfoCount].queueFamilyIndex = state.queueFamilyIndices.computeFamily;
                    queueCreateInfo[queueCreateInfoCount].queueCount = std::min(state.queueFamilyIndices.transferFamily == state.queueFamilyIndices.computeFamily ? 2u : 1u,
                        queueFamilyProperties[state.queueFamilyIndices.computeFamily].queueCount);
                    queueCreateInfo[queueCreateInfoCount].pQueuePriorities = prio + 1;
                    queueCreateInfoCount++;
                }

                if (state.queueFamilyIndices.transferFamily != state.queueFamilyIndices.graphicsFamily
                    && state.queueFamilyIndices.transferFamily != state.queueFamilyIndices.computeFamily)
                {
                    queueCreateInfo[queueCreateInfoCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                    queueCreateInfo[queueCreateInfoCount].queueFamilyIndex = state.queueFamilyIndices.transferFamily;
                    queueCreateInfo[queueCreateInfoCount].queueCount = 1;
                    queueCreateInfo[queueCreateInfoCount].pQueuePriorities = prio + 2;
                    queueCreateInfoCount++;
                }

                Array<const char*> enabledExtensions;

                if (!headless)
                    enabledExtensions.Push(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

                if (state.physicalDeviceExtensions.getMemoryRequirements2)
                {
                    enabledExtensions.Push(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
                }

                if (state.physicalDeviceExtensions.getMemoryRequirements2
                    && state.physicalDeviceExtensions.dedicatedAllocation)
                {
                    enabledExtensions.Push(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
                }

                if (state.physicalDeviceExtensions.imageFormatList)
                {
                    enabledExtensions.Push(VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME);
                }

                if (state.physicalDeviceExtensions.debugMarker)
                {
                    enabledExtensions.Push(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
                }

#ifdef _WIN32
                if (state.surfaceCapabilities2 &&
                    state.physicalDeviceExtensions.win32.fullScreenExclusive)
                {
                    enabledExtensions.Push(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME);
                }
#endif

                if (state.externalMemoryCapabilities &&
                    state.externalSemaphoreCapabilities &&
                    state.physicalDeviceExtensions.getMemoryRequirements2 &&
                    state.physicalDeviceExtensions.dedicatedAllocation &&
                    state.physicalDeviceExtensions.externalSemaphore &&
                    state.physicalDeviceExtensions.externalSemaphore &&
#ifdef _WIN32
                    state.physicalDeviceExtensions.win32.externalMemory &&
                    state.physicalDeviceExtensions.win32.externalSemaphore
#else
                    state.physicalDeviceExtensions.fd.externalMemory &&
                    state.physicalDeviceExtensions.fd.externalSemaphore
#endif
                    )
                {
                    state.supportsExternal = true;
                    enabledExtensions.Push(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
                    enabledExtensions.Push(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
#ifdef _WIN32
                    enabledExtensions.Push(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME);
                    enabledExtensions.Push(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
#else
                    enabledExtensions.Push(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
                    enabledExtensions.Push(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
#endif
                }
                else
                {
                    state.supportsExternal = false;
                }

                if (state.physicalDeviceExtensions.maintenance1)
                {
                    enabledExtensions.Push(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
                }

                if (state.physicalDeviceExtensions.maintenance2)
                {
                    enabledExtensions.Push(VK_KHR_MAINTENANCE2_EXTENSION_NAME);
                }

                if (state.physicalDeviceExtensions.maintenance3)
                {
                    enabledExtensions.Push(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
                }

                if (state.physicalDeviceExtensions.bindMemory2)
                {
                    enabledExtensions.Push(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME);
                }

                if (state.physicalDeviceExtensions.memoryBudget)
                {
                    enabledExtensions.Push(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
                }

                VkPhysicalDeviceFeatures2 features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
                vkGetPhysicalDeviceFeatures2(state.physicalDevice, &features);

                // Enable device features we might care about.
                {
                    VkPhysicalDeviceFeatures enabledFeatures = {};
                    if (features.features.textureCompressionETC2)
                        enabledFeatures.textureCompressionETC2 = VK_TRUE;
                    if (features.features.textureCompressionBC)
                        enabledFeatures.textureCompressionBC = VK_TRUE;
                    if (features.features.textureCompressionASTC_LDR)
                        enabledFeatures.textureCompressionASTC_LDR = VK_TRUE;
                    if (features.features.fullDrawIndexUint32)
                        enabledFeatures.fullDrawIndexUint32 = VK_TRUE;
                    if (features.features.multiDrawIndirect)
                        enabledFeatures.multiDrawIndirect = VK_TRUE;
                    if (features.features.imageCubeArray)
                        enabledFeatures.imageCubeArray = VK_TRUE;
                    if (features.features.fillModeNonSolid)
                        enabledFeatures.fillModeNonSolid = VK_TRUE;
                    if (features.features.independentBlend)
                        enabledFeatures.independentBlend = VK_TRUE;
                    if (features.features.sampleRateShading)
                        enabledFeatures.sampleRateShading = VK_TRUE;
                    if (features.features.fragmentStoresAndAtomics)
                        enabledFeatures.fragmentStoresAndAtomics = VK_TRUE;
                    if (features.features.shaderStorageImageExtendedFormats)
                        enabledFeatures.shaderStorageImageExtendedFormats = VK_TRUE;
                    if (features.features.shaderStorageImageMultisample)
                        enabledFeatures.shaderStorageImageMultisample = VK_TRUE;
                    if (features.features.largePoints)
                        enabledFeatures.largePoints = VK_TRUE;
                    if (features.features.shaderInt16)
                        enabledFeatures.shaderInt16 = VK_TRUE;
                    if (features.features.shaderInt64)
                        enabledFeatures.shaderInt64 = VK_TRUE;

                    if (features.features.shaderSampledImageArrayDynamicIndexing)
                        enabledFeatures.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
                    if (features.features.shaderUniformBufferArrayDynamicIndexing)
                        enabledFeatures.shaderUniformBufferArrayDynamicIndexing = VK_TRUE;
                    if (features.features.shaderStorageBufferArrayDynamicIndexing)
                        enabledFeatures.shaderStorageBufferArrayDynamicIndexing = VK_TRUE;
                    if (features.features.shaderStorageImageArrayDynamicIndexing)
                        enabledFeatures.shaderStorageImageArrayDynamicIndexing = VK_TRUE;

                    features.features = enabledFeatures;
                }

                VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
                createInfo.pNext = &features;
                createInfo.queueCreateInfoCount = queueFamilyCount;
                createInfo.pQueueCreateInfos = queueCreateInfo;
                createInfo.enabledExtensionCount = enabledExtensions.Size();
                createInfo.ppEnabledExtensionNames = enabledExtensions.Data();

                if (vkCreateDevice(state.physicalDevice, &createInfo, nullptr, &state.device) != VK_SUCCESS)
                {
                    return false;
                }

                LogInfo("Created VkDevice with adapter '%s' API version: %u.%u.%u",
                    state.physicalDeviceProperties.deviceName,
                    VK_VERSION_MAJOR(state.physicalDeviceProperties.apiVersion),
                    VK_VERSION_MINOR(state.physicalDeviceProperties.apiVersion),
                    VK_VERSION_PATCH(state.physicalDeviceProperties.apiVersion));
                for (auto* ext_name : enabledExtensions)
                {
                    LogInfo("Device extension '%s'", ext_name);
                }

                volkLoadDeviceTable(&state.deviceTable, state.device);
                state.deviceTable.vkGetDeviceQueue(state.device, state.queueFamilyIndices.graphicsFamily, graphicsQueueIndex, &state.graphicsQueue);
                state.deviceTable.vkGetDeviceQueue(state.device, state.queueFamilyIndices.computeFamily, computeQueueIndex, &state.computeQueue);
                state.deviceTable.vkGetDeviceQueue(state.device, state.queueFamilyIndices.transferFamily, copyQueueIndex, &state.copyQueue);

                // Create memory allocator.
                {
                    VmaAllocatorCreateInfo createInfo{};
                    createInfo.physicalDevice = state.physicalDevice;
                    createInfo.device = state.device;
                    createInfo.instance = state.instance;

                    if (state.physicalDeviceExtensions.getMemoryRequirements2 &&
                        state.physicalDeviceExtensions.dedicatedAllocation)
                    {
                        createInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
                    }

                    if (state.physicalDeviceExtensions.bindMemory2)
                    {
                        createInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;
                    }

                    if (state.physicalDeviceExtensions.memoryBudget)
                    {
                        createInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
                    }

                    VkResult result = vmaCreateAllocator(&createInfo, &state.memoryAllocator);
                    if (result != VK_SUCCESS)
                    {
                        //VK_THROW(result, "Cannot create allocator");
                        return false;
                    }
                }

                return true;
            }

            static void VulkanShutdown()
            {
                if (state.instance == VK_NULL_HANDLE)
                    return;

                state.deviceTable.vkDeviceWaitIdle(state.device);

                if (state.memoryAllocator != VK_NULL_HANDLE)
                {
                    VmaStats stats;
                    vmaCalculateStats(state.memoryAllocator, &stats);

                    if (stats.total.usedBytes > 0) {
                        LogInfo("Total device memory leaked: %llx bytes.", stats.total.usedBytes);
                    }

                    vmaDestroyAllocator(state.memoryAllocator);
                }

                vkDestroyDevice(state.device, NULL);

                if (state.debugUtilsMessenger != VK_NULL_HANDLE) {
                    vkDestroyDebugUtilsMessengerEXT(state.instance, state.debugUtilsMessenger, nullptr);
                }

                vkDestroyInstance(state.instance, nullptr);
                state.instance = VK_NULL_HANDLE;
                memset(&state, 0, sizeof(state));
            }

            static void VulkanSetObjectName(VkObjectType objectType, uint64_t objectHandle, const char* objectName)
            {
                if (!state.debugUtils)
                {
                    return;
                }

                VkDebugUtilsObjectNameInfoEXT info;
                info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
                info.pNext = nullptr;
                info.objectType = objectType;
                info.objectHandle = objectHandle;
                info.pObjectName = objectName;
                vkSetDebugUtilsObjectNameEXT(state.device, &info);
            }

            /* Barriers */
            static VkAccessFlags VkGetAccessMask(TextureState state, VkImageAspectFlags aspectMask)
            {
                switch (state)
                {
                case TextureState::Undefined:
                case TextureState::General:
                case TextureState::Present:
                    //case VGPUTextureLayout_PreInitialized:
                    return (VkAccessFlagBits)0;
                case TextureState::RenderTarget:
                    if (aspectMask == VK_IMAGE_ASPECT_COLOR_BIT)
                    {
                        return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                    }

                    return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

                case TextureState::ShaderRead:
                    return VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;

                case TextureState::ShaderWrite:
                    return VK_ACCESS_SHADER_WRITE_BIT;

                    //case TextureState::ResolveDest:
                    case TextureState::CopyDest:
                        return VK_ACCESS_TRANSFER_WRITE_BIT;
                    //case TextureState::ResolveSource:
                    case TextureState::CopySource:
                        return VK_ACCESS_TRANSFER_READ_BIT;

                default:
                    ALIMER_UNREACHABLE();
                    return (VkAccessFlagBits)-1;
                }
            }

            static VkImageLayout VkGetImageLayout(TextureState layout, VkImageAspectFlags aspectMask)
            {
                switch (layout)
                {
                case TextureState::Undefined:
                    return VK_IMAGE_LAYOUT_UNDEFINED;

                case TextureState::General:
                    return VK_IMAGE_LAYOUT_GENERAL;

                case TextureState::RenderTarget:
                    if (aspectMask == VK_IMAGE_ASPECT_COLOR_BIT) {
                        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    }

                    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                case TextureState::DepthStencil:
                    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                case TextureState::DepthStencilReadOnly:
                    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

                case TextureState::ShaderRead:
                    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                case TextureState::ShaderWrite:
                    return VK_IMAGE_LAYOUT_GENERAL;

                case TextureState::Present:
                    return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

                default:
                    ALIMER_UNREACHABLE();
                    return (VkImageLayout)-1;
                }
            }

            static VkPipelineStageFlags VkGetShaderStageMask(TextureState layout, VkImageAspectFlags aspectMask, bool src)
            {
                switch (layout)
                {
                case TextureState::Undefined:
                case TextureState::General:
                    assert(src);
                    return src ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : (VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
                case TextureState::ShaderRead:
                case TextureState::ShaderWrite:
                    return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT; // #OPTME Assume the worst
                case TextureState::RenderTarget:
                    if (aspectMask == VK_IMAGE_ASPECT_COLOR_BIT)
                    {
                        return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    }

                    return src ? VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT : VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

                case TextureState::DepthStencilReadOnly:
                    return src ? VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT : VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                    //case Resource::State::IndirectArg:
                      //   return VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
                case TextureState::CopyDest:
                case TextureState::CopySource:
                    //case VGPUTextureLayout_ResolveDest:
                    //case VGPUTextureLayout_ResolveSource:
                    return VK_PIPELINE_STAGE_TRANSFER_BIT;
                case TextureState::Present:
                    return src ? (VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                default:
                    ALIMER_UNREACHABLE();
                    return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                }
            }

            static void TextureBarrier(TextureHandle handle, VkCommandBuffer commandBuffer, TextureState newState)
            {
                Texture& texture = state.textures[handle.value];
                if (texture.state == newState) {
                    return;
                }

                const VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // GetVkAspectMask(texture.format);

                // Create an image barrier object
                VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
                barrier.pNext = nullptr;
                barrier.srcAccessMask = VkGetAccessMask(texture.state, aspectMask);
                barrier.dstAccessMask = VkGetAccessMask(newState, aspectMask);
                barrier.oldLayout = VkGetImageLayout(texture.state, aspectMask);
                barrier.newLayout = VkGetImageLayout(newState, aspectMask);
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = texture.handle;
                barrier.subresourceRange.aspectMask = aspectMask;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

                VkPipelineStageFlags srcStageMask = VkGetShaderStageMask(texture.state, aspectMask, true);
                VkPipelineStageFlags dstStageMask = VkGetShaderStageMask(newState, aspectMask, false);

                state.deviceTable.vkCmdPipelineBarrier(
                    commandBuffer,
                    srcStageMask,
                    dstStageMask,
                    0, 0, nullptr, 0, nullptr, 1, &barrier);

                texture.state = newState;
            }

            static ContextHandle VulkanCreateContext(const ContextInfo& info) {
                if (state.contexts.isFull()) {
                    LogError("Not enough free context slots.");
                    return kInvalidContext;
                }

                VkResult result = VK_SUCCESS;

                const int id = state.contexts.alloc();
                Context& context = state.contexts[id];
                context.surfaceFormat.format = VK_FORMAT_UNDEFINED;
                context.surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                ContextHandle contextHandle = { (uint32_t)id };

#if defined(VK_USE_PLATFORM_WIN32_KHR)
                VkWin32SurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
                createInfo.hinstance = GetModuleHandle(NULL);
                createInfo.hwnd = reinterpret_cast<HWND>(info.handle);
                result = vkCreateWin32SurfaceKHR(state.instance, &createInfo, nullptr, &context.surface);
#endif

                if (result != VK_SUCCESS)
                {
                    LogError("Failed to create surface for SwapChain");
                    state.contexts.dealloc(contextHandle.value);
                    return kInvalidContext;
                }

                VkBool32 surfaceSupported = false;
                result = vkGetPhysicalDeviceSurfaceSupportKHR(state.physicalDevice,
                    state.queueFamilyIndices.graphicsFamily,
                    context.surface,
                    &surfaceSupported);

                if (result != VK_SUCCESS || !surfaceSupported)
                {
                    vkDestroySurfaceKHR(state.instance, context.surface, nullptr);
                    state.contexts.dealloc(contextHandle.value);
                    return kInvalidContext;
                }

                if (!ResizeContext(contextHandle, info.width, info.height)) {
                    state.contexts.dealloc(contextHandle.value);
                    return kInvalidContext;
                }

                return contextHandle;
            }

            static void VulkanDestroyContext(ContextHandle handle)
            {
                Context& context = state.contexts[handle.value];

                state.deviceTable.vkDeviceWaitIdle(state.device);

                for (uint32_t i = 0; i < context.maxInflightFrames; i++) {
                    Frame* frame = &context.frames[i];

                    state.deviceTable.vkFreeCommandBuffers(state.device, frame->commandPool, 1, &frame->commandBuffer);
                    state.deviceTable.vkDestroyCommandPool(state.device, frame->commandPool, nullptr);
                    state.deviceTable.vkDestroyFence(state.device, frame->fence, nullptr);
                    state.deviceTable.vkDestroySemaphore(state.device, frame->imageAcquiredSemaphore, nullptr);
                    state.deviceTable.vkDestroySemaphore(state.device, frame->renderCompleteSemaphore, nullptr);
                }

                free(context.frames);

                if (context.handle != VK_NULL_HANDLE)
                {
                    state.deviceTable.vkDestroySwapchainKHR(state.device, context.handle, nullptr);
                    context.handle = VK_NULL_HANDLE;
                }

                // Destroy surface
                if (context.surface != VK_NULL_HANDLE)
                {
                    vkDestroySurfaceKHR(state.instance, context.surface, nullptr);
                    context.surface = VK_NULL_HANDLE;
                }

                state.contexts.dealloc(handle.value);
            }

            static VkPresentModeKHR ChooseSwapPresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, bool vsyncEnabled)
            {
                /* Choose present mode. */
                uint32_t presentModesCount;
                VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModesCount, nullptr));

                VkPresentModeKHR* presentModes = StackAlloc<VkPresentModeKHR>(presentModesCount);
                VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModesCount, presentModes));

                for (uint32_t i = 0; i < presentModesCount; i++)
                {
                    if (vsyncEnabled)
                    {
                        if (presentModes[i] == VK_PRESENT_MODE_FIFO_KHR)
                            return VK_PRESENT_MODE_FIFO_KHR;
                        else if (presentModes[i] == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
                            return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
                    }
                    else
                    {
                        if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
                            return VK_PRESENT_MODE_IMMEDIATE_KHR;
                        else if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
                            return VK_PRESENT_MODE_MAILBOX_KHR;
                    }
                }

                // If no match was found, return the first present mode or default to FIFO.
                if (presentModesCount > 0)
                    return presentModes[0];

                return VK_PRESENT_MODE_FIFO_KHR;
            }

            static bool VulkanResizeContext(ContextHandle handle, uint32_t width, uint32_t height) {
                Context& context = state.contexts[handle.value];

                VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo = {
                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
                    nullptr,
                    context.surface
                };

                uint32_t format_count;
                VkSurfaceFormatKHR* formats;

                if (state.surfaceCapabilities2)
                {
                    if (vkGetPhysicalDeviceSurfaceFormats2KHR(state.physicalDevice, &surfaceInfo, &format_count, nullptr) != VK_SUCCESS)
                        return false;

                    VkSurfaceFormat2KHR* formats2 = StackAlloc<VkSurfaceFormat2KHR>(format_count);

                    for (uint32_t i = 0; i < format_count; i++)
                    {
                        formats2[i] = {};
                        formats2[i].sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
                    }

                    if (vkGetPhysicalDeviceSurfaceFormats2KHR(state.physicalDevice, &surfaceInfo, &format_count, formats2) != VK_SUCCESS)
                        return false;

                    formats = StackAlloc<VkSurfaceFormatKHR>(format_count);
                    for (uint32_t i = 0; i < format_count; i++)
                    {
                        formats[i] = formats2[i].surfaceFormat;
                    }
                }
                else
                {
                    if (vkGetPhysicalDeviceSurfaceFormatsKHR(state.physicalDevice, context.surface, &format_count, nullptr) != VK_SUCCESS)
                        return false;

                    formats = StackAlloc<VkSurfaceFormatKHR>(format_count);
                    if (vkGetPhysicalDeviceSurfaceFormatsKHR(state.physicalDevice, context.surface, &format_count, formats) != VK_SUCCESS)
                        return false;
                }

                const bool srgb = false;
                if (format_count == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
                {
                    context.surfaceFormat = formats[0];
                    context.surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
                }
                else
                {
                    if (format_count == 0)
                    {
                        LogError("Vulkan: Surface has no formats.");
                        return false;
                    }

                    bool found = false;
                    for (uint32_t i = 0; i < format_count; i++)
                    {
                        if (srgb)
                        {
                            if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB ||
                                formats[i].format == VK_FORMAT_B8G8R8A8_SRGB ||
                                formats[i].format == VK_FORMAT_A8B8G8R8_SRGB_PACK32)
                            {
                                context.surfaceFormat = formats[i];
                                found = true;
                            }
                        }
                        else
                        {
                            if (formats[i].format == VK_FORMAT_R8G8B8A8_UNORM ||
                                formats[i].format == VK_FORMAT_B8G8R8A8_UNORM ||
                                formats[i].format == VK_FORMAT_A8B8G8R8_UNORM_PACK32)
                            {
                                context.surfaceFormat = formats[i];
                                found = true;
                            }
                        }
                    }

                    if (!found)
                        context.surfaceFormat = formats[0];
                }

                VkSurfaceCapabilitiesKHR capabilities;
                if (state.surfaceCapabilities2)
                {
                    VkSurfaceCapabilities2KHR surfaceCapabilities2 = { VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR };

                    // TODO: Add fullscreen exclusive.

                    if (vkGetPhysicalDeviceSurfaceCapabilities2KHR(state.physicalDevice, &surfaceInfo, &surfaceCapabilities2) != VK_SUCCESS)
                        return false;

                    capabilities = surfaceCapabilities2.surfaceCapabilities;
                }
                else
                {
                    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state.physicalDevice, context.surface, &capabilities) != VK_SUCCESS)
                        return false;
                }

                if (capabilities.maxImageExtent.width == 0
                    && capabilities.maxImageExtent.height == 0)
                {
                    return false;
                }

                const bool tripleBuffer = false;
                uint32_t minImageCount = (tripleBuffer) ? 3 : capabilities.minImageCount + 1;
                if (capabilities.maxImageCount > 0 &&
                    minImageCount > capabilities.maxImageCount)
                {
                    minImageCount = capabilities.maxImageCount;
                }

                // Choose swapchain extent (Size)
                VkExtent2D newExtent = {};
                if (capabilities.currentExtent.width != UINT32_MAX ||
                    capabilities.currentExtent.height != UINT32_MAX ||
                    width == 0 || height == 0)
                {
                    newExtent = capabilities.currentExtent;
                }
                else
                {
                    newExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, width));
                    newExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, height));
                }
                newExtent.width = std::max(newExtent.width, 1u);
                newExtent.height = std::max(newExtent.height, 1u);

                // Enable transfer source and destination on swap chain images if supported
                VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                if (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
                    imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
                }
                if (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
                    imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                }

                VkSurfaceTransformFlagBitsKHR preTransform;
                if ((capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) != 0) {
                    // We prefer a non-rotated transform
                    preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
                }
                else {
                    preTransform = capabilities.currentTransform;
                }

                VkCompositeAlphaFlagBitsKHR compositeMode = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
                if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
                    compositeMode = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
                if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
                    compositeMode = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
                if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
                    compositeMode = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
                if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
                    compositeMode = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;

                VkPresentModeKHR presentMode = ChooseSwapPresentMode(state.physicalDevice, context.surface, true);

                VkSwapchainKHR oldSwapchain = context.handle;

                VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
                createInfo.surface = context.surface;
                createInfo.minImageCount = minImageCount;
                createInfo.imageFormat = context.surfaceFormat.format;
                createInfo.imageColorSpace = context.surfaceFormat.colorSpace;
                createInfo.imageExtent = newExtent;
                createInfo.imageArrayLayers = 1;
                createInfo.imageUsage = imageUsage;
                createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                createInfo.preTransform = preTransform;
                createInfo.compositeAlpha = compositeMode;
                createInfo.presentMode = presentMode;
                createInfo.clipped = VK_TRUE;
                createInfo.oldSwapchain = oldSwapchain;

                VkResult result = state.deviceTable.vkCreateSwapchainKHR(state.device, &createInfo, nullptr, &context.handle);
                if (result != VK_SUCCESS)
                {
                    //VK_THROW(result, "Cannot create Swapchain");
                    return false;
                }

                LogDebug("[Vulkan]: Created SwapChain");

                if (oldSwapchain != VK_NULL_HANDLE)
                {
                    state.deviceTable.vkDestroySwapchainKHR(state.device, oldSwapchain, nullptr);
                }

                // Get SwapChain images
                if (state.deviceTable.vkGetSwapchainImagesKHR(state.device, context.handle, &context.imageCount, nullptr) != VK_SUCCESS) {
                    return false;
                }

                VkImage swapChainImages[16] = {};
                result = state.deviceTable.vkGetSwapchainImagesKHR(state.device, context.handle, &context.imageCount, swapChainImages);
                if (result != VK_SUCCESS)
                {
                    //VK_THROW(result, "[Vulkan]: Failed to retrive SwapChain-Images");
                    return false;
                }

                context.surfaceExtent = newExtent;
                context.frameIndex = 0;
                context.imageIndex = 0;
                context.maxInflightFrames = context.imageCount;
                context.frames = (Frame*)malloc(sizeof(Frame) * context.maxInflightFrames);
                context.backbuffers.Resize(context.imageCount);
                for (uint32_t i = 0; i < context.maxInflightFrames; i++)
                {
                    context.frames[i].index = i;

                    VkCommandPoolCreateInfo commandPoolInfo = {};
                    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
                    commandPoolInfo.queueFamilyIndex = state.queueFamilyIndices.graphicsFamily;
                    if (state.deviceTable.vkCreateCommandPool(state.device, &commandPoolInfo, nullptr, &context.frames[i].commandPool) != VK_SUCCESS) {
                        return false;
                    }

                    VkFenceCreateInfo fenceInfo = {};
                    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
                    if (state.deviceTable.vkCreateFence(state.device, &fenceInfo, NULL, &context.frames[i].fence) != VK_SUCCESS) {
                        return false;
                    }

                    VkSemaphoreCreateInfo semaphoreInfo = {};
                    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                    if (state.deviceTable.vkCreateSemaphore(state.device, &semaphoreInfo, NULL, &context.frames[i].imageAcquiredSemaphore) != VK_SUCCESS) {
                        return false;
                    }

                    if (state.deviceTable.vkCreateSemaphore(state.device, &semaphoreInfo, NULL, &context.frames[i].renderCompleteSemaphore) != VK_SUCCESS) {
                        return false;
                    }

                    context.frames[i].commandBuffer = VK_NULL_HANDLE;
                }

                const char* names[] =
                {
                    "BackBuffer[0]",
                    "BackBuffer[1]",
                    "BackBuffer[2]",
                    "BackBuffer[3]",
                    "BackBuffer[4]",
                };

                for (uint32_t i = 0; i < context.imageCount; i++)
                {
                    TextureInfo textureInfo = {};
                    textureInfo.width = context.surfaceExtent.width;
                    textureInfo.height = context.surfaceExtent.height;
                    textureInfo.format = PixelFormat::BGRA8UNorm;
                    textureInfo.usage = TextureUsage::OutputAttachment;
                    textureInfo.label = names[i];
                    textureInfo.externalHandle = swapChainImages[i];
                    context.backbuffers[i] = CreateTexture(textureInfo);
                }

                return true;
            }

            static void HandleSurfaceChanges(ContextHandle handle)
            {
                Context& context = state.contexts[handle.value];

                VkSurfaceCapabilitiesKHR capabilities;
                VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state.physicalDevice,
                    context.surface,
                    &capabilities));

                // Only recreate the swapchain if the dimensions have changed;
                // handle_surface_changes() is called on VK_SUBOPTIMAL_KHR,
                // which might not be due to a surface resize
                if (capabilities.currentExtent.width != context.surfaceExtent.width ||
                    capabilities.currentExtent.height != context.surfaceExtent.height)
                {
                    // Recreate swapchain
                    state.deviceTable.vkDeviceWaitIdle(state.device);

                    VulkanResizeContext(handle, capabilities.currentExtent.width, capabilities.currentExtent.height);

                    context.surfaceExtent = capabilities.currentExtent;
                }
            }

            static void DestroyFrame(Frame* frame)
            {

            }

            static VkResult AcquireNextImage(ContextHandle handle, VkSemaphore semaphore, VkFence fence = VK_NULL_HANDLE)
            {
                Context& context = state.contexts[handle.value];
                return state.deviceTable.vkAcquireNextImageKHR(state.device,
                    context.handle,
                    std::numeric_limits<uint64_t>::max(),
                    semaphore,
                    fence,
                    &context.imageIndex);
            }

            static bool VulkanBeginFrame(ContextHandle handle)
            {
                HandleSurfaceChanges(handle);

                Context& context = state.contexts[handle.value];
                Frame* frame = &context.frames[context.frameIndex];

                VK_CHECK(state.deviceTable.vkWaitForFences(state.device, 1, &frame->fence, VK_TRUE, UINT64_MAX));
                VK_CHECK(state.deviceTable.vkResetFences(state.device, 1, &frame->fence));

                DestroyFrame(frame);

                if (context.handle)
                {
                    VkResult result = AcquireNextImage(handle, frame->imageAcquiredSemaphore, VK_NULL_HANDLE);

                    if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
                    {
                        HandleSurfaceChanges(handle);

                        result = AcquireNextImage(handle, frame->imageAcquiredSemaphore, VK_NULL_HANDLE);
                    }
                }

                // Reset command pool first.
                VK_CHECK(state.deviceTable.vkResetCommandPool(state.device, frame->commandPool, 0));

                // Allocate frame command buffer
                VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
                allocateInfo.commandPool = frame->commandPool;
                allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                allocateInfo.commandBufferCount = 1u;
                VK_CHECK(state.deviceTable.vkAllocateCommandBuffers(state.device, &allocateInfo, &frame->commandBuffer));

                // Begin frame command buffer.
                VkCommandBufferBeginInfo beginInfo = {};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                VK_CHECK(state.deviceTable.vkBeginCommandBuffer(frame->commandBuffer, &beginInfo));

                return true;
            }

            static void VulkanEndFrame(ContextHandle handle)
            {
                Context& context = state.contexts[handle.value];
                Frame* frame = &context.frames[context.frameIndex];

                // End frame command buffer.
                VK_CHECK(state.deviceTable.vkEndCommandBuffer(frame->commandBuffer));

                const VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = &frame->imageAcquiredSemaphore;
                submitInfo.pWaitDstStageMask = &waitDstStageMask;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &frame->commandBuffer;
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = &frame->renderCompleteSemaphore;
                VK_CHECK(state.deviceTable.vkQueueSubmit(state.graphicsQueue, 1, &submitInfo, frame->fence));

                if (context.handle)
                {
                    VkPresentInfoKHR presentInfo = {};
                    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                    presentInfo.waitSemaphoreCount = 1;
                    presentInfo.pWaitSemaphores = &frame->renderCompleteSemaphore;
                    presentInfo.swapchainCount = 1;
                    presentInfo.pSwapchains = &context.handle;
                    presentInfo.pImageIndices = &context.imageIndex;

                    VkResult result = state.deviceTable.vkQueuePresentKHR(state.graphicsQueue, &presentInfo);

                    if (result == VK_SUBOPTIMAL_KHR
                        || result == VK_ERROR_OUT_OF_DATE_KHR)
                    {
                        HandleSurfaceChanges(handle);
                    }
                }

                // Advance frame index.
                context.frameIndex = (context.frameIndex + 1) % context.maxInflightFrames;
            }

            static void VulkanBeginRenderPass(ContextHandle handle, const Color& clearColor, float clearDepth, uint8_t clearStencil)
            {
                Context& context = state.contexts[handle.value];
                Frame* frame = &context.frames[context.frameIndex];

                VkRenderPassBeginInfo renderPassBeginInfo = {};
                renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                //renderPassBeginInfo.renderPass = wd->RenderPass;
                //renderPassBeginInfo.framebuffer = fd->Framebuffer;
                renderPassBeginInfo.renderArea.extent.width = context.surfaceExtent.width;
                renderPassBeginInfo.renderArea.extent.height = context.surfaceExtent.height;
                //renderPassBeginInfo.clearValueCount = (viewport->Flags & ImGuiViewportFlags_NoRendererClear) ? 0 : 1;
                //renderPassBeginInfo.pClearValues = (viewport->Flags & ImGuiViewportFlags_NoRendererClear) ? NULL : &wd->ClearValue;
                state.deviceTable.vkCmdBeginRenderPass(frame->commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
            }

            static void VulkanEndRenderPass(ContextHandle handle)
            {
                Context& context = state.contexts[handle.value];
                Frame* frame = &context.frames[context.frameIndex];

                state.deviceTable.vkCmdEndRenderPass(frame->commandBuffer);
            }

            /* Texture */
            static TextureHandle VulkanCreateTexture(const TextureInfo& info)
            {
                if (state.textures.isFull()) {
                    LogError("Not enough free texture slots.");
                    return kInvalidTexture;
                }

                const int id = state.textures.alloc();
                Texture& texture = state.textures[id];
                texture.format = GetVkFormat(info.format);
                if (info.externalHandle != nullptr)
                {
                    texture.handle = (VkImage)info.externalHandle;
                }

                VkImageViewCreateInfo viewCreateInfo = {};
                viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                viewCreateInfo.pNext = nullptr;
                viewCreateInfo.flags = 0;
                viewCreateInfo.image = texture.handle;
                viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                viewCreateInfo.format = texture.format;
                viewCreateInfo.components = {
                    VK_COMPONENT_SWIZZLE_R,
                    VK_COMPONENT_SWIZZLE_G,
                    VK_COMPONENT_SWIZZLE_B,
                    VK_COMPONENT_SWIZZLE_A
                };
                viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                viewCreateInfo.subresourceRange.baseMipLevel = 0;
                viewCreateInfo.subresourceRange.levelCount = 1;
                viewCreateInfo.subresourceRange.baseArrayLayer = 0;
                viewCreateInfo.subresourceRange.layerCount = 1;

                VK_CHECK(state.deviceTable.vkCreateImageView(state.device, &viewCreateInfo, nullptr, &texture.view));

                return { (uint32_t)id };
            }

            static void VulkanDestroyTexture(TextureHandle handle)
            {
                Texture& texture = state.textures[handle.value];
                state.textures.dealloc(handle.value);
            }

            bool IsSupported(void)
            {
                return VulkanIsSupported();
            }

            Renderer* CreateRenderer(void) {
                static Renderer renderer = { nullptr };
                renderer.Init = VulkanInit;
                renderer.Shutdown = VulkanShutdown;
                renderer.CreateContext = VulkanCreateContext;
                renderer.DestroyContext = VulkanDestroyContext;
                renderer.ResizeContext = VulkanResizeContext;
                renderer.BeginFrame = VulkanBeginFrame;
                renderer.EndFrame = VulkanEndFrame;
                renderer.BeginRenderPass = VulkanBeginRenderPass;
                renderer.EndRenderPass = VulkanEndRenderPass;

                renderer.CreateTexture = VulkanCreateTexture;
                renderer.DestroyTexture = VulkanDestroyTexture;
                return &renderer;
            }
        }
    }
}
