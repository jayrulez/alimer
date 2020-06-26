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
#include <algorithm>
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

    std::unique_ptr<GraphicsDevice> GraphicsDevice::Create(WindowHandle window, const Desc& desc)
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

        std::unique_ptr<GraphicsDevice> device = nullptr;
        switch (backend)
        {
#if defined(ALIMER_VULKAN)
        case BackendType::Vulkan:
            if (VulkanGraphicsDevice::IsAvailable())
            {
                device = std::make_unique<VulkanGraphicsDevice>(window, desc);
                LOG_INFO("Using Vulkan driver");
            }
            break;
#endif
#if defined(ALIMER_D3D12)
        case BackendType::Direct3D12:
            if (D3D12GraphicsDevice::IsAvailable())
            {
                device = std::make_unique<D3D12GraphicsDevice>(window, desc);
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
            size.width = width;
            size.height = height;
        }
    }


    static inline uint32_t CalculateMipLevels(uint32_t width, uint32_t height = 0, uint32_t depth = 0)
    {
        uint32_t levels = 1;
        for (uint32_t size = max(max(width, height), depth); size > 1; levels += 1)
        {
            size /= 2;
        }

        return levels;
    }

    RefPtr<Texture> GraphicsDevice::CreateTexture2D(uint32_t width, uint32_t height, PixelFormat format, uint32_t mipLevels, uint32_t arraySize, TextureUsage usage, const void* initialData)
    {
        const bool autoGenerateMipmaps = mipLevels == kMaxPossibleMipLevels;
        const bool hasInitData = initialData != nullptr;
        if (autoGenerateMipmaps && hasInitData)
        {
            usage |= TextureUsage::RenderTarget | TextureUsage::GenerateMipmaps;
        }

        TextureDescription desc = {};
        desc.type = TextureType::Type2D;
        desc.format = format;
        desc.usage = usage;
        desc.width = width;
        desc.height = height;
        desc.depth = 1;
        desc.mipLevels = autoGenerateMipmaps ? CalculateMipLevels(width, height) : mipLevels;
        desc.arraySize = arraySize;
        desc.sampleCount = TextureSampleCount::Count1;

        return CreateTexture(desc, initialData);
    }

    RefPtr<Texture> GraphicsDevice::CreateTextureCube(uint32_t size, PixelFormat format, uint32_t mipLevels, uint32_t arraySize, TextureUsage usage, const void* initialData)
    {
        const bool autoGenerateMipmaps = mipLevels == kMaxPossibleMipLevels;
        const bool hasInitData = initialData != nullptr;
        if (autoGenerateMipmaps && hasInitData)
        {
            usage |= TextureUsage::RenderTarget;
        }

        TextureDescription desc = {};
        desc.type = TextureType::TypeCube;
        desc.format = format;
        desc.usage = usage;
        desc.width = size;
        desc.height = size;
        desc.depth = 1;
        desc.mipLevels = autoGenerateMipmaps ? CalculateMipLevels(size) : mipLevels;
        desc.arraySize = arraySize;
        desc.sampleCount = TextureSampleCount::Count1;

        return CreateTexture(desc, initialData);
    }

    void GraphicsDevice::TrackResource(GraphicsResource* resource)
    {
        std::lock_guard<std::mutex> lock(trackedResourcesMutex);
        trackedResources.push_back(resource);
    }

    void GraphicsDevice::UntrackResource(GraphicsResource* resource)
    {
        std::lock_guard<std::mutex> lock(trackedResourcesMutex);
        trackedResources.erase(std::remove(trackedResources.begin(), trackedResources.end(), resource), trackedResources.end());
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

    Texture* GraphicsDevice::GetBackbufferTexture() const
    {
        return backbufferTextures[backbufferIndex].Get();
    }

    Texture* GraphicsDevice::GetDepthStencilTexture() const
    {
        return depthStencilTexture.Get();
    }
}
