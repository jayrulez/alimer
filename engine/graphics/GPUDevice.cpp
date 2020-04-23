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
#include "graphics/GPUDevice.h"
#include "graphics/GPUBuffer.h"
#include "graphics/CommandQueue.h"
#include "graphics/SwapChain.h"

#if defined(ALIMER_GRAPHICS_D3D12)
#include "graphics/d3d12/D3D12GPUDevice.h"
#endif

namespace alimer
{
#ifdef _DEBUG
#   define DEFAULT_ENABLE_VALIDATION true
#else
#   define DEFAULT_ENABLE_VALIDATION false
#endif

    bool GPUDevice::enableValidation = DEFAULT_ENABLE_VALIDATION;
    bool GPUDevice::enableGPUBasedValidation = false;

    bool GPUDevice::IsEnabledValidation()
    {
        return enableValidation;
    }

    void GPUDevice::SetEnableValidation(bool value)
    {
        enableValidation = value;
    }

    RefPtr<GPUDevice> GPUDevice::Create(BackendType preferredBackend, GPUPowerPreference powerPreference)
    {
        GPUDevice* device = nullptr;

#if defined(ALIMER_GRAPHICS_D3D12)
        device = new D3D12GPUDevice();
#endif

        if (device == nullptr || device->Init(powerPreference) == false)
        {
            return nullptr;
        }

        return RefPtr<GPUDevice>(device);
    }

    std::shared_ptr<CommandQueue> GPUDevice::GetCommandQueue(CommandQueueType type) const
    {
        std::shared_ptr<CommandQueue> commandQueue;
        switch (type)
        {
        case CommandQueueType::Graphics:
            commandQueue = graphicsCommandQueue;
            break;
        case CommandQueueType::Compute:
            commandQueue = computeCommandQueue;
            break;
        case CommandQueueType::Copy:
            commandQueue = copyCommandQueue;
            break;
        default:
            ALIMER_ASSERT_FAIL("Invalid command queue type.");
        }

        return commandQueue;
    }

    /// Create new SwapChain.
    RefPtr<SwapChain> GPUDevice::CreateSwapChain(void* windowHandle, const SwapChainDescriptor* descriptor)
    {
        ALIMER_ASSERT(windowHandle);
        ALIMER_ASSERT(descriptor);

        SwapChain* handle = CreateSwapChainCore(windowHandle, descriptor);
        if (handle == nullptr) {
            return nullptr;
        }

        return RefPtr<SwapChain>(handle);
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
}
