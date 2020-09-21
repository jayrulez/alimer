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

#if defined(ALIMER_ENABLE_D3D11)
#   include "Graphics/D3D11/D3D11GraphicsDevice.h"
#endif
#if defined(ALIMER_ENABLE_D3D12) && defined(TODO_D3D12)
#   include "Graphics/D3D12/D3D12GraphicsDevice.h"
#endif
#if defined(ALIMER_ENABLE_VULKAN) && defined(TODO_VK)
#   include "Graphics/Vulkan/VulkanGraphicsDevice.h"
#endif

namespace Alimer
{
    GraphicsDevice* GraphicsDevice::Instance = nullptr;

    GraphicsDevice::GraphicsDevice()
    {
        RegisterSubsystem(this);
    }

    std::set<GPUBackendType> GraphicsDevice::GetAvailableBackends()
    {
        static std::set<GPUBackendType> availableBackends;

        if (availableBackends.empty())
        {
            availableBackends.insert(GPUBackendType::Null);

#if defined(ALIMER_ENABLE_D3D11)
            if (D3D11GraphicsDevice::IsAvailable())
                availableBackends.insert(GPUBackendType::Direct3D12);
#endif
#if defined(ALIMER_ENABLE_D3D12) && defined(TODO_D3D12)
            if (D3D12GraphicsDevice::IsAvailable())
                availableBackends.insert(GPUBackendType::Direct3D12);
#endif
#if defined(ALIMER_ENABLE_VULKAN) && defined(TODO_VK)
            if (VulkanGraphicsDevice::IsAvailable())
                availableBackends.insert(GPUBackendType::Vulkan);
#endif
        }

        return availableBackends;
    }

    GraphicsDevice* GraphicsDevice::Create(GPUBackendType preferredBackend, const GraphicsDeviceDescription& desc)
    {
        if (Instance != nullptr)
        {
            LOGW("Cannot create more than one instance of GraphicsDevice");
            return Instance;
        }

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
#if defined(ALIMER_ENABLE_D3D11) 
        case GPUBackendType::Direct3D11:
            if (D3D11GraphicsDevice::IsAvailable())
            {
                Instance = new D3D11GraphicsDevice(nullptr, desc);
            }
            else
            {
                LOGE("Direct3D11 backend is not supported on this platform");
                return nullptr;
            }
#endif

#if defined(ALIMER_ENABLE_D3D12) && defined(TODO_D3D12)
        case GPUBackendType::Direct3D12:
            if (D3D12GraphicsDevice::IsAvailable())
            {
                return new D3D12GraphicsDevice(desc);
            }

            LOGE("Direct3D12 backend is not supported on this platform");
            return nullptr;
#endif

#if defined(ALIMER_ENABLE_VULKAN) && defined(TODO_VK)
        case GPUBackendType::Vulkan:
            if (VulkanGraphicsDevice::IsAvailable())
            {
                return new VulkanGraphicsDevice(desc);
            }

            LOGE("Vulkan backend is not supported on this platform");
            return nullptr;
#endif

        default:
            break;
        }

        if (Instance == nullptr)
            return nullptr;

        LOGI("Successfully created {} GraphicsDevice", ToString(Instance->GetCaps().backendType));
        return Instance;
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

    RefPtr<GPUBuffer> GraphicsDevice::CreateBuffer(const BufferDescription& desc, const void* initialData)
    {
        // TODO: Validate
        return nullptr;
        //return CreateBufferCore(desc, initialData);
    }
}

