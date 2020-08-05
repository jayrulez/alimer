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

namespace alimer
{
    class D3D11CommandContext;
    class D3D11SwapChain;

    class GraphicsImpl final
    {
    public:
        GraphicsImpl();
        ~GraphicsImpl();

        bool Initialize(Window& window, GPUDeviceFlags flags);
        void Shutdown();
        bool BeginFrame();
        uint64_t EndFrame(uint64_t currentCPUFrame);

        void HandleDeviceLost(HRESULT hr);

        IDXGIFactory2* GetDXGIFactory() const { return dxgiFactory; }
        DXGIFactoryCaps GetDXGIFactoryCaps() const { return dxgiFactoryCaps; }
        ID3D11Device1* GetD3DDevice() const { return d3dDevice; }

        /// Get the device capabilities.
        ALIMER_FORCE_INLINE const GraphicsCapabilities& GetCaps() const { return caps; }

        //CommandContext* GetDefaultContext() const override;

        //Vector<D3D11SwapChain*> viewports;

    private:
        void CreateFactory();
        void InitCapabilities(IDXGIAdapter1* dxgiAdapter);
        void WaitForGPU();

        // Resource creation methods.
        //RefPtr<SwapChain> CreateSwapChain(void* windowHandle, uint32_t width, uint32_t height, bool isFullscreen, PixelFormat preferredColorFormat, PixelFormat depthStencilFormat) override;
        //RefPtr<Texture> CreateTexture(const TextureDescription& desc, const void* initialData) override;

        static constexpr uint64_t kRenderLatency = 2;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        HMODULE dxgiLib;
#endif

        IDXGIFactory2* dxgiFactory = nullptr;
        DXGIFactoryCaps dxgiFactoryCaps = DXGIFactoryCaps::None;
        uint32_t presentFlagsNoVSync = 0;
        D3D_FEATURE_LEVEL minFeatureLevel{ D3D_FEATURE_LEVEL_11_0 };

        ID3D11Device1* d3dDevice = nullptr;
        //UniquePtr<D3D11CommandContext> defaultContext;
        D3D_FEATURE_LEVEL d3dFeatureLevel = D3D_FEATURE_LEVEL_9_1;
        bool isLost = false;

        GraphicsCapabilities caps{};
    };
}
