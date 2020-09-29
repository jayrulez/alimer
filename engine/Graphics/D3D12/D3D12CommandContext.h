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

#include "Graphics/CommandContext.h"
#include "D3D12Backend.h"

namespace Alimer
{
    class D3D12Texture;
    class D3D12CommandQueue;

    class D3D12CommandContext final : public CommandContext
    {
    public:
        D3D12CommandContext(D3D12GraphicsDevice* device, D3D12CommandQueue* queue);
        ~D3D12CommandContext() override;

        void Reset();
        void Flush(bool waitForCompletion) override;
        void PushDebugGroup(const char* name) override;
        void PopDebugGroup() override;
        void InsertDebugMarker(const char* name) override;

        void BeginRenderPass(const RenderPassDesc& renderPass) override;
        void EndRenderPass() override;

        void SetScissorRect(const RectI& scissorRect)  override;
        void SetScissorRects(const RectI* scissorRects, uint32_t count) override;
        void SetViewport(const Viewport& viewport) override;
        void SetViewports(const Viewport* viewports, uint32_t count) override;
        void SetBlendColor(const Color& color) override;

        //void BindBuffer(uint32_t slot, Buffer* buffer) override;
        //void BindBufferData(uint32_t slot, const void* data, uint32_t size) override;

        // Barriers 
        void TransitionResource(D3D12Texture* resource, TextureLayout newLayout, bool flushImmediate = false);
        void InsertUAVBarrier(D3D12GpuResource* resource, bool flushImmediate = false);
        void FlushResourceBarriers(void);

        ALIMER_FORCE_INLINE ID3D12GraphicsCommandList* GetCommandList() const
        {
            return commandList;
        }

    private:
        D3D12GraphicsDevice* device;
        D3D12CommandQueue* queue;

        bool useRenderPass;
        ID3D12CommandAllocator* currentAllocator = nullptr;
        ID3D12GraphicsCommandList* commandList = nullptr;
        ID3D12GraphicsCommandList4* commandList4 = nullptr;

        /* Barriers */
        static constexpr uint32_t kMaxResourceBarriers = 16;
        uint32_t numBarriersToFlush = 0;
        D3D12_RESOURCE_BARRIER resourceBarriers[kMaxResourceBarriers];

        D3D12_RENDER_PASS_RENDER_TARGET_DESC colorRenderPassTargets[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
        D3D12_CPU_DESCRIPTOR_HANDLE colorRTVS[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    };
}
