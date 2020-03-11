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

#pragma once

#include "config.h"
#include "Graphics/Types.h"

#if defined(ALIMER_VULKAN)

#ifndef VK_NULL_HANDLE
#   define VK_NULL_HANDLE 0
#endif

#ifndef VK_DEFINE_HANDLE
#   define VK_DEFINE_HANDLE(object) typedef struct object##_T* object;
#endif

#if !defined(VK_DEFINE_NON_DISPATCHABLE_HANDLE)
#   if defined(__LP64__) || defined(_WIN64) || (defined(__x86_64__) && !defined(__ILP32__) ) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
#       define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef struct object##_T *object;
#   else
#       define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef uint64_t object;
#   endif
#endif

VK_DEFINE_HANDLE(VkInstance);
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDebugUtilsMessengerEXT);
VK_DEFINE_HANDLE(VkPhysicalDevice);
VK_DEFINE_HANDLE(VkDevice);
VK_DEFINE_HANDLE(VkQueue);
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSemaphore);
VK_DEFINE_HANDLE(VkCommandBuffer);
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkFence);
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDeviceMemory);
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkBuffer);
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkImage);
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkEvent);
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkQueryPool);
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkBufferView);
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkImageView);
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkShaderModule);
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkPipelineCache);
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkPipelineLayout)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkRenderPass);
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkPipeline);
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDescriptorSetLayout);
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSampler);
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDescriptorPool);
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDescriptorSet);
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkFramebuffer);
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkCommandPool);

namespace alimer
{
    struct VulkanDeviceFeatures
    {
        /// VK_KHR_get_surface_capabilities2
        bool surfaceCapabilities2 = false;

        /// VK_KHR_get_physical_device_properties2
        bool physicalDeviceProperties2 = false;
        /// VK_KHR_external_memory_capabilities + VK_KHR_external_semaphore_capabilities
        bool external = false;
        /// VK_EXT_debug_utils
        bool debugUtils = false;
    };
}

#elif defined(ALIMER_D3D12)
#endif
