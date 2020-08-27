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

#if defined(ALIMER_ENABLE_VULKAN)
#include "Graphics/Vulkan/VulkanGraphicsImpl.h"
#endif

namespace Alimer
{
    GraphicsDevice* GraphicsDevice::Instance;

    GraphicsDevice::GraphicsDevice(GraphicsImpl* impl)
    {
        //LOGI("Using {} driver", ToString(impl->GetBackendType()));
    }

    GraphicsDevice::~GraphicsDevice()
    {
    }

    bool GraphicsDevice::Initialize(Window* window, const GraphicsDeviceSettings& settings)
    {
        if (Instance != nullptr)
        {
            LOGW("Cannot initialize more than one GraphicsDevice instances");
            return true;
        }

        Instance = new GraphicsDevice(nullptr);
        return true;
    }

    void GraphicsDevice::WaitForGPU()
    {
        //impl->WaitForGPU();
    }

    bool GraphicsDevice::BeginFrame()
    {
        ALIMER_ASSERT_MSG(!frameActive, "Frame is still active, please call EndFrame first.");

        /*if (!impl->BeginFrame()) {
            return false;
        }*/

        // Now the frame is active again.
        frameActive = true;
        return true;
    }

    void GraphicsDevice::EndFrame()
    {
        ALIMER_ASSERT_MSG(frameActive, "Frame is not active, please call BeginFrame");

        //impl->EndFrame();

        // Frame is not active anymore.
        frameActive = false;
        ++frameCount;
    }

    /*GPUBackendType GraphicsDevice::GetBackendType() const
    {
        return impl->GetBackendType();
    }

    const GPUFeatures& GraphicsDevice::GetFeatures() const
    {
        return impl->GetFeatures();
    }

    const GPULimits& GraphicsDevice::GetLimits() const
    {
        return impl->GetLimits();
    }*/
}

