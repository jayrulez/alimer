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
#include "graphics/CommandQueue.h"
#include "graphics/SwapChain.h"

#if defined(ALIMER_GRAPHICS_D3D12)
#   include "graphics/d3d12/D3D12GraphicsDevice.h"
#endif

namespace alimer
{
#ifdef _DEBUG
#   define DEFAULT_ENABLE_VALIDATION true
#else
#   define DEFAULT_ENABLE_VALIDATION false
#endif

    bool GraphicsDevice::enableValidation = DEFAULT_ENABLE_VALIDATION;
    bool GraphicsDevice::enableGPUBasedValidation = false;

    bool GraphicsDevice::IsEnabledValidation()
    {
        return enableValidation;
    }

    void GraphicsDevice::SetEnableValidation(bool value)
    {
        enableValidation = value;
    }

    std::set<BackendType> GraphicsDevice::GetAvailableBackends()
    {
        static std::set<BackendType> availableProviders;

        if (availableProviders.empty())
        {
            availableProviders.insert(BackendType::Null);

#if defined(ALIMER_GRAPHICS_VULKAN)
            //if (VulkanGraphicsProvider::IsAvailable())
            //    availableProviders.insert(BackendType::Vulkan);
#endif

#if defined(ALIMER_GRAPHICS_D3D12)
            if (D3D12GraphicsDevice::IsAvailable())
                availableProviders.insert(BackendType::Direct3D12);
#endif

#if defined(ALIMER_GRAPHICS_D3D11)
            if (D3D11GPUDevice::IsAvailable())
                availableProviders.insert(BackendType::Direct3D11);
#endif

#if defined(ALIMER_GRAPHICS_OPENGL)
            availableProviders.insert(BackendType::OpenGL);
#endif
        }

        return availableProviders;
    }

    RefPtr<GraphicsDevice> GraphicsDevice::Create(BackendType preferredBackend, GPUPowerPreference powerPreference)
    {
        BackendType backend = preferredBackend;
        if (preferredBackend == BackendType::Count)
        {
            auto availableBackends = GetAvailableBackends();

            if (availableBackends.find(BackendType::Metal) != availableBackends.end())
                backend = BackendType::Metal;
            else if (availableBackends.find(BackendType::Direct3D12) != availableBackends.end())
                backend = BackendType::Direct3D12;
            else if (availableBackends.find(BackendType::Vulkan) != availableBackends.end())
                backend = BackendType::Vulkan;
            else if (availableBackends.find(BackendType::Direct3D11) != availableBackends.end())
                backend = BackendType::Direct3D11;
            else if (availableBackends.find(BackendType::OpenGL) != availableBackends.end())
                backend = BackendType::OpenGL;
            else
                backend = BackendType::Null;
        }

        GraphicsDevice* device = nullptr;
        switch (backend)
        {
#if defined(ALIMER_VULKAN)
        case BackendType::Vulkan:
            ALIMER_LOGINFO("Using Vulkan render driver");
            provider = std::make_unique<VulkanGraphicsProvider>(applicationName, flags);
            break;
#endif
#if defined(ALIMER_GRAPHICS_D3D12)
        case BackendType::Direct3D12:
            ALIMER_LOGINFO("Creating Direct3D12 GraphicsDevice");
            device = new D3D12GraphicsDevice();
            break;
#endif

#if defined(ALIMER_D3D11)
        case BackendType::Direct3D11:
            ALIMER_LOGINFO("Using Direct3D11 render driver");
            gpuDevice = make_shared<D3D11GPUDevice>(window, desc);
            break;
#endif

#if defined(ALIMER_OPENGL)
        case BackendType::OpenGL:
            ALIMER_LOGINFO("Using OpenGL render driver");
            provider = make_shared<GLGPUDevice>(window, desc);
            break;
#endif

        case BackendType::Metal:
            break;
        default:
            /* TODO: create null backend. */
            break;
        }

        if (device == nullptr || device->Init(powerPreference) == false)
        {
            return nullptr;
        }

        return RefPtr<GraphicsDevice>(device);
    }

    void GraphicsDevice::WaitForIdle()
    {
        graphicsCommandQueue->WaitForIdle();
        computeCommandQueue->WaitForIdle();
        copyCommandQueue->WaitForIdle();
    }

    std::shared_ptr<CommandQueue> GraphicsDevice::GetCommandQueue(CommandQueueType type) const
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
    RefPtr<SwapChain> GraphicsDevice::CreateSwapChain(void* windowHandle, const SwapChainDescriptor* descriptor)
    {
        ALIMER_ASSERT(windowHandle);
        ALIMER_ASSERT(descriptor);

        SwapChain* handle = CreateSwapChainCore(windowHandle, descriptor);
        if (handle == nullptr) {
            return nullptr;
        }

        return RefPtr<SwapChain>(handle);
    }

    void GraphicsDevice::AddGPUResource(GraphicsResource* resource)
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
    }
}
