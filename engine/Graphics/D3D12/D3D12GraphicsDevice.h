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
#include "D3D12Backend.h"
#include <queue>
#include <mutex>

#if !defined(ALIMER_DISABLE_SHADER_COMPILER)
#include <dxcapi.h>
#endif

namespace Alimer
{
    class D3D12Texture;
    class D3D12CommandContext;

    class D3D12GraphicsDevice final : public GraphicsDevice
    {
    public:
        D3D12GraphicsDevice(GraphicsDebugFlags flags, PhysicalDevicePreference adapterPreference);
        ~D3D12GraphicsDevice() override;

        void SetDeviceLost();
        void FinishFrame();
        void WaitForGPU() override;
        CommandContext* GetImmediateContext() const override;
        RefPtr<SwapChain> CreateSwapChain(void* windowHandle, const SwapChainDesc& desc) override;

        auto GetDXGIFactory() const noexcept { return dxgiFactory; }
        bool IsTearingSupported() const noexcept { return isTearingSupported; }
        auto GetD3DDevice() const noexcept { return d3dDevice; }
        auto GetGraphicsQueue() const noexcept { return graphicsQueue; }

    private:
        void Shutdown();
        void InitCapabilities(IDXGIAdapter1* dxgiAdapter);
        void GetAdapter(bool lowPower, IDXGIAdapter1** ppAdapter);

        DWORD dxgiFactoryFlags = 0;
        IDXGIFactory4* dxgiFactory = nullptr;
        bool isTearingSupported = false;

        ID3D12Device* d3dDevice = nullptr;
        D3D12MA::Allocator* allocator = nullptr;
        D3D_FEATURE_LEVEL featureLevel = kD3D12MinFeatureLevel;
        ID3D12CommandQueue* graphicsQueue;
        D3D12CommandContext* immediateContext;
    };
}
