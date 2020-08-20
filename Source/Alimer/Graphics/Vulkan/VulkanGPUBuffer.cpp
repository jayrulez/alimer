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

#include "VulkanGPUBuffer.h"
#include "VulkanGPUDevice.h"

namespace alimer
{
    namespace
    {
        VkBufferUsageFlags VulkanBufferUsage(GPUBufferUsage usage)
        {
            VkBufferUsageFlags flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            if (any(usage & GPUBufferUsage::Vertex)) {
                flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            }
            if (any(usage & GPUBufferUsage::Index)) {
                flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            }
            if (any(usage & GPUBufferUsage::Uniform)) {
                flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            }
            if (any(usage & GPUBufferUsage::Storage)) {
                flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            }
            if (any(usage & GPUBufferUsage::Indirect)) {
                flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
            }
            return flags;
        }
    }

    VulkanGPUBuffer::VulkanGPUBuffer(VulkanGPUDevice* device_, const GPUBufferDescriptor& descriptor)
        : GPUBuffer(descriptor)
        , device(device_)
    {
        VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        createInfo.size = descriptor.size;
        createInfo.usage = VulkanBufferUsage(descriptor.usage);
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;

        VmaAllocationCreateInfo memoryInfo{};
        memoryInfo.flags = 0u;
        memoryInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        VmaAllocationInfo allocationInfo{};
        VkResult result = vmaCreateBuffer(device->GetAllocator(),
            &createInfo, &memoryInfo,
            &handle, &allocation,
            &allocationInfo);
    }

    VulkanGPUBuffer::~VulkanGPUBuffer()
    {
        Destroy();
    }

    void VulkanGPUBuffer::Destroy()
    {
        if (handle != VK_NULL_HANDLE
            && allocation != VK_NULL_HANDLE)
        {
            //unmap();
            vmaDestroyBuffer(device->GetAllocator(), handle, allocation);
        }
    }

    void VulkanGPUBuffer::BackendSetName()
    {
        device->SetObjectName(VK_OBJECT_TYPE_BUFFER, (uint64_t)handle, name);
    }
}
