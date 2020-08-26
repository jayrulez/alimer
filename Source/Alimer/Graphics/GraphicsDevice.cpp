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
#include "Graphics/GraphicsDevice.h"

#if defined(ALIMER_ENABLE_BACKEND_D3D11)
#   include "Graphics/D3D11/D3D11GraphicsDevice.h"
#endif

#if defined(ALIMER_VULKAN) && defined(TODO_VK)
#   include "Graphics/Vulkan/VulkanGPUDevice.h"
#endif

namespace Alimer
{
#if defined(ALIMER_ENABLE_BACKEND_D3D11)
    GPUBackendType GraphicsDevice::s_backendType = GPUBackendType::D3D11;
#elif defined(ALIMER_ENABLE_BACKEND_METAL)
    GPUBackendType GraphicsDevice::s_backendType = wgpu::BackendType::Metal;
#elif defined(ALIMER_ENABLE_BACKEND_VULKAN)
    GPUBackendType GraphicsDevice::s_backendType = wgpu::BackendType::Vulkan;
#elif defined(ALIMER_ENABLE_BACKEND_OPENGL)
    GPUBackendType GraphicsDevice::s_backendType = wgpu::BackendType::OpenGL;
#else
#    error
#endif

    GraphicsDevice::GraphicsDevice(Window* window, GPUBackendType backendType)
        : window{ window }
        , backendType{ backendType }
    {
        LOGI("Using {} driver", ToString(backendType));
    }

    GraphicsDevice* GraphicsDevice::Create(Window* window, const GraphicsDeviceSettings& settings)
    {
        if (s_backendType == GPUBackendType::Count)
        {
            s_backendType = GPUBackendType::D3D11;
            //backendType = GPUBackendType::Vulkan;
        }

        switch (s_backendType)
        {
#if defined(ALIMER_VULKAN) && defined(TODO_VK)
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

#if defined(ALIMER_ENABLE_BACKEND_D3D11)
        case GPUBackendType::D3D11:
            return new D3D11GraphicsDevice(window, settings);
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

    bool GraphicsDevice::BeginFrame()
    {
        ALIMER_ASSERT_MSG(!frameActive, "Frame is still active, please call EndFrame first.");

        if (!BeginFrameImpl()) {
            return false;
        }

        // Now the frame is active again.
        frameActive = true;
        return true;
    }

    void GraphicsDevice::EndFrame()
    {
        ALIMER_ASSERT_MSG(frameActive, "Frame is not active, please call BeginFrame");

        EndFrameImpl();

        // Frame is not active anymore.
        frameActive = false;
        ++frameCount;
    }
}

