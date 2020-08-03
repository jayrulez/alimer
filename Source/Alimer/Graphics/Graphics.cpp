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

#if defined(ALIMER_D3D11)
#endif

#if defined(ALIMER_VULKAN)
#include "Graphics/Vulkan/VulkanGraphicsImpl.h"
#endif

#if defined(ALIMER_D3D12)
#include "Graphics/D3D12/D3D12GraphicsDevice.h"
#endif

#if defined(ALIMER_OPENGL)
#include "Graphics/OpenGL/GLGraphics.h"
#endif

namespace alimer
{
    Graphics::Graphics(Window& window)
        : window{ window }
    {
        RegisterSubsystem(this);
    }

    Graphics::~Graphics()
    {
        RemoveSubsystem(this);
    }

    RendererType Graphics::GetDefaultRenderer(RendererType preferredBackend)
    {
        if (preferredBackend == RendererType::Count)
        {
            auto availableDrivers = GetAvailableRenderDrivers();

            if (availableDrivers.find(RendererType::Direct3D12) != availableDrivers.end())
                return RendererType::Direct3D12;
            else if (availableDrivers.find(RendererType::Vulkan) != availableDrivers.end())
                return RendererType::Vulkan;
            else if (availableDrivers.find(RendererType::Direct3D11) != availableDrivers.end())
                return RendererType::Direct3D11;
            else if (availableDrivers.find(RendererType::OpenGL) != availableDrivers.end())
                return RendererType::OpenGL;
            else
                return RendererType::Null;
        }

        return preferredBackend;
    }

    std::set<RendererType> Graphics::GetAvailableRenderDrivers()
    {
        static std::set<RendererType> availableDrivers;

        if (availableDrivers.empty())
        {
            availableDrivers.insert(RendererType::Null);

#if defined(ALIMER_D3D11)
            availableDrivers.insert(RendererType::Direct3D11);
#endif

#if defined(ALIMER_OPENGL)
            availableDrivers.insert(RendererType::OpenGL);
#endif
        }

        return availableDrivers;
    }

    Graphics* Graphics::Create(RendererType preferredBackend, Window& window, const GraphicsSettings& settings)
    {
        RendererType backend = GetDefaultRenderer(preferredBackend);

        Graphics* graphics = nullptr;
        switch (backend)
        {
#if defined(ALIMER_D3D11)
        case RendererType::Direct3D11:
            break;
#endif

#if defined(ALIMER_D3D12)
        case RendererType::Direct3D12:
            if (D3D12GraphicsDevice::IsAvailable()) {
                //Graphics = new D3D12GraphicsDevice(window, flags);
            }
            break;
#endif

#if defined(ALIMER_VULKAN)
        case RendererType::Vulkan:
            if (VulkanGraphicsImpl::IsAvailable()) {
                //device = new VulkanGraphicsImpl(applicationName, flags);
            }
            break;
#endif

#if defined(ALIMER_OPENGL)
        case RendererType::OpenGL:
            graphics = new GLGraphics(window, settings);
            break;
#endif

        default:
            break;
        }

        return graphics;
    }

    void Graphics::TrackResource(GraphicsResource* resource)
    {
        std::lock_guard<std::mutex> lock(trackedResourcesMutex);
        trackedResources.push_back(resource);
    }

    void Graphics::UntrackResource(GraphicsResource* resource)
    {
        std::lock_guard<std::mutex> lock(trackedResourcesMutex);

        auto it = std::find(trackedResources.begin(), trackedResources.end(), resource);
        if (it != trackedResources.end()) {
            trackedResources.erase(it);
        }
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

            trackedResources.clear();
        }
    }
}
