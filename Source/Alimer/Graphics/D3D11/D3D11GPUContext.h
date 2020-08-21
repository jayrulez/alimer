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

#pragma once

#include "Graphics/GPUContext.h"
#include "D3D11Backend.h"

namespace alimer
{
    class ALIMER_API D3D11GPUContext final : public GPUContext
    {
    public:
        /// Constructor.
        D3D11GPUContext(D3D11GPUDevice * device, ID3D11DeviceContext1* context, bool isMain_);
        /// Destructor
        ~D3D11GPUContext() override;

        void Flush();

        void PushDebugGroup(const String& name) override;
        void PopDebugGroup() override;
        void InsertDebugMarker(const String& name) override;

        void BeginRenderPass(uint32_t numColorAttachments, const RenderPassColorAttachment* colorAttachments, const RenderPassDepthStencilAttachment* depthStencil) override;
        void EndRenderPass() override;

        void SetScissorRect(const URect& scissorRect) override;
        void SetScissorRects(const URect* scissorRects, uint32_t count) override;
        void SetViewport(const Viewport& viewport) override;
        void SetViewports(const Viewport* viewports, uint32_t count) override;

        void SetBlendColor(const Color& color) override;

        void BindBuffer(uint32_t slot, GPUBuffer* buffer) override;
        void BindBufferData(uint32_t slot, const void* data, uint32_t size) override;

    private:
        bool BeginFrameImpl() override;
        void EndFrameImpl() override;
        void CreateObjects();
        void CreateSwapChainObjects();

    private:
        D3D11GPUDevice* device;
        ID3D11DeviceContext1* handle = nullptr;
        ID3DUserDefinedAnnotation* annotation = nullptr;
        ID3D11RenderTargetView* zeroRTVS[kMaxColorAttachments] = {};
    };
}
