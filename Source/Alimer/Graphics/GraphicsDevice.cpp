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

#include "graphics/GraphicsDevice.h"
#include "Core/Log.h"
#include "Core/Assert.h"

#include "config.h"
#if defined(ALIMER_GRAPHICS_D3D12)
#include "graphics/Direct3D12/D3D12GraphicsDevice.h"
#endif

namespace alimer
{
    GraphicsDevice::GraphicsDevice()
    {

    }

    const GraphicsDeviceCaps& GraphicsDevice::GetCaps() const
    {
        return caps;
    }

    std::unique_ptr<GraphicsDevice> GraphicsDevice::Create(FeatureLevel minFeatureLevel, bool enableDebugLayer)
    {
        std::unique_ptr<GraphicsDevice> device;

#if defined(ALIMER_GRAPHICS_D3D12)
        device.reset(new D3D12GraphicsDevice(minFeatureLevel, enableDebugLayer));
#endif

        return device;
    }

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

}
