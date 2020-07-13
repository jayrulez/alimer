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

#if defined(ALIMER_D3D12)
#include "Graphics/D3D12/D3D12GraphicsImpl.h"
#elif defined(ALIMER_VULKAN)
#include "Graphics/Vulkan/VulkanGraphicsImpl.h"
#endif

#include "imgui_impl_glfw.h"

namespace alimer
{
    Graphics* Graphics::Instance = nullptr;

    Graphics::Graphics(Window* window)
    {
        backbufferWidth = Max(window->GetSize().width, 1);
        backbufferHeight = Max(window->GetSize().height, 1);
    }

    bool Graphics::Initialize(Window* window, bool enableDebugLayer, BackendType backendType)
    {
        ALIMER_ASSERT(window);

        if (Instance != nullptr) {
            LOG_WARN("Cannot create more than one Graphics instance");
            return true;
        }

        Instance = new D3D12GraphicsImpl(window, enableDebugLayer);
        return true;
    }

    void Graphics::Shutdown()
    {
        if (Instance != nullptr)
        {
            delete Instance;
            Instance = nullptr;
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
