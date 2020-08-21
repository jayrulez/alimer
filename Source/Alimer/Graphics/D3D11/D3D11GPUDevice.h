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

#include "Graphics/GPUDevice.h"
#include "D3D11Backend.h"
#include <mutex>

namespace Alimer
{
    class D3D11GPUAdapter;
    class D3D11GPUContext;

    class D3D11GPUDevice final : public GPUDevice
    {
    public:
        static bool IsAvailable();
        D3D11GPUDevice(const GraphicsDeviceDescription& desc);
        ~D3D11GPUDevice() override;

        void Shutdown();

        void HandleDeviceLost(HRESULT hr);

        GPUAdapter* GetAdapter() const override;
        GPUContext* GetMainContext() const override;
        IDXGIFactory2* GetDXGIFactory() const { return dxgiFactory; }
        bool IsTearingSupported() const { return isTearingSupported; }
        DXGIFactoryCaps GetDXGIFactoryCaps() const { return dxgiFactoryCaps; }
        ID3D11Device1* GetD3DDevice() const { return d3dDevice; }
        bool IsLost() const { return isLost; }

        /* Resource creation methods */
        GPUContext* CreateContextCore(const GPUContextDescription& desc) override;

    private:
        void CreateFactory();
        void InitCapabilities();
        bool BeginFrameImpl() override;
        void EndFrameImpl() override;

        static constexpr uint64_t kRenderLatency = 2;

        IDXGIFactory2* dxgiFactory = nullptr;
        bool isTearingSupported = false;
        DXGIFactoryCaps dxgiFactoryCaps = DXGIFactoryCaps::None;

        D3D11GPUAdapter* adapter = nullptr;
        ID3D11Device1*  d3dDevice = nullptr;
        ID3D11DeviceContext1* d3dContext = nullptr;

        D3D_FEATURE_LEVEL d3dFeatureLevel = D3D_FEATURE_LEVEL_9_1;
        bool isLost = false;

        D3D11GPUContext* mainContext = nullptr;
    };
}
