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
#include "D3D12Backend.h"
#include <EASTL/unique_ptr.h>

namespace D3D12MA
{
    class Allocator;
    class Allocation;
};

namespace alimer
{
    class D3D12GPU;
    class D3D12GPUDevice;

    class D3D12GPUSwapChain final 
    {
    public:
        D3D12GPUSwapChain(D3D12GPUDevice* device, WindowHandle windowHandle, bool fullscreen, PixelFormat backbufferFormat, bool enableVSync);
        ~D3D12GPUSwapChain();

        void AfterReset();
        void Resize(uint32_t width, uint32_t height);
        void Present();

    private:
        static constexpr uint32_t kNumBackBuffers = 2;

        D3D12GPUDevice* device;
        IDXGISwapChain3* handle;
        uint32_t syncInterval;
        uint32_t presentFlags;
        uint32_t backBufferIndex = 0;
        uint32_t width;
        uint32_t height;
    };

    class D3D12GPUDevice final : public GPUDevice
    {
    public:
        /// Constructor.
        D3D12GPUDevice(D3D12GPU* gpu, IDXGIAdapter1* adapter, WindowHandle windowHandle, const GPUDevice::Desc& desc);
        ~D3D12GPUDevice() override;

        bool BeginFrame() override;
        void EndFrame() override;

        DXGIFactoryCaps GetDXGIFactoryCaps() const;
        IDXGIFactory4* GetDXGIFactory() const;
        ALIMER_FORCE_INLINE ID3D12Device* GetD3DDevice() const { return d3dDevice.Get(); }
        ALIMER_FORCE_INLINE D3D12MA::Allocator* GetAllocator() const { return allocator; }
        ALIMER_FORCE_INLINE bool SupportsRenderPass() const { return supportsRenderPass; }
        ALIMER_FORCE_INLINE ID3D12CommandQueue* GetGraphicsQueue() const { return graphicsQueue; }
        ALIMER_FORCE_INLINE GPUContext* GetMainContext() const { return nullptr; }

    private:
        D3D12GPU* gpu;
        ComPtr<ID3D12Device> d3dDevice;
        D3D12MA::Allocator* allocator;
        bool supportsRenderPass{ false };

        ID3D12CommandQueue* graphicsQueue = nullptr;
        eastl::unique_ptr<D3D12GPUSwapChain> swapChain;
    };

    class D3D12GPU final
    {
    public:
        static bool IsAvailable();
        static D3D12GPU* Get();

        GPUDevice* CreateDevice(WindowHandle windowHandle, const GPUDevice::Desc& desc);

        ALIMER_FORCE_INLINE DXGIFactoryCaps GetDXGIFactoryCaps() const { return dxgiFactoryCaps; }
        ALIMER_FORCE_INLINE IDXGIFactory4* GetDXGIFactory() const { return dxgiFactory; }

    private:
        D3D12GPU();
        ~D3D12GPU();

        DWORD dxgiFactoryFlags = 0;
        IDXGIFactory4* dxgiFactory = nullptr;
        DXGIFactoryCaps dxgiFactoryCaps = DXGIFactoryCaps::FlipPresent | DXGIFactoryCaps::HDR;
    };
}
