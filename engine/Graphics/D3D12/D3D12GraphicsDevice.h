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

namespace Alimer
{
    class D3D11CommandBuffer;
    class D3D11SwapChain;

    class D3D12GraphicsDevice final : public GraphicsDevice
    {
    public:
        static bool IsAvailable();

        D3D12GraphicsDevice(const GraphicsDeviceDescription& desc);
        ~D3D12GraphicsDevice() override;

        void Shutdown();
        void HandleDeviceLost();

        bool IsLost() const { return isLost; }
        IDXGIFactory4* GetDXGIFactory() const { return dxgiFactory.Get(); }
        bool IsTearingSupported() const { return any(dxgiFactoryCaps & DXGIFactoryCaps::Tearing); }
        DXGIFactoryCaps GetDXGIFactoryCaps() const { return dxgiFactoryCaps; }
        ID3D12Device* GetD3DDevice() const { return d3dDevice; }
        D3D12MA::Allocator* GetAllocator() const { return allocator; }

    private:
        void GetAdapter(GraphicsAdapterPreference adapterPreference, IDXGIAdapter1** ppAdapter);
        void CreateCommandQueues();
        void InitCapabilities(IDXGIAdapter1* adapter);

        void WaitForGPU() override;
        bool BeginFrame() override;
        void EndFrame() override;
        CommandBuffer* GetCommandBuffer() override;

        DWORD dxgiFactoryFlags;
        ComPtr<IDXGIFactory4> dxgiFactory;
        DXGIFactoryCaps dxgiFactoryCaps;

        D3D_FEATURE_LEVEL d3dMinFeatureLevel;

        ID3D12Device* d3dDevice = nullptr;
        D3D12MA::Allocator* allocator = nullptr;

        D3D_FEATURE_LEVEL d3dFeatureLevel = D3D_FEATURE_LEVEL_9_1;
        bool supportsRenderPass = false;
        bool isLost = false;
    };
}
