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

#include "Graphics/GraphicsImpl.h"
#include "Graphics/GraphicsDevice.h"
#include "D3D11Backend.h"
#include <mutex>

namespace alimer
{
    class D3D11GPUAdapter;

    struct D3D11Buffer
    {
        enum { MAX_COUNT = 4096 };

        ID3D11Buffer* handle;
    };

    class D3D11GPUDevice final : public GraphicsDevice
    {
    public:
        static bool IsAvailable();
        D3D11GPUDevice(const GPUDeviceDescriptor& descriptor);
        ~D3D11GPUDevice() override;

        void Shutdown();

        bool BeginFrameImpl() override;
        void EndFrameImpl() override;
        void Present(GPUSwapChain* swapChain, bool verticalSync) override;
        void HandleDeviceLost();

        GPUAdapter* GetAdapter() const override;
        GPUContext* GetMainContext() const override;
        GPUSwapChain* GetMainSwapChain() const override;

        IDXGIFactory2* GetDXGIFactory() const { return dxgiFactory; }
        bool IsTearingSupported() const { return isTearingSupported; }
        DXGIFactoryCaps GetDXGIFactoryCaps() const { return dxgiFactoryCaps; }
        ID3D11Device1* GetD3DDevice() const { return d3dDevice; }

        /* Resource creation methods */
        GPUSwapChain* CreateSwapChainCore(const GPUSwapChainDescriptor& descriptor) override;

        BufferHandle AllocBufferHandle();
        BufferHandle CreateBuffer(BufferUsage usage, uint32_t size, uint32_t stride, const void* data);
        void Destroy(BufferHandle handle);
        void SetName(BufferHandle handle, const char* name);

    private:
        void CreateFactory();
        void InitCapabilities();

        static constexpr uint64_t kRenderLatency = 2;

        IDXGIFactory2* dxgiFactory = nullptr;
        bool isTearingSupported = false;
        DXGIFactoryCaps dxgiFactoryCaps = DXGIFactoryCaps::None;

        D3D11GPUAdapter* adapter = nullptr;
        ID3D11Device1*              d3dDevice = nullptr;

        D3D_FEATURE_LEVEL d3dFeatureLevel = D3D_FEATURE_LEVEL_9_1;
        bool isLost = false;

        GPUContext* mainContext = nullptr;
        RefPtr<GPUSwapChain> mainSwapChain;

        /* Resource pools */
        std::mutex handle_mutex;
        GPUResourcePool<D3D11Buffer, D3D11Buffer::MAX_COUNT> buffers;
    };
}
