#include <vulkan/vulkan.h>

extern "C" PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
extern "C" PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
