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

#pragma once

#include "graphics/Texture.h"
#include "VulkanBackend.h"

namespace alimer
{
    class ALIMER_API TextureVK final : public Texture
    {
    public:
        TextureVK(GraphicsDeviceVK* device);
        ~TextureVK() override;

        bool Init(const TextureDesc* pDesc, const void* initialData);
        void InitExternal(VkImage image, const TextureDesc* pDesc);

        void Destroy() override;
        void Barrier(VkCommandBuffer commandBuffer, TextureState newState);

        ALIMER_FORCEINLINE const TextureDesc& GetDesc() const override { return desc; }

        TextureState GetState() const { return state; }

    private:
        TextureDesc desc;

        VkImage handle = VK_NULL_HANDLE;
        VkFormat vkFormat = VK_FORMAT_UNDEFINED;
        VmaAllocation allocation = VK_NULL_HANDLE;
        TextureState state = TextureState::Undefined;
    };
}