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

#if defined(ALIMER_D3D11)
#include "Graphics/D3D11/D3D11GraphicsDevice.h"
#elif defined(ALIMER_D3D12)
#include "Graphics/D3D12/D3D12GraphicsDevice.h"
#elif defined(ALIMER_VULKAN)
#else
#endif

namespace alimer
{
    GraphicsDevice::GraphicsDevice(GPUDeviceFlags flags)
        : impl(new GraphicsImpl(flags))
    {

    }

    GraphicsDevice::~GraphicsDevice()
    {
        delete impl;
        impl = nullptr;
    }

    bool GraphicsDevice::Initialize(Window& window)
    {
        if (initialized) {
            return true;
        }

        initialized = impl->Initialize(window);
        return true;
    }

    bool GraphicsDevice::BeginFrame()
    {
        return impl->BeginFrame();
    }

    void GraphicsDevice::EndFrame()
    {
        ++currentCPUFrame;
        currentGPUFrame = impl->EndFrame(currentCPUFrame);
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

    const GraphicsCapabilities& GraphicsDevice::GetCaps() const
    {
        return impl->GetCaps();
    }

    GraphicsImpl* GraphicsDevice::GetImpl() const
    {
        return impl;
    }
}
