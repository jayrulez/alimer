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
#include "Graphics/GraphicsDevice.h"
#include "Core/Log.h"
#include "Core/Assert.h"

#if defined(ALIMER_D3D12_BACKEND)
#include "Graphics/D3D12/D3D12GraphicsDevice.h"
#endif

namespace alimer
{
    GraphicsDevice::GraphicsDevice(Window* window, const Desc& desc)
        : window{ window }
        , desc{ desc }
    {

    }

    std::unique_ptr<GraphicsDevice> GraphicsDevice::Create(Window* window, const Desc& desc)
    {
        BackendType backendType = desc.preferredBackendType;
        if (backendType == BackendType::Count) {
#if defined(ALIMER_D3D12_BACKEND)
            if (D3D12GraphicsDevice::IsAvailable()) {
                backendType = BackendType::Direct3D12;
            }
#endif
        }

        switch (backendType)
        {
#if defined(ALIMER_D3D12_BACKEND)
        case BackendType::Direct3D12:
            if (D3D12GraphicsDevice::IsAvailable()) {
                return std::make_unique<D3D12GraphicsDevice>(window, desc);
            }

            return nullptr;
#endif

        default:
            return nullptr;
        }

        return nullptr;
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
            for (auto i = trackedResources.begin(); i != trackedResources.end(); ++i)
            {
                (*i)->Release();
            }

            trackedResources.Clear();
        }
    }
}
