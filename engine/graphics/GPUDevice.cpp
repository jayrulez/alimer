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
#include "os/window.h"
#include "graphics/GPUDevice.h"
#include "graphics/GPUBuffer.h"
#include "graphics/Swapchain.h"

#if defined(ALIMER_GRAPHICS_VULKAN)
//#include "graphics/vulkan/VulkanGraphicsDevice.h"
#endif

#if defined(ALIMER_GRAPHICS_D3D12)
#include "graphics/d3d12/D3D12GPUDevice.h"
#endif

#if defined(ALIMER_GRAPHICS_D3D11)
//#include "graphics/d3d12/D3D12GraphicsDevice.h"
#endif

#if defined(ALIMER_GRAPHICS_OPENGL)
//#include "graphics/opengl/GLGPUDevice.h"
#endif

namespace alimer
{
    GPUDevice::GPUDevice(GPUProvider* provider, GPUAdapter* adapter)
        : _provider(provider)
        , _adapter(adapter)
    {
    }

    void GPUDevice::AddGPUResource(GPUResource* resource)
    {
        std::lock_guard<std::mutex> lock(_gpuResourceMutex);
        _gpuResources.push_back(resource);
    }

    void GPUDevice::RemoveGPUResource(GPUResource* resource)
    {
        std::lock_guard<std::mutex> lock(_gpuResourceMutex);
        _gpuResources.erase(std::remove(_gpuResources.begin(), _gpuResources.end(), resource), _gpuResources.end());
    }

    void GPUDevice::ReleaseTrackedResources()
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
    }

    RefPtr<SwapChain> GPUDevice::CreateSwapChain(const SwapChainDescriptor* descriptor)
    {
        ALIMER_ASSERT(descriptor);
        SwapChain* swapchain = CreateSwapChainCore(descriptor);
        if (swapchain == nullptr)
            return nullptr;
        return ConstructRefPtr<SwapChain>(swapchain);
    }

#if TOOD

    void GPUDevice::Present()
    {
        ALIMER_ASSERT(mainSwapchain.IsNotNull());
        Present({ mainSwapchain.Get() });
    }
#endif // TOOD

}
