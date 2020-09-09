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

#include "config.h"
#include "Core/Log.h"
#include "Math/MathHelper.h"
#include "Graphics/GraphicsDevice.h"

#if defined(ALIMER_ENABLE_D3D12)
#   include "Graphics/D3D12/D3D12GraphicsDevice.h"
#endif
#if defined(ALIMER_ENABLE_VULKAN)
#   include "Graphics/Vulkan/VulkanGraphicsDevice.h"
#endif

namespace Alimer
{
    GraphicsDevice::GraphicsDevice()
    {
    }

    std::set<GPUBackendType> GraphicsDevice::GetAvailableBackends()
    {
        static std::set<GPUBackendType> availableBackends;

        if (availableBackends.empty())
        {
            availableBackends.insert(GPUBackendType::Null);

#if defined(ALIMER_ENABLE_D3D12)
            if(D3D12GraphicsDevice::IsAvailable())
                availableBackends.insert(GPUBackendType::Direct3D12);
#endif
#if defined(ALIMER_ENABLE_VULKAN)
            if (VulkanGraphicsDevice::IsAvailable())
                availableBackends.insert(GPUBackendType::Vulkan);
#endif
        }

        return availableBackends;
    }

    RefPtr<GraphicsDevice> GraphicsDevice::Create(GPUBackendType preferredBackend, const GraphicsDeviceDescription& desc)
    {
        if (preferredBackend == GPUBackendType::Count)
        {
            const auto availableBackends = GetAvailableBackends();

            if (availableBackends.find(GPUBackendType::Direct3D12) != availableBackends.end())
                preferredBackend = GPUBackendType::Direct3D12;
            else if (availableBackends.find(GPUBackendType::Metal) != availableBackends.end())
                preferredBackend = GPUBackendType::Metal;
            else if (availableBackends.find(GPUBackendType::Vulkan) != availableBackends.end())
                preferredBackend = GPUBackendType::Vulkan;
            else
                preferredBackend = GPUBackendType::Null;
        }

        switch (preferredBackend)
        {
#if defined(ALIMER_ENABLE_D3D12)
        case GPUBackendType::Direct3D12:
            if (D3D12GraphicsDevice::IsAvailable())
            {
                return MakeRefPtr<D3D12GraphicsDevice>(desc);
            }

            LOGE("Direct3D12 backend is not supported on this platform");
            return nullptr;
#endif

#if defined(ALIMER_ENABLE_VULKAN)
        case GPUBackendType::Vulkan:
            if (VulkanGraphicsDevice::IsAvailable())
            {
                return MakeRefPtr<VulkanGraphicsDevice>(desc);
            }

            LOGE("Vulkan backend is not supported on this platform");
            return nullptr;
#endif

        default:
            break;
        }

        return nullptr;
    }

    void GraphicsDevice::AddGraphicsResource(GraphicsResource* resource)
    {
        ALIMER_ASSERT(resource);

        std::lock_guard<std::mutex> LockGuard(gpuObjectMutex);
        gpuObjects.push_back(resource);
    }

    void GraphicsDevice::RemoveGraphicsResource(GraphicsResource* resource)
    {
        ALIMER_ASSERT(resource);
        std::lock_guard<std::mutex> LockGuard(gpuObjectMutex);

        auto it = std::find(gpuObjects.begin(), gpuObjects.end(), resource);
        if (it != gpuObjects.end())
            gpuObjects.erase(it);
    }

    const GraphicsDeviceCaps& GraphicsDevice::GetCaps() const
    {
        return caps;
    }

    SwapChain* GraphicsDevice::GetPrimarySwapChain() const
    {
        return primarySwapChain.Get();
    }

    CommandQueue* GraphicsDevice::GetCommandQueue(CommandQueueType type) const
    {
        std::shared_ptr<CommandQueue> commandQueue;
        switch (type)
        {
        case CommandQueueType::Graphics:
            return graphicsCommandQueue.get();
        case CommandQueueType::Compute:
            return computeCommandQueue.get();
        case CommandQueueType::Copy:
            return copyCommandQueue.get();
        default:
            ALIMER_ASSERT_FAIL("Invalid command queue type.");
        }

        return nullptr;
    }
}

