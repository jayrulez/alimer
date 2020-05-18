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

#if TODO_VK
#include "CommandBufferVK.h"
#include "CommandPoolVK.h"
#include "CommandQueueVK.h"
#include "GraphicsDeviceVK.h"
#include "SwapChainVK.h"
#include "TextureVK.h"
#include "core/Assert.h"
#include "core/Log.h"

namespace alimer
{
    CommandBufferVK::CommandBufferVK(CommandQueueVK* queue, CommandPoolVK* pool, VkCommandBufferLevel level)
        : queue{ queue }
        , pool{ pool }
        , handle{ handle }
    {
        VkCommandBufferAllocateInfo allocateIinfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocateIinfo.commandPool = pool->GetHandle();
        allocateIinfo.level = level;
        allocateIinfo.commandBufferCount = 1;

        VkResult result = vkAllocateCommandBuffers(pool->GetDevice()->GetHandle(), &allocateIinfo, &handle);
        if (result != VK_SUCCESS)
        {
            VK_THROW(result, "Failed to allocate command buffer");
        }
    }

    CommandBufferVK::~CommandBufferVK()
    {
        // Destroy command buffer
        if (handle != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers(pool->GetDevice()->GetHandle(), pool->GetHandle(), 1, &handle);
        }

        handle = VK_NULL_HANDLE;
    }

    void CommandBufferVK::Begin() const
    {
        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VkResult result = vkBeginCommandBuffer(handle, &beginInfo);
        if (result != VK_SUCCESS)
        {
            VK_THROW(result, "[CommandBufferVK]: Begin CommandBuffer failed");
        }
    }

    void CommandBufferVK::End() const
    {
        VkResult result = vkEndCommandBuffer(handle);
        if (result != VK_SUCCESS)
        {
            VK_THROW(result, "[CommandBufferVK]: End CommandBuffer failed");
        }
    }

    void CommandBufferVK::TextureBarrier(TextureVK* texture, TextureState newState)
    {
        texture->Barrier(handle, newState);
    }

    void CommandBufferVK::Present(ISwapChain* swapChain)
    {
        auto vkSwapChain = static_cast<SwapChainVK*>(swapChain);

        TextureBarrier(vkSwapChain->GetCurrentTexture(), TextureState::Present);
        queue->Present(vkSwapChain->GetHandle(), vkSwapChain->GetCurrentBackBufferIndex());
    }
}
#endif // TODO_VK

