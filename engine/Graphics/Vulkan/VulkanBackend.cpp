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

#include "VulkanBackend.h"
#include "VulkanCommandPool.h"
#include "VulkanCommandBuffer.h"
#include "VulkanGraphicsDevice.h"
#include <algorithm>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

namespace Alimer
{
    const VkFormatDesc kVkFormatDesc[] =
    {
        {PixelFormat::Invalid,                      VK_FORMAT_UNDEFINED},
        // 8-bit pixel formats
        {PixelFormat::R8Unorm,                      VK_FORMAT_R8_UNORM},
        {PixelFormat::R8Snorm,                      VK_FORMAT_R8_SNORM},
        {PixelFormat::R8Uint,                       VK_FORMAT_R8_UINT},
        {PixelFormat::R8Sint,                       VK_FORMAT_R8_SINT},
        // 16-bit pixel formats
        {PixelFormat::R16Unorm,                     VK_FORMAT_R16_UNORM},
        {PixelFormat::R16Snorm,                     VK_FORMAT_R16_SNORM},
        {PixelFormat::R16Uint,                      VK_FORMAT_R16_UINT},
        {PixelFormat::R16Sint,                      VK_FORMAT_R16_SINT},
        {PixelFormat::R16Float,                     VK_FORMAT_R16_SFLOAT},
        {PixelFormat::RG8Unorm,                     VK_FORMAT_R8G8_UNORM},
        {PixelFormat::RG8Snorm,                     VK_FORMAT_R8G8_SNORM},
        {PixelFormat::RG8Uint,                      VK_FORMAT_R8G8_UINT},
        {PixelFormat::RG8Sint,                      VK_FORMAT_R8G8_SINT},
        // 32-bit pixel formats
        {PixelFormat::R32Uint,                      VK_FORMAT_R32_UINT},
        {PixelFormat::R32Sint,                      VK_FORMAT_R32_SINT},
        {PixelFormat::R32Float,                     VK_FORMAT_R32_SFLOAT},
        {PixelFormat::RG16Unorm,                    VK_FORMAT_R16G16_UNORM},
        {PixelFormat::RG16Snorm,                    VK_FORMAT_R16G16_SNORM},
        {PixelFormat::RG16Uint,                     VK_FORMAT_R16G16_UINT},
        {PixelFormat::RG16Sint,                     VK_FORMAT_R16G16_SINT},
        {PixelFormat::RG16Float,                    VK_FORMAT_R16G16_SFLOAT},
        {PixelFormat::RGBA8Unorm,                   VK_FORMAT_R8G8B8A8_UNORM},
        {PixelFormat::RGBA8UnormSrgb,               VK_FORMAT_R8G8B8A8_SRGB},
        {PixelFormat::RGBA8Snorm,                   VK_FORMAT_R8G8B8A8_SNORM},
        {PixelFormat::RGBA8Uint,                    VK_FORMAT_R8G8B8A8_UINT},
        {PixelFormat::RGBA8Sint,                    VK_FORMAT_R8G8B8A8_SINT},
        {PixelFormat::BGRA8Unorm,                   VK_FORMAT_B8G8R8A8_UNORM},
        {PixelFormat::BGRA8UnormSrgb,               VK_FORMAT_B8G8R8A8_SRGB},
        // Packed 32-Bit Pixel formats
        {PixelFormat::RGB10A2Unorm,                 VK_FORMAT_A2B10G10R10_UNORM_PACK32},
        {PixelFormat::RG11B10Float,                 VK_FORMAT_B10G11R11_UFLOAT_PACK32},
        {PixelFormat::RGB9E5Float,                  VK_FORMAT_E5B9G9R9_UFLOAT_PACK32},
        // 64-Bit Pixel Formats
        {PixelFormat::RG32Uint,                     VK_FORMAT_R32G32_UINT},
        {PixelFormat::RG32Sint,                     VK_FORMAT_R32G32_SINT},
        {PixelFormat::RG32Float,                    VK_FORMAT_R32G32_SFLOAT},
        {PixelFormat::RGBA16Unorm,                  VK_FORMAT_R16G16B16A16_UNORM},
        {PixelFormat::RGBA16Snorm,                  VK_FORMAT_R16G16B16A16_SNORM},
        {PixelFormat::RGBA16Uint,                   VK_FORMAT_R16G16B16A16_UINT},
        {PixelFormat::RGBA16Sint,                   VK_FORMAT_R16G16B16A16_SINT},
        {PixelFormat::RGBA16Float,                  VK_FORMAT_R16G16B16A16_SFLOAT},
        // 128-Bit Pixel Formats
        {PixelFormat::RGBA32Uint,                   VK_FORMAT_R32G32B32A32_UINT},
        {PixelFormat::RGBA32Sint,                   VK_FORMAT_R32G32B32A32_SINT},
        {PixelFormat::RGBA32Float,                  VK_FORMAT_R32G32B32A32_SFLOAT},
        // Depth-stencil formats
        {PixelFormat::Depth16Unorm,                 VK_FORMAT_D16_UNORM},
        {PixelFormat::Depth32Float,                 VK_FORMAT_D32_SFLOAT},
        {PixelFormat::Depth24UnormStencil8,         VK_FORMAT_D24_UNORM_S8_UINT},
        // Compressed BC formats
        {PixelFormat::BC1RGBAUnorm,                 VK_FORMAT_BC1_RGB_UNORM_BLOCK},
        {PixelFormat::BC1RGBAUnormSrgb,             VK_FORMAT_BC1_RGB_SRGB_BLOCK},
        {PixelFormat::BC2RGBAUnorm,                 VK_FORMAT_BC2_UNORM_BLOCK},
        {PixelFormat::BC2RGBAUnormSrgb,             VK_FORMAT_BC2_SRGB_BLOCK},
        {PixelFormat::BC3RGBAUnorm,                 VK_FORMAT_BC3_UNORM_BLOCK},
        {PixelFormat::BC3RGBAUnormSrgb,             VK_FORMAT_BC3_SRGB_BLOCK},
        {PixelFormat::BC4RUnorm,                    VK_FORMAT_BC4_UNORM_BLOCK},
        {PixelFormat::BC4RSnorm,                    VK_FORMAT_BC4_SNORM_BLOCK},
        {PixelFormat::BC5RGUnorm,                   VK_FORMAT_BC5_UNORM_BLOCK},
        {PixelFormat::BC5RGSnorm,                   VK_FORMAT_BC5_SNORM_BLOCK},
        {PixelFormat::BC6HRGBUfloat,                VK_FORMAT_BC6H_UFLOAT_BLOCK },
        {PixelFormat::BC6HRGBSfloat,                VK_FORMAT_BC6H_SFLOAT_BLOCK },
        {PixelFormat::BC7RGBAUnorm,                 VK_FORMAT_BC7_UNORM_BLOCK},
        {PixelFormat::BC7RGBAUnormSrgb,             VK_FORMAT_BC7_SRGB_BLOCK},
    };
    static_assert((unsigned)PixelFormat::Count == ALIMER_STATIC_ARRAY_SIZE(kVkFormatDesc), "Missmatch PixelFormat size");

    const char* ToString(VkResult result)
    {
        switch (result)
        {
#define STR(r)   \
	case VK_##r: \
		return #r
            STR(NOT_READY);
            STR(TIMEOUT);
            STR(EVENT_SET);
            STR(EVENT_RESET);
            STR(INCOMPLETE);
            STR(ERROR_OUT_OF_HOST_MEMORY);
            STR(ERROR_OUT_OF_DEVICE_MEMORY);
            STR(ERROR_INITIALIZATION_FAILED);
            STR(ERROR_DEVICE_LOST);
            STR(ERROR_MEMORY_MAP_FAILED);
            STR(ERROR_LAYER_NOT_PRESENT);
            STR(ERROR_EXTENSION_NOT_PRESENT);
            STR(ERROR_FEATURE_NOT_PRESENT);
            STR(ERROR_INCOMPATIBLE_DRIVER);
            STR(ERROR_TOO_MANY_OBJECTS);
            STR(ERROR_FORMAT_NOT_SUPPORTED);
            STR(ERROR_SURFACE_LOST_KHR);
            STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
            STR(SUBOPTIMAL_KHR);
            STR(ERROR_OUT_OF_DATE_KHR);
            STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
            STR(ERROR_VALIDATION_FAILED_EXT);
            STR(ERROR_INVALID_SHADER_NV);
#undef STR
        default:
            return "UNKNOWN_ERROR";
        }
    }

    /* VulkanFencePool */
    VulkanFencePool::VulkanFencePool(VkDevice device)
        : device{ device }
    {
    }

    VulkanFencePool::~VulkanFencePool()
    {
        Wait();
        Reset();

        // Destroy all fences
        for (VkFence fence : fences)
        {
            vkDestroyFence(device, fence, nullptr);
        }

        fences.clear();
    }

    VkFence VulkanFencePool::RequestFence()
    {
        // Check if there is an available fence
        if (activeFenceCount < fences.size())
        {
            return fences.at(activeFenceCount++);
        }

        VkFenceCreateInfo createInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        VkFence fence{ VK_NULL_HANDLE };
        VkResult result = vkCreateFence(device, &createInfo, nullptr, &fence);
        if (result != VK_SUCCESS)
        {
            LOGE("Vulkan: Failed to create fence.");
        }

        fences.push_back(fence);
        activeFenceCount++;
        return fences.back();
    }

    VkResult VulkanFencePool::Wait(uint32_t timeout) const
    {
        if (activeFenceCount < 1 || fences.empty())
        {
            return VK_SUCCESS;
        }

        return vkWaitForFences(device, activeFenceCount, fences.data(), VK_TRUE, timeout);
    }

    VkResult VulkanFencePool::Reset()
    {
        if (activeFenceCount < 1 || fences.empty())
        {
            return VK_SUCCESS;
        }

        VkResult result = vkResetFences(device, activeFenceCount, fences.data());
        if (result != VK_SUCCESS)
        {
            return result;
        }

        activeFenceCount = 0;
        return VK_SUCCESS;
    }

    /* VulkanSemaphorePool */
    VulkanSemaphorePool::VulkanSemaphorePool(VkDevice device)
        : device{ device }
    {
    }

    VulkanSemaphorePool::~VulkanSemaphorePool()
    {
        Reset();

        // Destroy all semaphores
        for (VkSemaphore semaphore : semaphores)
        {
            vkDestroySemaphore(device, semaphore, nullptr);
        }

        semaphores.clear();
    }

    VkSemaphore VulkanSemaphorePool::RequestSemaphore()
    {
        // Check if there is an available semaphore
        if (activeSemaphoreCount < semaphores.size())
        {
            return semaphores.at(activeSemaphoreCount++);
        }


        VkSemaphoreCreateInfo createInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VkSemaphore semaphore{ VK_NULL_HANDLE };
        VkResult result = vkCreateSemaphore(device, &createInfo, nullptr, &semaphore);
        if (result != VK_SUCCESS)
        {
            LOGE("Vulkan: Failed to create semaphore.");
        }

        semaphores.push_back(semaphore);
        activeSemaphoreCount++;
        return semaphore;
    }

    void VulkanSemaphorePool::Reset()
    {
        activeSemaphoreCount = 0;
    }

    uint32_t VulkanSemaphorePool::GetActiveSemaphoreCount() const
    {
        return activeSemaphoreCount;
    }

    /* VulkanRenderFrame */
    VulkanRenderFrame::VulkanRenderFrame(VulkanGraphicsDevice& device)
        : device{ device }
        , fencePool{device.GetHandle()}
        , semaphorePool{ device.GetHandle() }
    {
        commandPool = std::make_unique<VulkanCommandPool>(device, device.GetGraphicsQueueFamilyIndex());
    }

    void VulkanRenderFrame::Reset()
    {
        VK_CHECK(fencePool.Wait());
        fencePool.Reset();
        commandPool->Reset();

        semaphorePool.Reset();
    }

    VkFence VulkanRenderFrame::RequestFence()
    {
        return fencePool.RequestFence();
    }

    VkSemaphore VulkanRenderFrame::RequestSemaphore()
    {
        return semaphorePool.RequestSemaphore();
    }

    CommandBuffer* VulkanRenderFrame::RequestCommandBuffer()
    {
        return commandPool->RequestCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    }
}
