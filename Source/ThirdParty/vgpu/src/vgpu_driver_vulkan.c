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

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "vgpu_driver.h"

/* Needed by vk_mem_alloc */
PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = NULL;
PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = NULL;

// Functions that don't require an instance
#define GPU_FOREACH_ANONYMOUS(X)\
    X(vkCreateInstance)\
    X(vkEnumerateInstanceExtensionProperties)\
    X(vkEnumerateInstanceLayerProperties)\
    X(vkEnumerateInstanceVersion)

// Functions that require an instance but don't require a device
#define GPU_FOREACH_INSTANCE(X)\
    X(vkDestroyInstance)\
    X(vkCreateDebugUtilsMessengerEXT)\
    X(vkDestroyDebugUtilsMessengerEXT)

#define GPU_DECLARE(fn) static PFN_##fn fn;
#define GPU_LOAD_ANONYMOUS(fn) fn = (PFN_##fn) vkGetInstanceProcAddr(NULL, #fn);
#define GPU_LOAD_INSTANCE(fn) fn = (PFN_##fn) vkGetInstanceProcAddr(vk.instance, #fn);
#define GPU_LOAD_DEVICE(fn) fn = (PFN_##fn) vkGetDeviceProcAddr(vk.device, #fn);

/* Declare function pointers */
GPU_FOREACH_ANONYMOUS(GPU_DECLARE);
GPU_FOREACH_INSTANCE(GPU_DECLARE);

typedef struct vk_texture {
    VkImage handle;
    uint32_t width;
    uint32_t height;
} vk_texture;

typedef struct vk_framebuffer {
    uint32_t width;
    uint32_t height;
    uint32_t layers;
} vk_framebuffer;

/* Global data */
static struct {
    bool available_initialized;
    bool available;
    void* library;

    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VmaAllocator allocator;
} vk;

static bool vk_init(const vgpu_config* config) {
    VkResult result = VK_SUCCESS;

    // Instance
    { 
        const char* layers[] = {
          "VK_LAYER_LUNARG_object_tracker",
          "VK_LAYER_LUNARG_core_validation",
          "VK_LAYER_LUNARG_parameter_validation"
        };

        const char* extensions[] = {
          "VK_EXT_debug_utils"
        };

        uint32_t apiVersion = VK_API_VERSION_1_1;
        if (vkEnumerateInstanceVersion &&
            vkEnumerateInstanceVersion(&apiVersion) != VK_SUCCESS)
        {
            apiVersion = VK_API_VERSION_1_1;
        }

        if (apiVersion < VK_API_VERSION_1_1) {
            apiVersion = VK_API_VERSION_1_1;
        }

        VkInstanceCreateInfo instance_info = {
          .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
          .pApplicationInfo = &(VkApplicationInfo) {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .apiVersion = apiVersion
          },
          .enabledLayerCount = config->debug ? _vgpu_count_of(layers) : 0,
          .ppEnabledLayerNames = layers,
          .enabledExtensionCount = config->debug ? _vgpu_count_of(extensions) : 0,
          .ppEnabledExtensionNames = extensions
        };

        if (vkCreateInstance(&instance_info, NULL, &vk.instance)) {
            vgpu_shutdown();
            return false;
        }

        GPU_FOREACH_INSTANCE(GPU_LOAD_INSTANCE);
    }

    VmaAllocatorCreateFlags allocator_flags = 0;

    const VmaAllocatorCreateInfo allocator_info = {
        .flags = allocator_flags,
        .physicalDevice = vk.physical_device,
        .device = vk.device,
        .instance = vk.instance
    };

    result = vmaCreateAllocator(&allocator_info, &vk.allocator);
    if (result != VK_SUCCESS) {
        //GPU_VK_THROW("Vulkan: Cannot create memory allocator.");
        vgpu_shutdown();
        return false;
    }

    return true;
}

static void vk_shutdown(void) {
}

static bool vk_frame_begin(void) {
    return true;
}

static void vk_frame_end(void) {

}

/* Texture */
static vgpu_texture vk_texture_create(const vgpu_texture_info* info) {
    vk_texture* texture = VGPU_ALLOC_HANDLE(vk_texture);
    return (vgpu_texture)texture;
}

static void vk_texture_destroy(vgpu_texture handle) {
    vk_texture* texture = (vk_texture*)handle;
    VGPU_FREE(texture);
}

static uint32_t vk_texture_get_width(vgpu_texture handle, uint32_t mipLevel) {
    vk_texture* texture = (vk_texture*)handle;
    return _vgpu_max(1, texture->width >> mipLevel);
}

static uint32_t vk_texture_get_height(vgpu_texture handle, uint32_t mipLevel) {
    vk_texture* texture = (vk_texture*)handle;
    return _vgpu_max(1, texture->height >> mipLevel);
}

/* Framebuffer */
static vgpu_framebuffer vk_framebuffer_create(const VGPUFramebufferDescription* desc) {
    vk_framebuffer* framebuffer = VGPU_ALLOC_HANDLE(vk_framebuffer);
    return (vgpu_framebuffer)framebuffer;
}

static vgpu_framebuffer vk_framebuffer_create_from_window(uintptr_t window_handle, VGPUPixelFormat color_format, VGPUPixelFormat depth_stencil_format) {
    vk_framebuffer* framebuffer = VGPU_ALLOC_HANDLE(vk_framebuffer);
    return (vgpu_framebuffer)framebuffer;
}

static void vk_framebuffer_destroy(vgpu_framebuffer handle) {
    vk_framebuffer* framebuffer = (vk_framebuffer*)handle;
    VGPU_FREE(framebuffer);
}

/* Driver functions */
static bool vulkan_is_supported(void) {
    if (vk.available_initialized) {
        return vk.available;
    }

    vk.available_initialized = true;
#if defined(_WIN32)
    vk.library = LoadLibraryA("vulkan-1.dll");
    if (!vk.library) {
        return false;
    }

    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(vk.library, "vkGetInstanceProcAddr");
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
        return false;

    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(vk.library, "vkGetInstanceProcAddr");
#endif

    GPU_FOREACH_ANONYMOUS(GPU_LOAD_ANONYMOUS);

    // We reuire vulkan 1.1.0 or higher API
    VkInstanceCreateInfo instance_info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &(VkApplicationInfo) {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = VK_API_VERSION_1_1
      },
    };

    VkInstance instance;
    VkResult result = vkCreateInstance(&instance_info, NULL, &instance);
    if (result != VK_SUCCESS) {
        return false;
    }

    vkDestroyInstance = (PFN_vkDestroyInstance)vkGetInstanceProcAddr(instance, "vkDestroyInstance");
    vkDestroyInstance(instance, NULL);
    vk.available = true;
    return true;
}

static vgpu_context* vulkan_create_context(void) {
    static vgpu_context context = { 0 };
    ASSIGN_DRIVER(vk);
    return &context;
}

vgpu_driver vulkan_driver = {
    VGPU_BACKEND_TYPE_VULKAN,
    vulkan_is_supported,
    vulkan_create_context
};

#endif /* defined(VGPU_DRIVER_VULKAN) */
