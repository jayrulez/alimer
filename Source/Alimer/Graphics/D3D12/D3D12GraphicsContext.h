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

#include "graphics/GraphicsContext.h"
#include "D3D12Backend.h"

namespace alimer
{
    class D3D12Texture;

    class D3D12GraphicsContext final : public GraphicsContext
    {
    public:
        D3D12GraphicsContext(D3D12GraphicsDevice* device, const GraphicsContextDescription& desc);
        ~D3D12GraphicsContext() override;
        void Destroy() override;

        void WaitForGPU();
        void Begin(const char* name, bool profile) override;
        void End() override;

        void Flush(bool wait) override;
        Texture* GetCurrentColorTexture() const override;
        void BeginRenderPass(const RenderPassDescriptor* descriptor) override;
        void EndRenderPass() override;
        void SetBlendColor(const Color& color) override;

        /* Barriers */
        void TransitionResource(D3D12GpuResource* resource, D3D12_RESOURCE_STATES newState, bool flushImmediate = false);
        void InsertUAVBarrier(D3D12GpuResource* resource, bool flushImmediate = false);
        void FlushResourceBarriers(void);

    private:
        void CreateRenderTargets();

        static const u64 kRenderLatency = 2;
        static constexpr u32 kNumBackBuffers = 2;

        D3D12GraphicsDevice* device;
        bool useRenderPass;
        DXGI_FORMAT dxgiColorFormat;

        /* Frame data */
        ID3D12CommandQueue* commandQueue = nullptr;
        ID3D12CommandAllocator* commandAllocators[kRenderLatency] = { };
        ID3D12GraphicsCommandList* commandList;
        ID3D12GraphicsCommandList4* commandList4 = nullptr;
        ID3D12Fence* fence;
        HANDLE fenceEvent;

        /// Whether a frame is active or not
        bool frameActive{ false };
        u64 currentCPUFrame{ 0 };
        u64 frameIndex{ 0 };

        /* SwapChain data */
        IDXGISwapChain3* swapChain = nullptr;
        u32 syncInterval = 1;
        u32 presentFlags = 0;
        u32 backbufferIndex = 0;
        D3D12Texture* colorTextures[kNumBackBuffers] = {};

        /* Barriers */
        static constexpr u32 kMaxResourceBarriers = 16;
        u32 numBarriersToFlush = 0;
        D3D12_RESOURCE_BARRIER resourceBarriers[kMaxResourceBarriers];

        D3D12_RENDER_PASS_RENDER_TARGET_DESC colorRenderPassTargets[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
        D3D12_CPU_DESCRIPTOR_HANDLE colorRTVS[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    };
}
