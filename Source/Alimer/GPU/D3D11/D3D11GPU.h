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

    class D3D11GPUTexture final : public GPUTexture
    {
    public:
        D3D11GPUTexture(D3D11GPUDevice* device_, const GPUTexture::Desc& desc, ID3D11Texture2D* externalHandle);
        ~D3D11GPUTexture() override;

    private:
        D3D11GPUDevice* device;
        ID3D11Resource* handle;
    };

    class D3D11GPUSwapChain final 
    {
    public:
        D3D11GPUSwapChain(D3D11GPUDevice* device, WindowHandle windowHandle, bool isFullscreen, PixelFormat colorFormat_, bool enableVSync);
        ~D3D11GPUSwapChain();

        void AfterReset();
        void Resize(uint32_t width, uint32_t height);

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        IDXGISwapChain1* handle;
#else
        IDXGISwapChain3* handle;
#endif

    private:
        static constexpr uint32_t kNumBackBuffers = 2;

        D3D11GPUDevice* device;
        PixelFormat colorFormat;
        uint32_t width;
        uint32_t height;
        eastl::intrusive_ptr<D3D11GPUTexture> backbufferTexture;
    };

    class D3D11GPUContext final : public GPUContext
    {
    public:
        D3D11GPUContext(D3D11GPUDevice* device_, ID3D11DeviceContext* context_);
        ~D3D11GPUContext() override;

    private:
        D3D11GPUDevice* device;
        ID3D11DeviceContext1* context;
        ID3DUserDefinedAnnotation* annotation;
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
        ALIMER_FORCE_INLINE ID3D11Device1* GetD3DDevice() const { return d3dDevice; }
        ALIMER_FORCE_INLINE GPUContext* GetMainContext() const { return mainContext.get(); }

        eastl::vector<D3D11GPUSwapChain*> viewports;
    private:
        D3D11GPU* gpu;

        ID3D11Device1* d3dDevice;
        D3D_FEATURE_LEVEL d3dFeatureLevel = D3D_FEATURE_LEVEL_9_1;
        bool isLost{ false };
        uint32_t presentFlagsNoVSync = 0;
        eastl::intrusive_ptr<D3D11GPUContext> mainContext;
        eastl::unique_ptr<D3D11GPUSwapChain> mainViewport;
    };

    class D3D11GPU final
    {
    public:
        static bool IsAvailable();
        static D3D11GPU* Get();

        void CreateFactory();
        GPUDevice* CreateDevice(WindowHandle windowHandle, const GPUDevice::Desc& desc);

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
