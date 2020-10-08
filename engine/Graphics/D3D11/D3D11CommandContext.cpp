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

#include "D3D11CommandContext.h"
#include "D3D11Texture.h"
#include "D3D11RHI.h"

namespace Alimer
{
    D3D11CommandContext::D3D11CommandContext(D3D11RHIDevice* device_, ID3D11DeviceContext1* context_)
        : device(device_)
        , context(context_)
    {
        ThrowIfFailed(context->QueryInterface(&annotation));
    }

    D3D11CommandContext::~D3D11CommandContext()
    {
        SafeRelease(annotation);
        SafeRelease(context);
        //context->Release(); context = nullptr;
    }

    void D3D11CommandContext::PushDebugGroup(const std::string& name)
    {
        std::wstring wideLabel = ToUtf16(name);
        annotation->BeginEvent(wideLabel.c_str());
    }

    void D3D11CommandContext::PopDebugGroup()
    {
        annotation->EndEvent();
    }

    void D3D11CommandContext::InsertDebugMarker(const std::string& name)
    {
        std::wstring wideLabel = ToUtf16(name);
        annotation->SetMarker(wideLabel.c_str());
    }

    void D3D11CommandContext::SetViewport(const RHIViewport& viewport)
    {
        D3D11_VIEWPORT d3dViewport;
        d3dViewport.TopLeftX = viewport.x;
        d3dViewport.TopLeftY = viewport.y;
        d3dViewport.Width = viewport.width;
        d3dViewport.Height = viewport.height;
        d3dViewport.MinDepth = viewport.minDepth;
        d3dViewport.MaxDepth = viewport.maxDepth;
        context->RSSetViewports(1, &d3dViewport);
    }

    void D3D11CommandContext::SetScissorRect(const RectI& scissorRect)
    {
        D3D11_RECT d3dScissorRect;
        d3dScissorRect.left = static_cast<LONG>(scissorRect.x);
        d3dScissorRect.top = static_cast<LONG>(scissorRect.y);
        d3dScissorRect.right = static_cast<LONG>(scissorRect.x + scissorRect.width);
        d3dScissorRect.bottom = static_cast<LONG>(scissorRect.y + scissorRect.height);
        context->RSSetScissorRects(1, &d3dScissorRect);
    }

    void D3D11CommandContext::SetBlendColor(const Color& color)
    {

    }

    void D3D11CommandContext::BeginRenderPass(const RenderPassDesc& renderPass)
    {
        ID3D11RenderTargetView* renderTargetViews[kMaxColorAttachments];

        uint32_t numColorAttachments = 0;

        for (uint32_t i = 0; i < kMaxColorAttachments; i++)
        {
            auto& attachment = renderPass.colorAttachments[i];
            if (attachment.texture == nullptr)
                continue;

            D3D11Texture* texture = static_cast<D3D11Texture*>(attachment.texture);

            ID3D11RenderTargetView* rtv = texture->GetRTV(DXGI_FORMAT_UNKNOWN, attachment.mipLevel, attachment.slice);

            switch (attachment.loadAction)
            {
            case LoadAction::DontCare:
                context->DiscardView(rtv);
                break;

            case LoadAction::Clear:
                context->ClearRenderTargetView(rtv, &attachment.clearColor.r);
                break;

            default:
                break;
            }

            renderTargetViews[numColorAttachments++] = rtv;
        }

        context->OMSetRenderTargets(numColorAttachments, renderTargetViews, nullptr);
    }

    void D3D11CommandContext::EndRenderPass()
    {
        // TODO: Resolve 
        context->OMSetRenderTargets(kMaxColorAttachments, zeroRTVS, nullptr);
    }


}

