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

#include "config.h"
#include "Core/Log.h"
#include "Core/Assert.h"
#include "Core/Window.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/Texture.h"

#if defined(ALIMER_VULKAN)
#include "Graphics/Vulkan/VulkanGraphicsImpl.h"
#endif

#if defined(ALIMER_D3D12)
#include "Graphics/D3D12/D3D12GraphicsDevice.h"
#endif

namespace alimer
{
    GraphicsDevice* Graphics = nullptr;

#if defined(ALIMER_D3D12)
    GPUBackendType GraphicsDevice::preferredBackend = GPUBackendType::Direct3D12;
#elif defined(ALIMER_VULKAN)
    GPUBackendType GraphicsDevice::preferredBackend = GPUBackendType::Vulkan;
#else
    GPUBackendType GraphicsDevice::preferredBackend = GPUBackendType::Null;
#endif

    void GraphicsDevice::SetPreferredBackend(GPUBackendType backend)
    {
        preferredBackend = backend;
    }

    bool GraphicsDevice::Initialize(const std::string& applicationName, Window* window, GPUFlags flags)
    {
        if (Graphics != nullptr) {
            LOGW("Cannot create more than one Graphics instance");
            return true;
        }

        GPUBackendType backend = preferredBackend;

        if (backend == GPUBackendType::Count)
        {
#if defined(ALIMER_D3D12)
            if (D3D12GraphicsDevice::IsAvailable()) {
                backend = GPUBackendType::Direct3D12;
            }
#endif

#if defined(ALIMER_VULKAN)
            if (backend == GPUBackendType::Count && VulkanGraphicsImpl::IsAvailable()) {
                backend = GPUBackendType::Vulkan;
            }
#endif
        }

        switch (backend)
        {
#if defined(ALIMER_VULKAN)
        case GPUBackendType::Vulkan:
            if (VulkanGraphicsImpl::IsAvailable()) {
                Graphics = new VulkanGraphicsImpl(applicationName, flags);
            }
            break;
#endif

#if defined(ALIMER_D3D12)
        case GPUBackendType::Direct3D12:
            if (D3D12GraphicsDevice::IsAvailable()) {
                Graphics = new D3D12GraphicsDevice(window, flags);
            }
            break;
#endif

        default:
            break;
        }

        return Graphics != nullptr;
    }

    void GraphicsDevice::Shutdown()
    {
        if (Graphics != nullptr)
        {
            delete Graphics;
            Graphics = nullptr;
        }
    }

    void GraphicsDevice::TrackResource(GraphicsResource* resource)
    {
        std::lock_guard<std::mutex> lock(trackedResourcesMutex);
        trackedResources.push_back(resource);
    }

    void GraphicsDevice::UntrackResource(GraphicsResource* resource)
    {
        std::lock_guard<std::mutex> lock(trackedResourcesMutex);

        auto it = std::find(trackedResources.begin(), trackedResources.end(), resource);
        if (it != trackedResources.end()) {
            trackedResources.erase(it);
        }
    }

    void GraphicsDevice::ReleaseTrackedResources()
    {
        {
            std::lock_guard<std::mutex> lock(trackedResourcesMutex);

            // Release all GPU objects that still exist
            for (auto resource : trackedResources)
            {
                resource->Release();
            }

            trackedResources.clear();
        }
    }
}
