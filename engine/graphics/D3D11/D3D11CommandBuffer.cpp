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

#include "Core/Log.h"
#include "D3D11CommandBuffer.h"
#include "D3D11Texture.h"
#include "D3D11GraphicsDevice.h"

namespace Alimer
{
    D3D11CommandBuffer::D3D11CommandBuffer(D3D11GraphicsDevice* device, uint64_t memoryStreamBlockSize)
        : CommandBuffer(memoryStreamBlockSize)
        , device{ device }
    {
        if (memoryStreamBlockSize)
        {
            entryPoints.resize((size_t)CommandID::Count);
            entryPoints[static_cast<uint8_t>(CommandID::PushDebugGroup)] = &D3D11CommandBuffer::CmdPushDebugGroup;
            entryPoints[static_cast<uint8_t>(CommandID::PopDebugGroup)] = &D3D11CommandBuffer::CmdPopDebugGroup;
            entryPoints[static_cast<uint8_t>(CommandID::InsertDebugMarker)] = &D3D11CommandBuffer::CmdInsertDebugMarker;
            entryPoints[static_cast<uint8_t>(CommandID::BeginRenderPass)] = &D3D11CommandBuffer::CmdBeginRenderPass;
            entryPoints[static_cast<uint8_t>(CommandID::EndRenderPass)] = &D3D11CommandBuffer::CmdEndRenderPass;
        }
    }

    void D3D11CommandBuffer::Reset()
    {
        CommandBuffer::ResetState();
    }

    void D3D11CommandBuffer::Execute(ID3D11DeviceContext1* context, ID3DUserDefinedAnnotation* annotation)
    {
        // Seek to the beginning of the stream for reading.
        SeekG(0);

        // Read all of the stream's data and decode each command.
        while (true)
        {
            // Get current stream read position.
            auto streamPosition = TellG();

            // Parse and execute the next command in the stream.
            CommandID cmd = ReadCommandID();
            if (EndOfStream())
                break;

            (this->*entryPoints[static_cast<uint8_t>(cmd)])(context, annotation);
        }
    }

    void D3D11CommandBuffer::CommitCore()
    {
        device->CommitCommandBuffer(this);
    }

    void D3D11CommandBuffer::WaitUntilCompletedCore()
    {
        device->SubmitCommandBuffer(this);
    }

    void D3D11CommandBuffer::SetScissorRect(const URect& scissorRect)
    {
        /*D3D11_RECT d3dScissorRect;
        d3dScissorRect.left = static_cast<LONG>(scissorRect.x);
        d3dScissorRect.top = static_cast<LONG>(scissorRect.y);
        d3dScissorRect.right = static_cast<LONG>(scissorRect.x + scissorRect.width);
        d3dScissorRect.bottom = static_cast<LONG>(scissorRect.y + scissorRect.height);
        handle->RSSetScissorRects(1, &d3dScissorRect);*/
    }

    void D3D11CommandBuffer::SetScissorRects(const URect* scissorRects, uint32_t count)
    {
        /*D3D11_RECT d3dScissorRects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        for (uint32 i = 0; i < count; i++)
        {
            d3dScissorRects[i].left = static_cast<LONG>(scissorRects[i].x);
            d3dScissorRects[i].top = static_cast<LONG>(scissorRects[i].y);
            d3dScissorRects[i].right = static_cast<LONG>(scissorRects[i].x + scissorRects[i].width);
            d3dScissorRects[i].bottom = static_cast<LONG>(scissorRects[i].y + scissorRects[i].height);
        }

        handle->RSSetScissorRects(count, d3dScissorRects);*/
    }

    void D3D11CommandBuffer::SetViewport(const Viewport& viewport)
    {

    }

    void D3D11CommandBuffer::SetViewports(const Viewport* viewports, uint32_t count)
    {

    }

    void D3D11CommandBuffer::SetBlendColor(const Color& color)
    {

    }

    void D3D11CommandBuffer::BindBuffer(uint32_t slot, GPUBuffer* buffer)
    {

    }

    void D3D11CommandBuffer::BindBufferData(uint32_t slot, const void* data, uint32_t size)
    {

    }

    /* Shared functions between software command buffer and context one */
    void D3D11CommandBuffer::CmdPushDebugGroup(ID3DUserDefinedAnnotation* annotation, const std::string& label)
    {
        std::wstring wideLabel = ToUtf16(label);
        annotation->BeginEvent(wideLabel.c_str());
    }

    void D3D11CommandBuffer::CmdInsertDebugMarker(ID3DUserDefinedAnnotation* annotation, const std::string& label)
    {
        std::wstring wideLabel = ToUtf16(label);
        annotation->SetMarker(wideLabel.c_str());
    }

    void D3D11CommandBuffer::CmdPushDebugGroup(ID3D11DeviceContext1* context, ID3DUserDefinedAnnotation* annotation)
    {
        ALIMER_UNUSED(context);
        CmdPushDebugGroup(annotation, ReadString());
    }

    void D3D11CommandBuffer::CmdPopDebugGroup(ID3D11DeviceContext1* context, ID3DUserDefinedAnnotation* annotation)
    {
        ALIMER_UNUSED(context);
        annotation->EndEvent();
    }

    void D3D11CommandBuffer::CmdBeginRenderPass(ID3D11DeviceContext1* context, uint32_t numColorAttachments, const RenderPassColorAttachment* colorAttachments, const RenderPassDepthStencilAttachment* depthStencil)
    {
        ID3D11RenderTargetView* renderTargetViews[kMaxColorAttachments];

        for (uint32_t i = 0; i < numColorAttachments; i++)
        {
            D3D11Texture* texture = static_cast<D3D11Texture*>(colorAttachments[i].texture);

            ID3D11RenderTargetView* rtv = texture->GetRTV(DXGI_FORMAT_UNKNOWN, colorAttachments[i].mipLevel, colorAttachments[i].slice);

            switch (colorAttachments[i].loadAction)
            {
            case LoadAction::DontCare:
                context->DiscardView(rtv);
                break;

            case LoadAction::Clear:
                context->ClearRenderTargetView(rtv, &colorAttachments[i].clearColor.r);
                break;

            default:
                break;
            }

            renderTargetViews[i] = rtv;
        }

        context->OMSetRenderTargets(numColorAttachments, renderTargetViews, nullptr);
    }

    void D3D11CommandBuffer::CmdEndRenderPass(ID3D11DeviceContext1* context)
    {
        // TODO: Resolve 
        context->OMSetRenderTargets(kMaxColorAttachments, zeroRTVS, nullptr);
    }

    void D3D11CommandBuffer::CmdInsertDebugMarker(ID3D11DeviceContext1* context, ID3DUserDefinedAnnotation* annotation)
    {
        ALIMER_UNUSED(context);
        CmdInsertDebugMarker(annotation, ReadString());
    }

    void D3D11CommandBuffer::CmdBeginRenderPass(ID3D11DeviceContext1* context, ID3DUserDefinedAnnotation* annotation)
    {
        ALIMER_UNUSED(annotation);
        uint32_t numColorAttachments = Read<uint32_t>();
        const RenderPassColorAttachment* colorAttachments = ReadPtr<const RenderPassColorAttachment>(numColorAttachments);
        const RenderPassDepthStencilAttachment* depthStencil = nullptr;

        bool hasDepthStencil = Read<uint8_t>() != 0;
        if (hasDepthStencil)
        {
            depthStencil = ReadPtr<const RenderPassDepthStencilAttachment>();
        }

        CmdBeginRenderPass(context, numColorAttachments, colorAttachments, depthStencil);
    }

    void D3D11CommandBuffer::CmdEndRenderPass(ID3D11DeviceContext1* context, ID3DUserDefinedAnnotation* annotation)
    {
        ALIMER_UNUSED(annotation);
        CmdEndRenderPass(context);
    }

    /* D3D11ContextCommandBuffer */
    D3D11ContextCommandBuffer::D3D11ContextCommandBuffer(D3D11GraphicsDevice* device)
        : D3D11CommandBuffer(device, 0)
    {
        ThrowIfFailed(device->GetD3DDevice()->CreateDeferredContext1(0, d3dContext.ReleaseAndGetAddressOf()));
        ThrowIfFailed(d3dContext.As(&d3dAnnotation));
    }

    D3D11ContextCommandBuffer::~D3D11ContextCommandBuffer()
    {

    }

    void D3D11ContextCommandBuffer::Reset()
    {

    }

    void D3D11ContextCommandBuffer::Execute(ID3D11DeviceContext1* context, ID3DUserDefinedAnnotation* annotation)
    {
        d3dContext->FinishCommandList(false, commandList.ReleaseAndGetAddressOf());
        context->ExecuteCommandList(commandList.Get(), false);
    }

    void D3D11ContextCommandBuffer::PushDebugGroup(const char* name)
    {
        CmdPushDebugGroup(d3dAnnotation.Get(), name);
    }

    void D3D11ContextCommandBuffer::PopDebugGroup()
    {
        CmdPopDebugGroup(d3dContext.Get(), d3dAnnotation.Get());
    }

    void D3D11ContextCommandBuffer::InsertDebugMarker(const char* name)
    {
        std::wstring wideLabel = ToUtf16(name, strlen(name));
        d3dAnnotation->SetMarker(wideLabel.c_str());
    }

    void D3D11ContextCommandBuffer::BeginRenderPass(uint32_t numColorAttachments, const RenderPassColorAttachment* colorAttachments, const RenderPassDepthStencilAttachment* depthStencil)
    {
        CmdBeginRenderPass(d3dContext.Get(), numColorAttachments, colorAttachments, depthStencil);
    }

    void D3D11ContextCommandBuffer::EndRenderPass()
    {
        CmdEndRenderPass(d3dContext.Get());
    }
}

