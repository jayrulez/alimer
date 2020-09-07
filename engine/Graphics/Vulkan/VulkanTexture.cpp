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

#include "VulkanTexture.h"
#include "VulkanGraphicsDevice.h"

namespace Alimer
{
    VulkanTexture::VulkanTexture(VulkanGraphicsDevice* device, const TextureDescription& desc, VkImage handle_, VkImageLayout layout_)
        : Texture(device, desc)
        , layout(layout_)
    {
        if (handle_ != VK_NULL_HANDLE)
        {
            handle = handle_;
            allocation = VK_NULL_HANDLE;
        }
        else
        {
            VkImageCreateInfo createInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
            createInfo.flags = 0u;
            createInfo.imageType = VK_IMAGE_TYPE_2D;
            createInfo.format = VK_FORMAT_R8G8B8A8_UNORM; // GetVkFormat(desc->format);
            createInfo.extent.width = desc.width;
            createInfo.extent.height = desc.height;
            createInfo.extent.depth = desc.depth;
            createInfo.mipLevels = desc.mipLevels;
            createInfo.arrayLayers = desc.arrayLayers;
            createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT; // vgpu_vkGetImageUsage(desc->usage, desc->format);
            createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
            createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            VmaAllocationCreateInfo allocCreateInfo = {};
            allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            VkResult result = vmaCreateImage(device->GetAllocator(),
                &createInfo, &allocCreateInfo,
                &handle, &allocation,
                nullptr);

            if (result != VK_SUCCESS)
            {
                LOGE("Vulkan: Failed to create image");
            }
        }
    }

    VulkanTexture::~VulkanTexture()
    {
        Destroy();
    }

    void VulkanTexture::Destroy()
    {
        if (handle != VK_NULL_HANDLE
            && allocation != VK_NULL_HANDLE)
        {
            //unmap();
            //vmaDestroyImage(device->GetAllocator(), handle, allocation);
        }
    }

    void VulkanTexture::BackendSetName()
    {
        GetVulkanGraphicsDevice()->SetObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)handle, name);
    }

    VulkanGraphicsDevice* VulkanTexture::GetVulkanGraphicsDevice() const
    {
        return static_cast<VulkanGraphicsDevice*>(device.Get());
    }
}
