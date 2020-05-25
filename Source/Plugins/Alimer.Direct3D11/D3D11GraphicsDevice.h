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

#pragma once

#include "graphics/GraphicsDevice.h"
#include "D3D11Backend.h"

namespace Alimer
{
    class D3D11GraphicsProvider;

    class ALIMER_API D3D11GraphicsDevice final : public GraphicsDevice
    {
    public:
        /// Constructor.
        D3D11GraphicsDevice(D3D11GraphicsProvider * provider, const std::shared_ptr<GraphicsAdapter> & adapter);
        /// Destructor.
        ~D3D11GraphicsDevice() override;

        IDXGIFactory2* GetDXGIFactory() const { return dxgiFactory; }
        bool                IsTearingSupported() const { return isTearingSupported; }
        ID3D11Device1* GetD3DDevice() const { return d3dDevice; }
        D3D_FEATURE_LEVEL   GetDeviceFeatureLevel() const { return d3dFeatureLevel; }

    private:
        void CreateDeviceResources();
        void Shutdown();
        void InitCapabilities();

        //SwapChain* CreateSwapChainCore(const SwapChainDescriptor* descriptor) override;
        //void Present(const std::vector<Swapchain*>& swapchains) override;

        bool validation;
        IDXGIFactory2* dxgiFactory;
        bool isTearingSupported;
        ID3D11Device1* d3dDevice = nullptr;
        ID3D11DeviceContext1* d3dContext = nullptr;
        D3D_FEATURE_LEVEL d3dFeatureLevel = D3D_FEATURE_LEVEL_9_1;
        bool isLost = false;
    };
}
