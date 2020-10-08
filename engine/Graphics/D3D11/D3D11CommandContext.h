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

#include "Graphics/CommandContext.h"
#include "D3D11Backend.h"

namespace Alimer
{
    class D3D11CommandContext final : public CommandContext
    {
    public:
        D3D11CommandContext(D3D11RHIDevice* device_, ID3D11DeviceContext1* context_);
        ~D3D11CommandContext();

        void PushDebugGroup(const std::string& name) override;
        void PopDebugGroup() override;
        void InsertDebugMarker(const std::string& name) override;
        void SetViewport(const RHIViewport& viewport) override;
        void SetScissorRect(const RectI& scissorRect) override;
        void SetBlendColor(const Color& color) override;
        void BeginRenderPass(const RenderPassDesc& renderPass) override;
        void EndRenderPass() override;

        D3D11RHIDevice* device;
        ID3D11DeviceContext1* context;
        ID3DUserDefinedAnnotation* annotation = nullptr;

    private:
        ID3D11RenderTargetView* zeroRTVS[kMaxColorAttachments] = {};
    };
}
