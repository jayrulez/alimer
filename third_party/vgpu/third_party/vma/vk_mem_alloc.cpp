#ifndef NOMINMAX
#   define NOMINMAX
#endif

#include <vulkan/vulkan.h>

extern "C" {
    extern PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
    extern PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
}

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
