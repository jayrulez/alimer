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

#include "graphics/GraphicsImpl.h"
#include "D3D12Backend.h"

#define VALID_COMPUTE_QUEUE_RESOURCE_STATES \
    ( D3D12_RESOURCE_STATE_UNORDERED_ACCESS \
    | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE \
    | D3D12_RESOURCE_STATE_COPY_DEST \
    | D3D12_RESOURCE_STATE_COPY_SOURCE )

namespace alimer
{
    namespace d3d12
    {
        struct Resource;
    }

    class D3D12GraphicsContext final : public GpuCommandBuffer
    {
    public:
        D3D12GraphicsContext(D3D12GraphicsDevice& device_, QueueType type_);
        void Destroy();

        void Reset();
        void BeginMarker(const char* name) override;
        void EndMarker() override;
        void Flush(bool wait) override;
        void TransitionResource(d3d12::Resource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate = false);
        void FlushResourceBarriers(void);

        void BeginRenderPass(GPUTexture texture, const Color& clearColor) override;
        void EndRenderPass() override;

        ID3D12GraphicsCommandList* GetCommandList() const
        {
            return commandList;
        }

    private:
        D3D12GraphicsDevice& device;
        const QueueType type;

        ID3D12CommandAllocator* currentAllocator;
        ID3D12GraphicsCommandList* commandList;
        bool isProfiling = false;

        static constexpr uint32_t kMaxResourceBarriers = 16;

        D3D12_RESOURCE_BARRIER resourceBarriers[kMaxResourceBarriers];
        UINT numBarriersToFlush = 0;
    };
}
