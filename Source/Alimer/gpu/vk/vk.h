
#if defined(_WIN32) || defined(_WIN64)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#endif

#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif

#include <vulkan/vulkan.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    extern PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;

    /* Global functions, without instance. */
    extern PFN_vkCreateInstance vkCreateInstance;
    extern PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;
    extern PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
    extern PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion;

    /* Instance functions */
    extern PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
    extern PFN_vkDestroyInstance vkDestroyInstance;
    extern PFN_vkCreateDevice vkCreateDevice;
    extern PFN_vkDestroyDevice vkDestroyDevice;
    extern PFN_vkGetDeviceQueue vkGetDeviceQueue;
    extern PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
    extern PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
    extern PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
    extern PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
    extern PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;

    /* VK_EXT_debug_utils */
    extern PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
    extern PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
    extern PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;

#if defined(VK_KHR_surface)
    extern PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
    extern PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
    extern PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
    extern PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
    extern PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
#endif /* defined(VK_KHR_surface) */

#if defined(VK_KHR_win32_surface)
    extern PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
    extern PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR vkGetPhysicalDeviceWin32PresentationSupportKHR;
#endif /* defined(VK_KHR_win32_surface) */

    /* Device functions */
    extern PFN_vkQueueSubmit vkQueueSubmit;
    extern PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
    extern PFN_vkCreateCommandPool vkCreateCommandPool;
    extern PFN_vkDestroyCommandPool vkDestroyCommandPool;
    extern PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
    extern PFN_vkFreeCommandBuffers vkFreeCommandBuffers;
    extern PFN_vkCreateFence vkCreateFence;
    extern PFN_vkDestroyFence vkDestroyFence;
    extern PFN_vkWaitForFences vkWaitForFences;
    extern PFN_vkResetFences vkResetFences;
    extern PFN_vkCreateBuffer vkCreateBuffer;
    extern PFN_vkDestroyBuffer vkDestroyBuffer;
    extern PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
    extern PFN_vkBindBufferMemory vkBindBufferMemory;
    extern PFN_vkCreateImage vkCreateImage;
    extern PFN_vkDestroyImage vkDestroyImage;
    extern PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
    extern PFN_vkBindImageMemory vkBindImageMemory;
    extern PFN_vkAllocateMemory vkAllocateMemory;
    extern PFN_vkFreeMemory vkFreeMemory;
    extern PFN_vkMapMemory vkMapMemory;
    extern PFN_vkUnmapMemory vkUnmapMemory;
    extern PFN_vkCreateSampler vkCreateSampler;
    extern PFN_vkDestroySampler vkDestroySampler;
    extern PFN_vkCreateRenderPass vkCreateRenderPass;
    extern PFN_vkDestroyRenderPass vkDestroyRenderPass;
    extern PFN_vkCreateImageView vkCreateImageView;
    extern PFN_vkDestroyImageView vkDestroyImageView;
    extern PFN_vkCreateFramebuffer vkCreateFramebuffer;
    extern PFN_vkDestroyFramebuffer vkDestroyFramebuffer;
    extern PFN_vkCreateShaderModule vkCreateShaderModule;
    extern PFN_vkDestroyShaderModule vkDestroyShaderModule;
    extern PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
    extern PFN_vkDestroyPipeline vkDestroyPipeline;

    /* CommandBuffer functions */
    extern PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
    extern PFN_vkEndCommandBuffer vkEndCommandBuffer;
    extern PFN_vkCmdCopyBuffer vkCmdCopyBuffer;
    extern PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage;
    extern PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;
    extern PFN_vkCmdBindPipeline vkCmdBindPipeline;
    extern PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
    extern PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer;
    extern PFN_vkCmdDraw vkCmdDraw;
    extern PFN_vkCmdDrawIndexed vkCmdDrawIndexed;
    extern PFN_vkCmdDrawIndirect vkCmdDrawIndirect;
    extern PFN_vkCmdDrawIndexedIndirect vkCmdDrawIndexedIndirect;
    extern PFN_vkCmdDispatch vkCmdDispatch;
    extern PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
    extern PFN_vkCmdEndRenderPass vkCmdEndRenderPass;

#if defined(VK_KHR_swapchain)
    extern PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
    extern PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
    extern PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
    extern PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
    extern PFN_vkQueuePresentKHR vkQueuePresentKHR;
#endif /* defined(VK_KHR_swapchain) */

    bool vk_init_loader();
    void vk_init_instance(VkInstance instance);
    void vk_init_device(VkDevice device);

#ifdef __cplusplus
}
#endif
