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

#include "Graphics/Swapchain.h"
#include "D3D12Backend.h"

namespace alimer
{
    class D3D12Texture;

    class D3D12Swapchain final : public Swapchain
    {
    public:
        D3D12Swapchain(D3D12GraphicsDevice* device, const SwapchainDescription& description);
        ~D3D12Swapchain() override;
        void Destroy() override;
        void Present();

    private:
        void ResizeImpl() override;
        void AfterReset();
        void BackendSetName() override;

        D3D12GraphicsDevice* device;
        DXGI_FORMAT backbufferFormat;
        IDXGISwapChain3* handle;
        uint32 backbufferCount;
        uint32 backbufferIndex = 0;
        SharedPtr<D3D12Texture> backbufferTextures[kInflightFrameCount] = {};

        uint32 syncInterval = 1;
        uint32 presentFlags = 0;
    };
}
