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

#if defined(ALIMER_VULKAN)
#   include "graphics/vulkan/VulkanGPUDevice.h"
#endif

#if defined(ALIMER_D3D12)
//#include "graphics/d3d12/D3D12GPUDevice.h"
#endif

using namespace std;

namespace alimer
{
    set<GPUBackend> GPUDevice::getAvailableBackends()
    {
        static set<GPUBackend> availableBackends;

        if (availableBackends.empty())
        {
            availableBackends.insert(GPUBackend::Null);

#if defined(ALIMER_VULKAN)
            if (VulkanGPUDevice::IsAvailable())
                availableBackends.insert(GPUBackend::Vulkan);
#endif

#if defined(ALIMER_D3D12)
            /*if (D3D12GPUDevice::IsAvailable())
            {
                availableBackends.insert(GPUBackend::Direct3D12);
            }*/
#endif
        }

        return availableBackends;
    }

    unique_ptr<GPUDevice> GPUDevice::Create(GPUBackend preferredBackend, GPUDeviceFlags flags)
    {
        GPUBackend backend = preferredBackend;
        if (preferredBackend == GPUBackend::Count)
        {
            auto availableBackends = getAvailableBackends();

            if (availableBackends.find(GPUBackend::Metal) != availableBackends.end())
                backend = GPUBackend::Metal;
            else if (availableBackends.find(GPUBackend::Direct3D12) != availableBackends.end())
                backend = GPUBackend::Direct3D12;
            else if (availableBackends.find(GPUBackend::Vulkan) != availableBackends.end())
                backend = GPUBackend::Vulkan;
            else
                backend = GPUBackend::Null;
        }

        unique_ptr<GPUDevice> device;
        switch (backend)
        {
#if defined(ALIMER_VULKAN)
        case GPUBackend::Vulkan:
            ALIMER_LOGINFO("Using Vulkan render driver");
            device = make_unique<VulkanGPUDevice>(flags);
            break;
#endif
            /*
#if defined(ALIMER_D3D12)
        case GPUBackend::Direct3D12:
            ALIMER_LOGINFO("Using Direct3D12 render driver");
            device = make_unique<D3D12GPUDevice>(validation);
            break;
#endif*/

        case GPUBackend::Metal:
            break;
        default:
            break;
        }

        return device;
    }

    void GPUDevice::NotifyValidationError(const char* message)
    {

    }

    SharedPtr<GPUBuffer> GPUDevice::CreateBuffer(const BufferDescriptor* descriptor, const void* initialData)
    {
        ALIMER_ASSERT(descriptor);
        GPUBuffer* buffer = CreateBufferCore(descriptor, initialData);
        return buffer;
    }

    std::shared_ptr<Framebuffer> GPUDevice::createFramebuffer(const SwapChainDescriptor* descriptor)
    {
        return nullptr;
        //return createFramebufferCore(descriptor);
    }
}
