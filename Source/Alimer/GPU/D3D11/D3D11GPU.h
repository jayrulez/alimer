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

#include "GPU/GPU.h"
#include "D3D11Backend.h"
#include <EASTL/unique_ptr.h>

namespace alimer
{
    class D3D11GPU;
    class D3D11GPUDevice;

    class D3D11GPUSwapChain final 
    {
    public:
        D3D11GPUSwapChain(D3D11GPUDevice* device, WindowHandle windowHandle, bool fullscreen, PixelFormat backbufferFormat, bool enableVSync);
        ~D3D11GPUSwapChain();

        void AfterReset();
        void Resize(uint32_t width, uint32_t height);
        void Present();

    private:
        static constexpr uint32_t kNumBackBuffers = 2;

        D3D11GPUDevice* device;
        IDXGISwapChain3* handle;
        uint32_t syncInterval;
        uint32_t presentFlags;
        uint32_t backBufferIndex = 0;
        uint32_t width;
        uint32_t height;
    };

    class D3D11GPUDevice final : public GPUDevice
    {
    public:
        D3D11GPUDevice(D3D11GPU* gpu, IDXGIAdapter1* adapter, WindowHandle windowHandle, const GPUDevice::Desc& desc);
        ~D3D11GPUDevice() override;

        bool BeginFrame() override;
        void EndFrame() override;

        DXGIFactoryCaps GetDXGIFactoryCaps() const;
        IDXGIFactory2* GetDXGIFactory() const;
        ALIMER_FORCE_INLINE ID3D11Device1* GetD3DDevice() const { return d3dDevice.Get(); }

    private:
        D3D11GPU* gpu;
        ComPtr<ID3D11Device1> d3dDevice;
        eastl::unique_ptr<D3D11GPUSwapChain> swapChain;
    };

    class D3D11GPU final
    {
    public:
        static bool IsAvailable();
        static D3D11GPU* Get();

        void CreateFactory();
        eastl::intrusive_ptr<GPUDevice> CreateDevice(WindowHandle windowHandle, const GPUDevice::Desc& desc);

        ALIMER_FORCE_INLINE DXGIFactoryCaps GetDXGIFactoryCaps() const { return dxgiFactoryCaps; }
        ALIMER_FORCE_INLINE IDXGIFactory2* GetDXGIFactory() const { return dxgiFactory; }

    private:
        D3D11GPU();
        ~D3D11GPU();

        DWORD dxgiFactoryFlags = 0;
        IDXGIFactory2* dxgiFactory = nullptr;
        DXGIFactoryCaps dxgiFactoryCaps = DXGIFactoryCaps::FlipPresent | DXGIFactoryCaps::HDR;
    };
}
