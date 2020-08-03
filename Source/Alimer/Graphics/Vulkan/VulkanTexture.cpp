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
#include "VulkanGraphicsImpl.h"

namespace alimer
{
    namespace
    {
    }

    VulkanTexture::VulkanTexture(VulkanGraphicsImpl* device_, VkImage resource_)
        : Texture()
        , device(device_)
        , handle(resource_)
    {

    }

    VulkanTexture::VulkanTexture(VulkanGraphicsImpl* device_, const TextureDescription& desc, const void* initialData)
        : Texture(desc)
        , device(device_)
    {

    }

    VulkanTexture::~VulkanTexture()
    {
        Destroy();
    }

    void VulkanTexture::Destroy()
    {
        if (handle != VK_NULL_HANDLE && memory != VK_NULL_HANDLE)
        {
            //Unmap();
            vmaDestroyImage(device->GetMemoryAllocator(), handle, memory);
        }
    }

    void VulkanTexture::BackendSetName()
    {
        device->SetObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)handle, name);
    }
}