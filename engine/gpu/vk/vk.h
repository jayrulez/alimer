
#if defined(VULKAN_H_) && !defined(VK_NO_PROTOTYPES)
#	error To use vk, you need to define VK_NO_PROTOTYPES before including vulkan.h
#endif

#ifndef VK_NO_PROTOTYPES
#   define VK_NO_PROTOTYPES
#endif

//#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    extern PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
    extern PFN_vkCreateInstance vkCreateInstance;
    extern PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;
    extern PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;

#if defined(VK_VERSION_1_1)
    extern PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion;
#endif

    /* Instance functions */
    extern PFN_vkDestroyInstance vkDestroyInstance;
    extern PFN_vkCreateDevice vkCreateDevice;
    extern PFN_vkDestroyDevice vkDestroyDevice;
    extern PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
    extern PFN_vkEnumerateDeviceLayerProperties vkEnumerateDeviceLayerProperties;
    extern PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
    extern PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
    extern PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
    extern PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties;
    extern PFN_vkGetPhysicalDeviceImageFormatProperties vkGetPhysicalDeviceImageFormatProperties;
    extern PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
    extern PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
    extern PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
    extern PFN_vkGetPhysicalDeviceSparseImageFormatProperties vkGetPhysicalDeviceSparseImageFormatProperties;

#if defined(_WIN32)
    extern PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR vkGetPhysicalDeviceWin32PresentationSupportKHR;
    extern PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
#endif /* defined(_WIN32) */

    extern PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
    extern PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT;
    extern PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;
    extern PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
    extern PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;

    bool agpu_vk_init_loader();
    uint32_t agpu_vk_get_instance_version(void);
    void agpu_vk_init_instance(VkInstance instance);
    void agpu_vk_init_device(VkDevice device);

#ifdef __cplusplus
    }
#endif
