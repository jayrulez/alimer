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

#include "Graphics/GraphicsDevice.h"
#include "D3D11Backend.h"
#include <queue>
#include <mutex>
struct ImDrawData;

namespace alimer
{
    class D3D11GraphicsDevice final : public GraphicsDevice
    {
    public:
        static bool IsAvailable();

        D3D11GraphicsDevice();
        ~D3D11GraphicsDevice();

        IDXGIFactory2* GetDXGIFactory() const { return dxgiFactory.Get(); }
        DXGIFactoryCaps GetDXGIFactoryCaps() const { return dxgiFactoryCaps; }

        ID3D11Device1* GetD3DDevice() const { return d3dDevice.Get(); }
        ID3D11DeviceContext1* GetD3DDeviceContext() const { return d3dContext.Get(); }

    private:
        void CreateFactory();
        void GetAdapter(IDXGIAdapter1** ppAdapter, bool lowPower = false);
        void InitCapabilities(IDXGIAdapter1* dxgiAdapter);
        void BackendShutdown() override;
        void WaitForGPU() override;
        bool BeginFrameImpl() override;
        void EndFrameImpl() override;
        void HandleDeviceLost();

        // Resource creation methods.
        RefPtr<SwapChain> CreateSwapChain(const SwapChainDescription& desc) override;
        RefPtr<Texture> CreateTexture(const TextureDescription& desc, const void* initialData) override;

        // Commands
        CommandList BeginCommandList(const char* name) override;
        void InsertDebugMarker(CommandList commandList, const char* name) override;
        void PushDebugGroup(CommandList commandList, const char* name) override;
        void PopDebugGroup(CommandList commandList) override;

        void SetScissorRect(CommandList commandList, const Rect& scissorRect) override;
        void SetScissorRects(CommandList commandList, const Rect* scissorRects, uint32_t count) override;
        void SetViewport(CommandList commandList, const Viewport& viewport) override;
        void SetViewports(CommandList commandList, const Viewport* viewports, uint32_t count) override;
        void SetBlendColor(CommandList commandList, const Color& color) override;

        void BindBuffer(CommandList commandList, uint32_t slot, GraphicsBuffer* buffer) override;
        void BindBufferData(CommandList commandList, uint32_t slot, const void* data, uint32_t size) override;

        static constexpr uint64_t kRenderLatency = 2;

        ComPtr<IDXGIFactory2> dxgiFactory = nullptr;
        DXGIFactoryCaps dxgiFactoryCaps = DXGIFactoryCaps::None;

        D3D_FEATURE_LEVEL minFeatureLevel{ D3D_FEATURE_LEVEL_11_0 };

        ComPtr<ID3D11Device1> d3dDevice;
        ComPtr<ID3D11DeviceContext1> d3dContext;
        D3D_FEATURE_LEVEL d3dFeatureLevel = D3D_FEATURE_LEVEL_9_1;
        bool isLost = false;
    };
}
