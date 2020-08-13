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

#if TODO
#include "D3D11CommandContext.h"
#include "D3D11Texture.h"
#include "D3D11GraphicsDevice.h"

namespace alimer
{
    D3D11CommandContext::D3D11CommandContext(D3D11GraphicsDevice* device_, ID3D11DeviceContext* context_)
        : device(device_)
    {
        ThrowIfFailed(context_->QueryInterface(IID_PPV_ARGS(&context)));
        ThrowIfFailed(context_->QueryInterface(IID_PPV_ARGS(&annotation)));
        context_->Release();
    }

    D3D11CommandContext::~D3D11CommandContext()
    {
        SafeRelease(annotation);
        SafeRelease(context);
    }

    void D3D11CommandContext::Flush()
    {
        context->Flush();
    }

    void D3D11CommandContext::PushDebugGroup(const String& name)
    {
        auto wideName = ToUtf16(name);
        annotation->BeginEvent(wideName.c_str());
    }

    void D3D11CommandContext::PopDebugGroup()
    {
        annotation->EndEvent();
    }

    void D3D11CommandContext::InsertDebugMarker(const String& name)
    {
        auto wideName = ToUtf16(name);
        annotation->SetMarker(wideName.c_str());
    }

    void D3D11CommandContext::BeginRenderPass(const RenderPassDescription& renderPass)
    {
        
    }

    void D3D11CommandContext::EndRenderPass()
    {

    }

    void D3D11CommandContext::SetScissorRect(uint32 x, uint32 y, uint32 width, uint32 height)
    {
        D3D11_RECT d3dScissorRect;
        d3dScissorRect.left = LONG(x);
        d3dScissorRect.top = LONG(y);
        d3dScissorRect.right = LONG(x + width);
        d3dScissorRect.bottom = LONG(y + height);
        context->RSSetScissorRects(1, &d3dScissorRect);
    }

    void D3D11CommandContext::SetScissorRects(const Rect* scissorRects, uint32_t count)
    {
        D3D11_RECT d3dScissorRects[kMaxViewportAndScissorRects];
        for (uint32_t i = 0; i < count; ++i)
        {
            d3dScissorRects[i].left = LONG(scissorRects[i].x);
            d3dScissorRects[i].top = LONG(scissorRects[i].y);
            d3dScissorRects[i].right = LONG(scissorRects[i].x + scissorRects[i].width);
            d3dScissorRects[i].bottom = LONG(scissorRects[i].y + scissorRects[i].height);
        }
        context->RSSetScissorRects(count, d3dScissorRects);
    }

    void D3D11CommandContext::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
    {
        D3D11_VIEWPORT viewport;
        viewport.TopLeftX = x;
        viewport.TopLeftY = y;
        viewport.Width = width;
        viewport.Height = height;
        viewport.MinDepth = minDepth;
        viewport.MaxDepth = maxDepth;

        context->RSSetViewports(1, &viewport);
    }

    void D3D11CommandContext::SetBlendColor(const Color& color)
    {
        blendColor = color;
    }

    void D3D11CommandContext::BindBuffer(uint32_t slot, Buffer* buffer)
    {
        /*if (buffer != nullptr)
        {
            auto d3d11Buffer = static_cast<D3D11Buffer*>(buffer)->GetHandle();
            context->VSSetConstantBuffers(slot, 1, &d3d11Buffer);
            context->PSSetConstantBuffers(slot, 1, &d3d11Buffer);
        }
        else
        {
            context->VSSetConstantBuffers(slot, 1, nullptr);
            context->PSSetConstantBuffers(slot, 1, nullptr);
        }*/
    }

    void D3D11CommandContext::BindBufferData(uint32_t slot, const void* data, uint32_t size)
    {
        // TODO:
    }
}

#endif // TODO
