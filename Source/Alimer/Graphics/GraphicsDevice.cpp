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
#include "Graphics/GraphicsImpl.h"

#if defined(ALIMER_D3D11)
#   include "Graphics/D3D11/D3D11GraphicsImpl.h"
#endif

#if defined(ALIMER_VULKAN)
#   include "Graphics/Vulkan/VulkanGraphicsImpl.h"
#endif

namespace alimer
{
    GraphicsDevice* GraphicsDevice::Create(RendererType preferredRendererType)
    {
        if (preferredRendererType == RendererType::Count)
        {
            preferredRendererType = RendererType::Direct3D11;
        }

        switch (preferredRendererType)
        {
#if defined(ALIMER_VULKAN)
        case RendererType::Vulkan:
            //impl = new VulkanGraphicsImpl();
            break;
#endif

#if defined(ALIMER_METAL)
        case RendererType::Metal:
            break;
#endif

#if defined(ALIMER_D3D11)
        case RendererType::Direct3D11:
            return new D3D11GraphicsImpl();
#endif

#if defined(ALIMER_D3D12)
        case RendererType::Direct3D12:
            break;
#endif

        default:
            // TODO: Add Null device.
            return nullptr;
        }

        return nullptr;
    }

    void GraphicsDevice::SetVerticalSync(bool value)
    {
        impl->SetVerticalSync(value);
    }

    bool GraphicsDevice::GetVerticalSync() const
    {
        return impl->GetVerticalSync();
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

        ++frameCount;
        EndFrameImpl();

        // Frame is not active anymore.
        frameActive = false;
    }

    Texture* GraphicsDevice::GetBackbufferTexture() const
    {
        return impl->GetBackbufferTexture();
    }

    /* Commands */
    void GraphicsDevice::PushDebugGroup(const String& name, CommandList commandList)
    {
        impl->PushDebugGroup(name, commandList);
    }

    void GraphicsDevice::PopDebugGroup(CommandList commandList)
    {
        impl->PopDebugGroup(commandList);
    }

    void GraphicsDevice::InsertDebugMarker(const String& name, CommandList commandList)
    {
        impl->InsertDebugMarker(name, commandList);
    }

    void GraphicsDevice::BeginRenderPass(uint32_t numColorAttachments, const RenderPassColorAttachment* colorAttachments, const RenderPassDepthStencilAttachment* depthStencil, CommandList commandList)
    {
        ALIMER_ASSERT(numColorAttachments < kMaxColorAttachments);
        ALIMER_ASSERT(numColorAttachments || depthStencil);

        impl->BeginRenderPass(commandList, numColorAttachments, colorAttachments, depthStencil);
    }

    void GraphicsDevice::EndRenderPass(CommandList commandList)
    {
        impl->EndRenderPass(commandList);
    }
}

