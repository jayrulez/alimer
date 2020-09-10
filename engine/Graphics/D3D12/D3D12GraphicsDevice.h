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
    class D3D12Texture;

    class D3D12GraphicsDevice final : public GraphicsDevice
    {
    public:
        static bool IsAvailable();

        D3D12GraphicsDevice(const GraphicsDeviceDescription& desc);
        ~D3D12GraphicsDevice() override;

        void Shutdown();
        void HandleDeviceLost();

        D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count);

        template<typename T> void DeferredRelease(T*& resource, bool forceDeferred = false)
        {
            IUnknown* base = resource;
            DeferredRelease_(base, forceDeferred);
            resource = nullptr;
        }

        bool IsLost() const { return isLost; }
        IDXGIFactory4* GetDXGIFactory() const { return dxgiFactory.Get(); }
        bool IsTearingSupported() const { return any(dxgiFactoryCaps & DXGIFactoryCaps::Tearing); }
        DXGIFactoryCaps GetDXGIFactoryCaps() const { return dxgiFactoryCaps; }
        ID3D12Device* GetD3DDevice() const { return d3dDevice; }
        D3D12MA::Allocator* GetAllocator() const { return allocator; }
        ID3D12CommandQueue* GetGraphicsQueue() const { return graphicsQueue; }

    private:
        void GetAdapter(GraphicsAdapterPreference adapterPreference, IDXGIAdapter1** ppAdapter);
        void InitCapabilities(IDXGIAdapter1* adapter);

        void WaitForGPU() override;
        bool BeginFrame() override;
        void EndFrame(const std::vector<SwapChain*>& swapChains) override;
        void DeferredRelease_(IUnknown* resource, bool forceDeferred = false);
        void ProcessDeferredReleases(uint64 index);

        /* CommandList */
        struct FrameResources
        {
            ID3D12CommandAllocator* commandAllocators[kMaxCommandListCount];
        };

        FrameResources frames[kRenderLatency];
        FrameResources& GetFrameResources() { return frames[frameIndex]; }
        inline ID3D12GraphicsCommandList4* GetDirectCommandList(CommandList cmd) { return commandLists[cmd]; }

        CommandList BeginCommandList() override;
        void PushDebugGroup(CommandList commandList, const char* name) override;
        void PopDebugGroup(CommandList commandList) override;
        void InsertDebugMarker(CommandList commandList, const char* name) override;

        void BeginRenderPass(CommandList commandList, const RenderPassDescription* renderPass) override;
        void EndRenderPass(CommandList commandList) override;

        void SetScissorRect(CommandList commandList, const RectI& scissorRect) override;
        void SetScissorRects(CommandList commandList, const RectI* scissorRects, uint32_t count) override;
        void SetViewport(CommandList commandList, const Viewport& viewport) override;
        void SetViewports(CommandList commandList, const Viewport* viewports, uint32_t count) override;
        void SetBlendColor(CommandList commandList, const Color& color) override;

        DWORD dxgiFactoryFlags;
        ComPtr<IDXGIFactory4> dxgiFactory;
        DXGIFactoryCaps dxgiFactoryCaps;

        D3D_FEATURE_LEVEL d3dMinFeatureLevel;

        ID3D12Device* d3dDevice = nullptr;
        D3D12MA::Allocator* allocator = nullptr;

        D3D_FEATURE_LEVEL d3dFeatureLevel = D3D_FEATURE_LEVEL_9_1;
        bool supportsRenderPass = false;
        bool isLost = false;
        bool shuttingDown = false;

        ID3D12CommandQueue* graphicsQueue;
        ID3D12CommandQueue* computeQueue;
        ID3D12CommandQueue* copyQueue;

        D3D12Fence frameFence;
        uint64 GPUFrameCount{ 0 };
        uint64 frameIndex{ 0 }; // frameCount % RenderLatency

        std::vector<IUnknown*> deferredReleases[kRenderLatency] = {};

        D3D12DescriptorHeap RTVHeap{};
        D3D12DescriptorHeap DSVHeap{};

        /* Command Lists */
        std::atomic<CommandList> commandListCount{ 0 };
        ID3D12GraphicsCommandList4* commandLists[kMaxCommandListCount] = {};
        std::vector<D3D12Texture*> renderPassTextures[kMaxCommandListCount] = {};
    };
}
