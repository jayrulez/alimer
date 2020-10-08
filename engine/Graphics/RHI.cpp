//
// Copyright (c) 2019-2020 Amer Koleci and contributors.
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

#include "AlimerConfig.h"
#include "Core/Log.h"
#include "Math/MathHelper.h"
#include "RHI/RHI.h"

#if defined(ALIMER_D3D11)
#include "RHI/D3D11/D3D11RHI.h"
#endif
#if defined(ALIMER_D3D12)
#endif
#if defined(ALIMER_VULKAN)
#endif

namespace Alimer
{
    float RHIViewport::AspectRatio() const
    {
        if (width == 0.f || height == 0.f)
            return 0.f;

        return (width / height);
    }

    RHIDevice* RHIDevice::Create(const std::string& applicationName, GraphicsBackendType preferredBackendType, GraphicsDeviceFlags flags)
    {
        RHIDevice* device = new D3D11RHIDevice(flags);
        return device;
    }

    /*void GraphicsDevice::AddGraphicsResource(GraphicsResource* resource)
    {
        ALIMER_ASSERT(resource);

        std::lock_guard<std::mutex> LockGuard(gpuObjectMutex);
        gpuObjects.push_back(resource);
    }

    void GraphicsDevice::RemoveGraphicsResource(GraphicsResource* resource)
    {
        ALIMER_ASSERT(resource);
        std::lock_guard<std::mutex> LockGuard(gpuObjectMutex);

        auto it = std::find(gpuObjects.begin(), gpuObjects.end(), resource);
        if (it != gpuObjects.end())
        {
            gpuObjects.erase(it);
        }
    }*/
}

