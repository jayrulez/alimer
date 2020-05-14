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

#include "SyncPrimitivesPool.h"
#include "GraphicsDeviceVK.h"
#include "core/Assert.h"
#include "core/Log.h"

using namespace std;

namespace alimer
{
    SyncPrimitivesPool::SyncPrimitivesPool(GraphicsDeviceVK& device)
        : device{ device }
    {

    }

    SyncPrimitivesPool::~SyncPrimitivesPool()
    {
        Wait();
        Reset();

        // Destroy all semaphores
        for (VkSemaphore semaphore : semaphores)
        {
            vkDestroySemaphore(device.GetHandle(), semaphore, nullptr);
        }

        // Destroy all fences
        for (VkFence fence : fences)
        {
            vkDestroyFence(device.GetHandle(), fence, nullptr);
        }

        semaphores.clear();
        fences.clear();
    }

    void SyncPrimitivesPool::Reset()
    {
        activeSemaphoreCount = 0;

        if (activeFenceCount < 1 || fences.empty())
        {
            return;
        }

        VkResult result = vkResetFences(device.GetHandle(), activeFenceCount, fences.data());

        if (result != VK_SUCCESS)
        {
            return;
        }

        activeFenceCount = 0;
    }

    VkResult SyncPrimitivesPool::Wait(uint32_t timeout) const
    {
        if (activeFenceCount < 1 || fences.empty())
        {
            return VK_SUCCESS;
        }

        return vkWaitForFences(device.GetHandle(), activeFenceCount, fences.data(), true, timeout);
    }

    VkFence SyncPrimitivesPool::RequestFence()
    {
        // Check if there is an available fence
        if (activeFenceCount < fences.size())
        {
            return fences.at(activeFenceCount++);
        }

        VkFenceCreateInfo createInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };

        VkFence fence;
        VkResult result = vkCreateFence(device.GetHandle(), &createInfo, nullptr, &fence);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create fence.");
            return VK_NULL_HANDLE;
        }

        fences.push_back(fence);
        activeFenceCount++;
        return fences.back();
    }

    VkSemaphore SyncPrimitivesPool::RequestSemaphore()
    {
        // Check if there is an available semaphore
        if (activeSemaphoreCount < semaphores.size())
        {
            return semaphores.at(activeSemaphoreCount++);
        }

        VkSemaphoreCreateInfo createInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

        VkSemaphore semaphore;
        VkResult result = vkCreateSemaphore(device.GetHandle(), &createInfo, nullptr, &semaphore);

        if (result != VK_SUCCESS)
        {
            VK_THROW(result, "Failed to create semaphore.");
            return VK_NULL_HANDLE;
        }

        semaphores.push_back(semaphore);
        activeSemaphoreCount++;
        return semaphore;
    }
}
