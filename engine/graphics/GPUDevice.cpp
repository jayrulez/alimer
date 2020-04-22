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

#include "core/Log.h"
#include "core/Assert.h"
#include "os/window.h"
#include "graphics/GPUDevice.h"
#include "graphics/GPUBuffer.h"
#include "graphics/CommandQueue.h"

namespace alimer
{
    RefPtr<GPUDevice> GPUDevice::Create(Window* window, const Desc& desc)
    {
        ALIMER_ASSERT(window);

        RefPtr<GPUDevice> device(new GPUDevice(window, desc));
        if (device->Init() == false) {
            device = nullptr;
        }

        return device;
    }

    GPUDevice::GPUDevice(Window* window_, const Desc& desc)
        : window(window_)
        , desc{ desc }
    {
    }

    GPUDevice::~GPUDevice()
    {
        WaitForIdle();

        ReleaseTrackedResources();
        //ExecuteDeferredReleases();

        copyCommandQueue.reset();
        computeCommandQueue.reset();
        graphicsCommandQueue.reset();

        mainSwapChain.reset();

        ApiDestroy();
    }

    bool GPUDevice::Init()
    {
        if (ApiInit() == false) {
            return false;
        }

        graphicsCommandQueue = std::make_shared<CommandQueue>(*this, CommandQueueType::Graphics);
        computeCommandQueue = std::make_shared<CommandQueue>(*this, CommandQueueType::Compute);
        copyCommandQueue = std::make_shared<CommandQueue>(*this, CommandQueueType::Copy);

        mainSwapChain.reset(new SwapChain(*this, window->GetHandle(), window->GetSize()));

        return true;
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
