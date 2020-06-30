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
#include "Graphics/GraphicsDevice.h"
#include "Graphics/Texture.h"
#include "imgui_impl_glfw.h"

#if defined(ALIMER_VULKAN)
#include "Graphics/Vulkan/VulkanGraphicsDevice.h"
#endif

#if defined(ALIMER_D3D12)
#include "Graphics/D3D12/D3D12GraphicsDevice.h"
#endif

namespace alimer
{
    GraphicsDevice::GraphicsDevice(const Desc& desc)
        : desc{ desc }
        , colorFormat{ desc.colorFormat }
        , depthStencilFormat{ desc.depthStencilFormat }
    {
       
    }

    std::set<BackendType> GraphicsDevice::GetAvailableBackends()
    {
        static std::set<BackendType> availableDrivers;

        if (availableDrivers.empty())
        {
            availableDrivers.insert(BackendType::Null);

#if defined(ALIMER_D3D12)
            if (D3D12GraphicsDevice::IsAvailable())
            {
                availableDrivers.insert(BackendType::Direct3D12);
            }
#endif

#if defined(ALIMER_VULKAN)
            if (VulkanGraphicsDevice::IsAvailable())
            {
                availableDrivers.insert(BackendType::Vulkan);
            }
#endif
        }

        return availableDrivers;
    }

    GraphicsDevice* GraphicsDevice::Create(WindowHandle window, const Desc& desc)
    {
        BackendType backend = BackendType::Count;

        if (backend == BackendType::Count)
        {
            auto availableDrivers = GetAvailableBackends();

            if (availableDrivers.find(BackendType::Direct3D12) != availableDrivers.end())
                backend = BackendType::Direct3D12;
            else if (availableDrivers.find(BackendType::Vulkan) != availableDrivers.end())
                backend = BackendType::Vulkan;
            else
                backend = BackendType::Null;
        }

        GraphicsDevice* device = nullptr;
        switch (backend)
        {
#if defined(ALIMER_VULKAN)
        case BackendType::Vulkan:
            if (VulkanGraphicsDevice::IsAvailable())
            {
                device = new VulkanGraphicsDevice(window, desc);
                LOG_INFO("Using Vulkan driver");
            }
            break;
#endif
#if defined(ALIMER_D3D12)
        case BackendType::Direct3D12:
            if (D3D12GraphicsDevice::IsAvailable())
            {
                device = new D3D12GraphicsDevice(window, desc);
                LOG_INFO("Using Direct3D12 driver");
            }
            break;
#endif

        default:
            // TODO: Create Null backend
            break;
        }

        return device;
    }

    void GraphicsDevice::BeginFrame()
    {
        ALIMER_ASSERT_MSG(!frameActive, "Frame is still active, please call EndFrame first");

        if (!BeginFrameImpl()) {
            return;
        }

        ImGuiIO& io = ImGui::GetIO();
        IM_ASSERT(io.Fonts->IsBuilt());

        // Start the Dear ImGui frame
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Now the frame is active again.
        frameActive = true;
    }

    void GraphicsDevice::EndFrame()
    {
        ALIMER_ASSERT_MSG(frameActive, "Frame is not active, please call BeginFrame first.");

        ImGui::Render();
        EndFrameImpl();

        // Update and Render additional Platform Windows
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault(nullptr, nullptr);
        }

        // Frame is not active anymore
        frameActive = false;
    }

    void GraphicsDevice::Resize(uint32_t width, uint32_t height)
    {
        if ((width != size.width || height != size.height) && width > 0 && height > 0)
        {
            size.width = float(width);
            size.height = float(height);
        }
    }

    void GraphicsDevice::TrackResource(GraphicsResource* resource)
    {
        std::lock_guard<std::mutex> lock(trackedResourcesMutex);
        trackedResources.Push(resource);
    }

    void GraphicsDevice::UntrackResource(GraphicsResource* resource)
    {
        std::lock_guard<std::mutex> lock(trackedResourcesMutex);
        trackedResources.Remove(resource);
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

            trackedResources.Clear();
        }
    }

    Texture* GraphicsDevice::GetBackbufferTexture() const
    {
        return backbufferTextures[backbufferIndex].Get();
    }

    Texture* GraphicsDevice::GetDepthStencilTexture() const
    {
        return depthStencilTexture.Get();
    }
}
