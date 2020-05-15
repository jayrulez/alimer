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
#include "core/Log.h"
#include "core/Assert.h"
#include "graphics/GraphicsDevice.h"

#if defined(ALIMER_GRAPHICS_VULKAN) && defined(BLAH)
#   include "graphics/vulkan/GraphicsDeviceVK.h"
#endif

namespace alimer
{
    /*void GraphicsDevice::AddGPUResource(GraphicsResource* resource)
    {
        std::lock_guard<std::mutex> lock(_gpuResourceMutex);
        _gpuResources.push_back(resource);
    }

    void GraphicsDevice::RemoveGPUResource(GraphicsResource* resource)
    {
        std::lock_guard<std::mutex> lock(_gpuResourceMutex);
        _gpuResources.erase(std::remove(_gpuResources.begin(), _gpuResources.end(), resource), _gpuResources.end());
    }

    void GraphicsDevice::ReleaseTrackedResources()
    {
        {
            std::lock_guard<std::mutex> lock(_gpuResourceMutex);

            // Release all GPU objects that still exist
            for (auto i = _gpuResources.begin(); i != _gpuResources.end(); ++i)
            {
                (*i)->Release();
            }

            _gpuResources.clear();
        }
    }*/

    std::set<GraphicsAPI> GetAvailableGraphicsAPI()
    {
        static std::set<GraphicsAPI> availableProviders;

        if (availableProviders.empty())
        {

#if defined(ALIMER_GRAPHICS_D3D12)
            //if (D3D12GraphicsDevice::IsAvailable())
           //     availableProviders.insert(BackendType::Direct3D12);
#endif

#if defined(ALIMER_GRAPHICS_OPENGL)
           // availableProviders.insert(BackendType::OpenGL);
#endif
        }

        return availableProviders;
    }


    std::unique_ptr<IGraphicsDevice> CreateGraphicsDevice(GraphicsAPI api, const GraphicsDeviceDesc* pDesc)
    {
        ALIMER_ASSERT(pDesc);

        if (api == GraphicsAPI::Count)
        {
            auto availableBackends = GetAvailableGraphicsAPI();

            if (availableBackends.find(GraphicsAPI::Metal) != availableBackends.end())
                api = GraphicsAPI::Metal;
            else if (availableBackends.find(GraphicsAPI::Direct3D12) != availableBackends.end())
                api = GraphicsAPI::Direct3D12;
            else if (availableBackends.find(GraphicsAPI::Vulkan) != availableBackends.end())
                api = GraphicsAPI::Vulkan;
            else if (availableBackends.find(GraphicsAPI::Direct3D11) != availableBackends.end())
                api = GraphicsAPI::Direct3D11;
            else if (availableBackends.find(GraphicsAPI::OpenGL) != availableBackends.end())
                api = GraphicsAPI::OpenGL;
            else
                api = GraphicsAPI::Null;
        }

        return nullptr;
    }
}
