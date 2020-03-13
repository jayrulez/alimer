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
#include "graphics/GPUDevice.h"

#if defined(ALIMER_D3D12)
//#include "graphics/d3d12/vulkan_backend.h"
#endif

#if defined(ALIMER_D3D11)
#   include "graphics/d3d11/D3D11GPUDevice.h"
#endif

#if defined(ALIMER_VULKAN)
#   include "graphics/vulkan/VulkanGPUDevice.h"
#endif

namespace alimer
{
    bool GPUDevice::Init(const DeviceDesc& desc)
    {
        return BackendInit(desc);
    }

    GPUDevice* GPUDevice::Create(GPUBackend preferredBackend)
    {
        if (preferredBackend == GPUBackend::Count) {
            preferredBackend = GPUBackend::Direct3D11;
        }

        GPUDevice* gpuDevice = nullptr;
        switch (preferredBackend)
        {
#if defined(ALIMER_D3D11)
        case GPUBackend::Direct3D11:
            if (D3D11GPUDevice::IsAvailable())
                gpuDevice = new D3D11GPUDevice();
            break;
#endif

#if defined(ALIMER_VULKAN)
        case GPUBackend::Vulkan:
            if (VulkanGPUDevice::IsAvailable())
                gpuDevice = new VulkanGPUDevice();
            break;
#endif

        default:
            break;
        }

        return gpuDevice;
    }

    void GPUDevice::NotifyValidationError(const char* message)
    {

    }
}
