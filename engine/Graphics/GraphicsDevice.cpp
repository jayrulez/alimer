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

#if defined(ALIMER_VULKAN)
#include "Graphics/Vulkan/VulkanGraphicsDevice.h"
#endif
#if defined(ALIMER_D3D12)
//#include "Graphics/D3D12/D3D12GraphicsDevice.h"
#endif
#if defined(ALIMER_D3D11)
#include "Graphics/D3D11/D3D11GraphicsDevice.h"
#endif

#include "imgui_impl_glfw.h"

namespace alimer
{
    BackendType GraphicsDevice::PreferredBackendType = BackendType::Count;
    GraphicsDevice* GraphicsDevice::Instance = nullptr;

    std::set<BackendType> GraphicsDevice::GetAvailableBackends()
    {
        static std::set<BackendType> availableDrivers;

        if (availableDrivers.empty())
        {
            availableDrivers.insert(BackendType::Null);

#if defined(ALIMER_VULKAN)
            if (VulkanGraphicsDevice::IsAvailable())
            {
                availableDrivers.insert(BackendType::Vulkan);
            }
#endif

#if defined(ALIMER_D3D12)
            //if (D3D12GraphicsDevice::IsAvailable())
            //    availableDrivers.insert(BackendType::Direct3D12);
#endif

#if defined(ALIMER_D3D11)
            availableDrivers.insert(BackendType::Direct3D11);
#endif
        }

        return availableDrivers;
    }

    void GraphicsDevice::Initialize()
    {
        if (Instance != nullptr) {
            return;
        }

        BackendType backendType = PreferredBackendType;
        if (PreferredBackendType == BackendType::Count)
        {
            auto availableDrivers = GetAvailableBackends();

            if (availableDrivers.find(BackendType::Direct3D12) != availableDrivers.end())
                backendType = BackendType::Direct3D12;
            else if (availableDrivers.find(BackendType::Direct3D11) != availableDrivers.end())
                backendType = BackendType::Direct3D11;
            else if (availableDrivers.find(BackendType::Vulkan) != availableDrivers.end())
                backendType = BackendType::Vulkan;
            else
                backendType = BackendType::Null;
        }

        switch (backendType)
        {
#if defined(ALIMER_VULKAN)
        case BackendType::Vulkan:
            if (VulkanGraphicsDevice::IsAvailable())
            {
                Instance = new VulkanGraphicsDevice();
            }
            break;
#endif

            /*
#if defined(ALIMER_D3D12)
        case BackendType::Direct3D12:
            if (D3D12GraphicsDevice::IsAvailable()) {
                Instance = new D3D12GraphicsDevice();
            }
            break;
#endif*/

#if defined(ALIMER_D3D11)
        case BackendType::Direct3D11:
            Instance = new D3D11GraphicsDevice();
            break;
#endif

        default:
            break;
        }
    }

    void GraphicsDevice::Shutdown()
    {
        if (Instance != nullptr) {
            delete Instance;
            Instance = nullptr;
        }
    }

    void GraphicsDevice::BeginFrame()
    {
        ALIMER_ASSERT_MSG(!frameActive, "Frame is still active, please call EndFrame first");

        if (!BeginFrameImpl()) {
            return;
        }

        /*ImGuiIO& io = ImGui::GetIO();
        IM_ASSERT(io.Fonts->IsBuilt());

        // Start the Dear ImGui frame
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();*/

        // Now the frame is active again.
        frameActive = true;
    }

    void GraphicsDevice::EndFrame()
    {
        ALIMER_ASSERT_MSG(frameActive, "Frame is not active, please call BeginFrame first.");

        //ImGui::Render();
        EndFrameImpl();

        // Update and Render additional Platform Windows
        /*ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault(nullptr, nullptr);
        }*/

        // Frame is not active anymore
        frameActive = false;
    }

    void GraphicsDevice::Resize(uint32_t width, uint32_t height)
    {
        if ((width != backbufferWidth || height != backbufferHeight) && width > 0 && height > 0)
        {
            backbufferWidth = width;
            backbufferHeight = height;
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
}
