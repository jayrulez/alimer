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
#include "Core/Log.h"
#include "Core/Assert.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/Texture.h"

#if defined(ALIMER_D3D11_BACKEND)
#include "Graphics/D3D11/GraphicsDevice_D3D11.h"
#endif

#if defined(ALIMER_D3D12_BACKEND)
#include "Graphics/D3D12/D3D12GraphicsDevice.h"
#endif

namespace alimer
{
    GraphicsDevice::GraphicsDevice(const Desc& desc)
        : desc{ desc }
    {

    }

    GraphicsDevice* GraphicsDevice::Create(const Desc& desc, const SwapChainDesc& swapchainDesc)
    {
        BackendType backendType = desc.preferredBackendType;
        if (backendType == BackendType::Count) {
#if defined(ALIMER_D3D11_BACKEND)
            if (GraphicsDevice_D3D11::IsAvailable()) {
                backendType = BackendType::Direct3D11;
            }
#endif
        }

        GraphicsDevice* device = nullptr;
        switch (backendType)
        {
#if defined(ALIMER_D3D11_BACKEND)
        case BackendType::Direct3D11:
            if (GraphicsDevice_D3D11::IsAvailable()) {
                device = new GraphicsDevice_D3D11(desc);
            }
            else {
                return nullptr;
            }
            break;
#endif

        default:
            return nullptr;
        }

        if (device == nullptr) {
            return nullptr;
        }

        device->mainSwapChain = new SwapChain(*device, swapchainDesc);
        return device;
    }

    void GraphicsDevice::TrackResource(GraphicsResource* resource)
    {
        std::lock_guard<std::mutex> lock(trackedResourcesMutex);
        trackedResources.Push(resource);
    }

    void GraphicsDevice::UntrackResource(GraphicsResource* resource)
    {
        std::lock_guard<std::mutex> lock(trackedResourcesMutex);
        trackedResources.Remove(resource);
    }

    void GraphicsDevice::ReleaseTrackedResources()
    {
        {
            std::lock_guard<std::mutex> lock(trackedResourcesMutex);

            // Release all GPU objects that still exist
            for (auto i = trackedResources.begin(); i != trackedResources.end(); ++i)
            {
                (*i)->Release();
            }

            trackedResources.Clear();
        }
    }

    void GraphicsDevice::BeginDefaultRenderPass(const Color& clearColor, float clearDepth, uint8_t clearStencil, CommandList commandList)
    {
        RenderPassDesc passDesc = {};
        passDesc.colorAttachments[0].texture = mainSwapChain->GetBackbufferTexture()->GetHandle();
        passDesc.colorAttachments[0].clearColor = clearColor;
        passDesc.colorAttachments[0].loadAction = LoadAction::Clear;

        Texture* depthStencilTexture = mainSwapChain->GetDepthStencilTexture();
        if (depthStencilTexture != nullptr)
        {
            passDesc.depthStencilAttachment.texture = depthStencilTexture->GetHandle();
            passDesc.depthStencilAttachment.depthLoadAction = LoadAction::Clear;
            passDesc.depthStencilAttachment.clearDepth = clearDepth;
            //
            //passDesc.depthStencilAttachment.clearStencil = clearStencil;
            //passDesc.depthStencilAttachment.stencilLoadOp = LoadAction::Clear;
        }

        BeginRenderPass(passDesc, commandList);
    }
}
