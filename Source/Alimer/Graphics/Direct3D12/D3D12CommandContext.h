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

#include "graphics/CommandContext.h"
#include "D3D12Backend.h"

#define VALID_COMPUTE_QUEUE_RESOURCE_STATES \
    ( D3D12_RESOURCE_STATE_UNORDERED_ACCESS \
    | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE \
    | D3D12_RESOURCE_STATE_COPY_DEST \
    | D3D12_RESOURCE_STATE_COPY_SOURCE )

namespace alimer
{
    class D3D12Texture;

    class D3D12CommandContext final : public CommandContext
    {
    public:
        D3D12CommandContext(D3D12GraphicsDevice* device, D3D12_COMMAND_LIST_TYPE type_, const std::string& id_);
        ~D3D12CommandContext();
        void Destroy();

        void Reset();

        void BeginRenderPass(const RenderPassDescriptor* descriptor) override;
        void EndRenderPass() override;
        void SetBlendColor(const Color& color) override;
        void Flush(bool wait = false) override;

        /* Barriers */
        void TransitionResource(D3D12GpuResource* resource, D3D12_RESOURCE_STATES newState, bool flushImmediate = false);
        void InsertUAVBarrier(D3D12GpuResource* resource, bool flushImmediate = false);
        void FlushResourceBarriers(void);

        ID3D12GraphicsCommandList* GetCommandList() const { return commandList; }
        ID3D12GraphicsCommandList4* GetCommandList4() const { return commandList4; }

    private:
        const D3D12_COMMAND_LIST_TYPE type;

        ID3D12CommandAllocator* currentAllocator;
        ID3D12GraphicsCommandList* commandList;
        ID3D12GraphicsCommandList4* commandList4 = nullptr;
        bool useRenderPass;
        bool isProfiling = false;

        /* Barriers */
        static constexpr u32 kMaxResourceBarriers = 16;
        u32 numBarriersToFlush = 0;
        D3D12_RESOURCE_BARRIER resourceBarriers[kMaxResourceBarriers];

        D3D12_RENDER_PASS_RENDER_TARGET_DESC colorRenderPassTargets[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};

        D3D12Texture* swapchainTexture = nullptr;
        D3D12_CPU_DESCRIPTOR_HANDLE colorRTVS[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    };
}
