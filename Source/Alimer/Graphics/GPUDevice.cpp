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

#include "config.h"
#include "Core/Log.h"
#include "Math/MathHelper.h"
#include "Graphics/GPUDevice.h"

#if defined(ALIMER_D3D11)
#   include "Graphics/D3D11/D3D11GPUDevice.h"
#endif

#if defined(ALIMER_VULKAN)
#   include "Graphics/Vulkan/VulkanGPUDevice.h"
#endif

namespace alimer
{
    bool GPUDevice::enableGPUValidation = false;

    void GPUDevice::EnableGPUBasedBackendValidation(bool value)
    {
        enableGPUValidation = value;
    }

    bool GPUDevice::IsGPUBasedBackendValidationEnabled()
    {
        return enableGPUValidation;
    }

    GPUDevice::GPUDevice(GPUBackendType backendType_)
        : backendType(backendType_)
    {
        LOGI("Using {} driver", ToString(backendType));
    }

    GPUDevice* GPUDevice::Create(const String& appName, const GPUDeviceDescriptor& descriptor, GPUBackendType preferredRendererType)
    {
        GPUBackendType backendType = preferredRendererType;

        if (preferredRendererType == GPUBackendType::Count)
        {
            //backendType = GPUBackendType::D3D11;
            backendType = GPUBackendType::Vulkan;
        }

        switch (backendType)
        {
#if defined(ALIMER_VULKAN) 
        case GPUBackendType::Vulkan:
            if (VulkanGPUDevice::IsAvailable())
            {
                return new VulkanGPUDevice(appName, descriptor);
            }
            break;
#endif

#if defined(ALIMER_METAL)
        case RendererType::Metal:
            break;
#endif

#if defined(ALIMER_D3D11)
        case GPUBackendType::D3D11:
            return new D3D11GPUDevice(descriptor);
#endif

#if defined(ALIMER_D3D12)
        case GPUBackendType::D3D12:
            break;
#endif

        default:
            // TODO: Add Null device.
            return nullptr;
        }

        return nullptr;
    }

    GPUContext* GPUDevice::CreateContext(const GPUContextDescription& desc)
    {
        return CreateContextCore(desc);
    }
}

