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

#if defined(ALIMER_VULKAN)
#   include "graphics/vulkan/VulkanGPUDevice.h"
#endif

#if defined(ALIMER_D3D12)
//#include "graphics/d3d12/D3D12GPUDevice.h"
#endif

#if defined(ALIMER_OPENGL)
#   include "graphics/opengl/GLGPUDevice.h"
#endif

using namespace std;

namespace alimer
{
    GPUDevicePtr gpuDevice;

    GPUDevice::GPUDevice(Window* window_, const Desc& desc_)
        : window(window_)
        , desc(desc_)
    {

    }

    set<GPUBackend> GPUDevice::GetAvailableBackends()
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

#if defined(ALIMER_OPENGL)
            availableBackends.insert(GPUBackend::OpenGL);
#endif
        }

        return availableBackends;
    }

    GPUDevicePtr GPUDevice::Create(Window* window, const Desc& desc)
    {
        if (gpuDevice)
        {
            ALIMER_LOGE("Only single GPU instance is allowed.");
            return nullptr;
        }

        GPUBackend backend = desc.preferredBackend;
        if (desc.preferredBackend == GPUBackend::Count)
        {
            auto availableBackends = GetAvailableBackends();

            if (availableBackends.find(GPUBackend::Metal) != availableBackends.end())
                backend = GPUBackend::Metal;
            else if (availableBackends.find(GPUBackend::Direct3D12) != availableBackends.end())
                backend = GPUBackend::Direct3D12;
            else if (availableBackends.find(GPUBackend::Vulkan) != availableBackends.end())
                backend = GPUBackend::Vulkan;
            else if (availableBackends.find(GPUBackend::OpenGL) != availableBackends.end())
                backend = GPUBackend::OpenGL;
            else
                backend = GPUBackend::Null;
        }

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
#if defined(ALIMER_OPENGL)
        case GPUBackend::OpenGL:
            ALIMER_LOGINFO("Using OpenGL render driver");
            gpuDevice = make_shared<GLGPUDevice>(window, desc);
            break;
#endif

        case GPUBackend::Metal:
            break;
        default:
            break;
        }

        if (gpuDevice->Initialize() == false)
        {
            gpuDevice = nullptr;
        }

        return gpuDevice;
    }

    bool GPUDevice::Initialize()
    {
        if (BackendInit() == false) return false;

        return true;
    }

    void GPUDevice::Shutdown()
    {

    }

    void GPUDevice::NotifyValidationError(const char* message)
    {

    }
}
