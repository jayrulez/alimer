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
#include "Graphics/D3D12/D3D12GraphicsImpl.h"
#endif

namespace alimer
{
    GraphicsDevice* GPU = nullptr;

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
        if (GPU != nullptr) {
            LOGW("Cannot create more than one Graphics instance");
            return true;
        }

        GPUBackendType backend = preferredBackend;

        if (backend == GPUBackendType::Count)
        {
#if defined(ALIMER_D3D12)
            if (D3D12GraphicsImpl::IsAvailable()) {
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
                GPU = new VulkanGraphicsImpl(applicationName, flags);
            }
            break;
#endif

#if defined(ALIMER_D3D12)
        case GPUBackendType::Direct3D12:
            if (D3D12GraphicsImpl::IsAvailable()) {
                GPU = new D3D12GraphicsImpl(window, flags);
            }
            break;
#endif

        default:
            break;
        }

        return GPU != nullptr;
    }

    void GraphicsDevice::Shutdown()
    {
        if (GPU != nullptr)
        {
            delete GPU; GPU = nullptr;
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

    void RegisterGraphicsLibrary()
    {
        static bool registered = false;
        if (registered)
            return;
        registered = true;

        Texture::RegisterObject();
    }
}
