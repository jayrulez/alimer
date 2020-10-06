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

namespace Alimer
{
    class D3D11CommandBuffer;

    struct D3D11RHIBuffer final : public RHIBuffer
    {
    public:
        D3D11RHIBuffer(D3D11GraphicsDevice* device, RHIBuffer::Usage usage, uint64_t size, HeapType heapType, const void* initialData);
        ~D3D11RHIBuffer() override;
        void Destroy() override;

        void SetName(const std::string& newName) override;

        D3D11GraphicsDevice* device;
        ID3D11Buffer* handle = nullptr;
    };

    class D3D11GraphicsDevice final : public GraphicsDevice
    {
    public:
        static bool IsAvailable();
        D3D11GraphicsDevice(GraphicsDeviceFlags flags);
        ~D3D11GraphicsDevice() override;

        void Shutdown();
        void HandleDeviceLost();

        IDXGIFactory2* GetDXGIFactory() const { return dxgiFactory.Get(); }
        bool IsTearingSupported() const { return any(dxgiFactoryCaps & DXGIFactoryCaps::Tearing); }
        DXGIFactoryCaps GetDXGIFactoryCaps() const { return dxgiFactoryCaps; }
        ID3D11Device1* GetD3DDevice() const { return device; }

    private:
        void CreateFactory();

#if defined(_DEBUG)
        bool IsSdkLayersAvailable() noexcept;
#endif

        void InitCapabilities(IDXGIAdapter1* adapter);

        bool IsDeviceLost() const override;
        void WaitForGPU() override;
        FrameOpResult BeginFrame(SwapChain* swapChain, BeginFrameFlags flags) override;
        FrameOpResult EndFrame(SwapChain* swapChain, EndFrameFlags flags) override;
        SwapChain* CreateSwapChain() override;
        RHIBuffer* CreateBuffer(RHIBuffer::Usage usage, uint64_t size, HeapType heapType) override;
        RHIBuffer* CreateStaticBuffer(RHIResourceUploadBatch* batch, const void* initialData, RHIBuffer::Usage usage, uint64_t size) override;

        bool debugRuntime;
        Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory;
        DXGIFactoryCaps dxgiFactoryCaps = DXGIFactoryCaps::None;

        ID3D11Device1*          device = nullptr;
        ID3D11DeviceContext1*   context = nullptr;

        D3D_FEATURE_LEVEL       featureLevel = D3D_FEATURE_LEVEL_9_1;
        bool deviceLost = false;
    };
}
