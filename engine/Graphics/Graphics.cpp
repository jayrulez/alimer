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
#include "Graphics/Graphics.h"
#include "Graphics/Texture.h"

#if defined(ALIMER_VULKAN)
#include "Graphics/Vulkan/VulkanGraphicsImpl.h"
#endif

#if defined(ALIMER_D3D12)
#include "Graphics/D3D12/D3D12GraphicsImpl.h"
#endif

#include "imgui_impl_glfw.h"

namespace GPU
{
#if defined(ALIMER_D3D12)
    static BackendType s_BackendType = BackendType::Direct3D12;
#elif defined(ALIMER_VULKAN)
    static BackendType s_BackendType = BackendType::Vulkan;
#else
    static BackendType s_BackendType = BackendType::Null;
#endif

    void SetPreferredBackend(BackendType backend)
    {
        s_BackendType = backend;
    }
}

namespace alimer
{
    Graphics* GPU = nullptr;

    Graphics::Graphics(Window* window)
    {
        backbufferWidth = Max((uint32_t)window->GetSize().width, 1u);
        backbufferHeight = Max((uint32_t)window->GetSize().height, 1u);
    }

    bool Graphics::Initialize(Window* window, GPUFlags flags)
    {
        ALIMER_ASSERT(window);

        if (GPU != nullptr) {
            LOG_WARN("Cannot create more than one Graphics instance");
            return true;
        }

        GPU::BackendType backend = GPU::s_BackendType;

        if (backend == GPU::BackendType::Count)
        {
#if defined(ALIMER_D3D12)
            if (D3D12GraphicsImpl::IsAvailable()) {
                backend = GPU::BackendType::Direct3D12;
            }
#endif

#if defined(ALIMER_VULKAN)
            if (backend == GPU::BackendType::Count && VulkanGraphicsImpl::IsAvailable()) {
                backend = GPU::BackendType::Vulkan;
            }
#endif
        }

        switch (backend)
        {
#if defined(ALIMER_VULKAN)
        case GPU::BackendType::Vulkan:
            if (VulkanGraphicsImpl::IsAvailable()) {
                GPU = new VulkanGraphicsImpl(window, flags);
            }
            break;
#endif

#if defined(ALIMER_D3D12)
        case GPU::BackendType::Direct3D12:
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

    void Graphics::Shutdown()
    {
        if (GPU != nullptr)
        {
            delete GPU;
            GPU = nullptr;
        }
    }


    void Graphics::TrackResource(GraphicsResource* resource)
    {
        std::lock_guard<std::mutex> lock(trackedResourcesMutex);
        trackedResources.Push(resource);
    }

    void Graphics::UntrackResource(GraphicsResource* resource)
    {
        std::lock_guard<std::mutex> lock(trackedResourcesMutex);
        trackedResources.Remove(resource);
    }

    void Graphics::ReleaseTrackedResources()
    {
        {
            std::lock_guard<std::mutex> lock(trackedResourcesMutex);

            // Release all GPU objects that still exist
            for (auto resource : trackedResources)
            {
                resource->Release();
            }

            trackedResources.Clear();
        }
    }
}
