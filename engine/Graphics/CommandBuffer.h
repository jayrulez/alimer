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

#include "Graphics/GraphicsResource.h"
#include <string>

namespace alimer
{
    class ALIMER_API CommandBuffer
    {
    public:
        virtual ~CommandBuffer() = default;

        virtual void PresentBegin() = 0;
        virtual void PresentEnd() = 0;

        virtual void PushDebugGroup(const char* name) = 0;
        virtual void PopDebugGroup() = 0;
        virtual void InsertDebugMarker(const char* name) = 0;

        // Allocates temporary memory that the CPU can write and GPU can read.
        //	It is only alive for one frame and automatically invalidated after that.
        //	The CPU pointer gets invalidated as soon as there is a Draw() or Dispatch() event on the same thread
        //	This allocation can be used to provide temporary vertex buffer, index buffer or raw buffer data to shaders
        virtual GPUAllocation AllocateGPU(const uint64_t size) = 0;
        virtual void CopyResource(const GPUResource* pDst, const GPUResource* pSrc) = 0;
        virtual void UpdateBuffer(GraphicsBuffer* buffer, const void* data, uint64_t size = 0) = 0;

        virtual void RenderPassBegin(const RenderPass* renderpass) = 0;
        virtual void RenderPassEnd() = 0;
        virtual void SetViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f) = 0;
        virtual void SetViewport(const Viewport& viewport) = 0;
        virtual void SetViewports(uint32_t viewportCount, const Viewport* pViewports) = 0;
        virtual void SetScissorRect(const ScissorRect& rect) = 0;
        virtual void SetScissorRects(uint32_t scissorCount, const ScissorRect* rects) = 0;
        virtual void BindResource(ShaderStage stage, const GraphicsResource* resource, uint32_t slot, int subresource = -1) = 0;
        virtual void BindResources(ShaderStage stage, const GraphicsResource* const* resources, uint32_t slot, uint32_t count) = 0;
        virtual void BindUAV(ShaderStage stage, const GPUResource* resource, uint32_t slot, int subresource = -1) = 0;
        virtual void BindUAVs(ShaderStage stage, const GPUResource* const* resources, uint32_t slot, uint32_t count) = 0;
        virtual void BindSampler(ShaderStage stage, const Sampler* sampler, uint32_t slot) = 0;
        virtual void BindConstantBuffer(ShaderStage stage, const GraphicsBuffer* buffer, uint32_t slot) = 0;
        virtual void BindVertexBuffers(const GraphicsBuffer* const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint32_t* offsets) = 0;
        virtual void BindIndexBuffer(const GraphicsBuffer* indexBuffer, IndexFormat format, uint32_t offset) = 0;

        virtual void BindStencilRef(uint32_t value) = 0;
        virtual void BindBlendFactor(float r, float g, float b, float a) = 0;
        virtual void BindShadingRate(ShadingRate rate) {}
        virtual void BindShadingRateImage(const Texture* texture) {}

        virtual void SetRenderPipeline(RenderPipeline* pipeline) = 0;
        virtual void BindComputeShader(const Shader* shader) = 0;
        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t baseVertex = 0, uint32_t firstInstance = 0) = 0;
        virtual void DrawInstancedIndirect(const GraphicsBuffer* args, uint32_t args_offset) = 0;
        virtual void DrawIndexedInstancedIndirect(const GraphicsBuffer* args, uint32_t args_offset) = 0;

        virtual void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
        virtual void DispatchIndirect(const GraphicsBuffer* args, uint32_t args_offset) = 0;
        virtual void DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) {}
        virtual void DispatchMeshIndirect(const GraphicsBuffer* args, uint32_t args_offset) {}

        virtual void QueryBegin(const GPUQuery* query) = 0;
        virtual void QueryEnd(const GPUQuery* query) = 0;
        virtual void Barrier(const GPUBarrier* barriers, uint32_t numBarriers) = 0;
        virtual void BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructure* dst, const RaytracingAccelerationStructure* src = nullptr)
        {
        }
        virtual void BindRaytracingPipelineState(const RaytracingPipelineState* rtpso)
        {
        }
        virtual void DispatchRays(const DispatchRaysDesc* desc)
        {
        }

        virtual void BindDescriptorTable(PipelineBindPoint bindPoint, uint32_t space, const DescriptorTable* table)
        {
        }
        virtual void BindRootDescriptor(PipelineBindPoint bindPoint, uint32_t index, const GraphicsBuffer* buffer, uint32_t offset)
        {
        }
        virtual void BindRootConstants(PipelineBindPoint bindPoint, uint32_t index, const void* srcData)
        {
        }
    };
}
