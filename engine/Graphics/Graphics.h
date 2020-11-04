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
// The implementation is based on WickedEngine graphics code, MIT license (https://github.com/turanszkij/WickedEngine/blob/master/LICENSE.md)

#pragma once

#include "Core/Object.h"
#include "Graphics/GraphicsResource.h"
#include "Platform/WindowHandle.h"
#include <memory>
#include <set>

namespace alimer
{
    class ALIMER_API GraphicsBuffer : public GraphicsResource, public RefCounted
    {
    public:
        const GPUBufferDesc& GetDesc() const
        {
            return desc;
        }

    protected:
        GraphicsBuffer(const GPUBufferDesc& desc)
            : GraphicsResource(Type::Buffer)
            , desc{desc}
        {
        }

        GPUBufferDesc desc;
    };

    class ALIMER_API Sampler : public GraphicsResource, public RefCounted
    {
    public:
    protected:
        Sampler()
            : GraphicsResource(Type::Sampler)
        {
        }
    };

    class ALIMER_API RenderPipeline : public GraphicsResource, public RefCounted
    {
    protected:
        RenderPipeline()
            : GraphicsResource(Type::RenderPipeline)
        {
        }
    };

    struct GPUAllocation
    {
        void* data = nullptr; // application can write to this. Reads might be not supported or slow. The offset is already applied
        const GraphicsBuffer* buffer = nullptr; // application can bind it to the GPU
        uint32_t offset = 0; // allocation's offset from the GPUbuffer's beginning

        // Returns true if the allocation was successful
        inline bool IsValid() const
        {
            return data != nullptr && buffer != nullptr;
        }
    };

    class ALIMER_API CommandList
    {
    public:
        virtual ~CommandList() = default;

        virtual void PresentBegin() = 0;
        virtual void PresentEnd() = 0;

        virtual void PushDebugGroup(const char* name) = 0;
        virtual void PopDebugGroup() = 0;
        virtual void InsertDebugMarker(const char* name) = 0;

        // Allocates temporary memory that the CPU can write and GPU can read.
        //	It is only alive for one frame and automatically invalidated after that.
        //	The CPU pointer gets invalidated as soon as there is a Draw() or Dispatch() event on the same thread
        //	This allocation can be used to provide temporary vertex buffer, index buffer or raw buffer data to shaders
        virtual GPUAllocation AllocateGPU(const uint32_t size) = 0;
        virtual void CopyResource(const GPUResource* pDst, const GPUResource* pSrc) = 0;
        virtual void UpdateBuffer(GraphicsBuffer* buffer, const void* data, uint64_t size = 0) = 0;

        virtual void RenderPassBegin(const RenderPass* renderpass) = 0;
        virtual void RenderPassEnd() = 0;
        virtual void SetViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f) = 0;
        virtual void SetViewport(const Viewport& viewport) = 0;
        virtual void SetViewports(uint32_t viewportCount, const Viewport* pViewports) = 0;
        virtual void SetScissorRect(const ScissorRect& rect) = 0;
        virtual void SetScissorRects(uint32_t scissorCount, const ScissorRect* rects) = 0;
        virtual void BindResource(ShaderStage stage, const GPUResource* resource, uint32_t slot, int subresource = -1) = 0;
        virtual void BindResources(ShaderStage stage, const GPUResource* const* resources, uint32_t slot, uint32_t count) = 0;
        virtual void BindUAV(ShaderStage stage, const GPUResource* resource, uint32_t slot, int subresource = -1) = 0;
        virtual void BindUAVs(ShaderStage stage, const GPUResource* const* resources, uint32_t slot, uint32_t count) = 0;
        virtual void BindSampler(ShaderStage stage, const Sampler* sampler, uint32_t slot) = 0;
        virtual void BindConstantBuffer(ShaderStage stage, const GraphicsBuffer* buffer, uint32_t slot) = 0;
        virtual void BindVertexBuffers(const GraphicsBuffer* const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint32_t* offsets) = 0;
        virtual void BindIndexBuffer(const GraphicsBuffer* indexBuffer, IndexFormat format, uint32_t offset) = 0;

        virtual void BindStencilRef(uint32_t value) = 0;
        virtual void BindBlendFactor(float r, float g, float b, float a) = 0;
        virtual void BindShadingRate(ShadingRate rate) {}
        virtual void BindShadingRateImage(const Texture* texture)
        {
        }

        virtual void SetRenderPipeline(RenderPipeline* pipeline) = 0;
        virtual void BindComputeShader(const Shader* shader) = 0;
        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t baseVertex = 0, uint32_t firstInstance = 0) = 0;
        virtual void DrawInstancedIndirect(const GraphicsBuffer* args, uint32_t args_offset) = 0;
        virtual void DrawIndexedInstancedIndirect(const GraphicsBuffer* args, uint32_t args_offset) = 0;

        virtual void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
        virtual void DispatchIndirect(const GraphicsBuffer* args, uint32_t args_offset) = 0;
        virtual void DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) {}
        virtual void DispatchMeshIndirect(const GraphicsBuffer* args, uint32_t args_offset)
        {
        }

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

    struct GraphicsSettings final
    {
        std::string applicationName = "Alimer";
        GraphicsDeviceFlags flags = GraphicsDeviceFlags::None;
        GraphicsBackendType backendType = GraphicsBackendType::Count;
        PixelFormat backbufferFormat = PixelFormat::RGB10A2Unorm;
        PixelFormat depthStencilFormat = PixelFormat::Depth32Float;
        bool verticalSync = false;
        bool fullscreen = false;
    };

    class Graphics : public Object
    {
        ALIMER_OBJECT(Graphics, Object);

    public:
        static constexpr uint32_t BACKBUFFER_COUNT = 2;

    protected:
        uint64_t FRAMECOUNT = 0;
        uint64_t frameIndex = 0;

        PixelFormat BACKBUFFER_FORMAT = PixelFormat::RGB10A2Unorm;
        bool TESSELLATION = false;
        bool CONSERVATIVE_RASTERIZATION = false;
        bool RASTERIZER_ORDERED_VIEWS = false;
        bool UAV_LOAD_FORMAT_COMMON = false;
        bool UAV_LOAD_FORMAT_R11G11B10_FLOAT = false;
        bool RENDERTARGET_AND_VIEWPORT_ARRAYINDEX_WITHOUT_GS = false;
        bool RAYTRACING = false;
        bool RAYTRACING_INLINE = false;
        bool DESCRIPTOR_MANAGEMENT = false;
        bool VARIABLE_RATE_SHADING = false;
        bool VARIABLE_RATE_SHADING_TIER2 = false;
        bool MESH_SHADER = false;
        size_t SHADER_IDENTIFIER_SIZE = 0;
        size_t TOPLEVEL_ACCELERATION_STRUCTURE_INSTANCE_SIZE = 0;
        uint32_t VARIABLE_RATE_SHADING_TILE_SIZE = 0;

    public:
        Graphics(WindowHandle window, const GraphicsSettings& desc);
        virtual ~Graphics() = default;
        
        static std::set<GraphicsBackendType> GetAvailableBackends();
        static RefPtr<Graphics> Create(WindowHandle windowHandle, const GraphicsSettings& desc);

        virtual RefPtr<GraphicsBuffer> CreateBuffer(const GPUBufferDesc& desc, const void* initialData = nullptr) = 0;
        virtual bool CreateTexture(const TextureDesc* pDesc, const SubresourceData* pInitialData, Texture* pTexture) = 0;
        virtual bool CreateShader(ShaderStage stage, const void* pShaderBytecode, size_t BytecodeLength, Shader* pShader) = 0;
        virtual bool CreateShader(ShaderStage stage, const char* source, const char* entryPoint, Shader* pShader) = 0;
        virtual RefPtr<Sampler> CreateSampler(const SamplerDescriptor* descriptor) = 0;
        virtual bool CreateQuery(const GPUQueryDesc* pDesc, GPUQuery* pQuery) = 0;
        RefPtr<RenderPipeline> CreateRenderPipeline(const RenderPipelineDescriptor* descriptor);
        virtual bool CreateRenderPass(const RenderPassDesc* pDesc, RenderPass* renderpass) = 0;
        virtual bool CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* pDesc, RaytracingAccelerationStructure* bvh)
        {
            return false;
        }
        virtual bool CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* pDesc, RaytracingPipelineState* rtpso)
        {
            return false;
        }
        virtual bool CreateDescriptorTable(DescriptorTable* table)
        {
            return false;
        }
        virtual bool CreateRootSignature(RootSignature* rootsig)
        {
            return false;
        }

        virtual int CreateSubresource(Texture* texture, SUBRESOURCE_TYPE type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount) = 0;
        virtual int CreateSubresource(GraphicsBuffer* buffer, SUBRESOURCE_TYPE type, uint64_t offset, uint64_t size = ~0) = 0;

        virtual void WriteShadingRateValue(ShadingRate rate, void* dest){};
        virtual void WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest)
        {
        }
        virtual void WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest)
        {
        }
        virtual void WriteDescriptor(const DescriptorTable* table, uint32_t rangeIndex, uint32_t arrayIndex, const GPUResource* resource, int subresource = -1, uint64_t offset = 0)
        {
        }
        virtual void WriteDescriptor(const DescriptorTable* table, uint32_t rangeIndex, uint32_t arrayIndex, const Sampler* sampler)
        {
        }

        virtual void Map(const GPUResource* resource, Mapping* mapping) = 0;
        virtual void Unmap(const GPUResource* resource) = 0;
        virtual bool QueryRead(const GPUQuery* query, GPUQueryResult* result) = 0;

        virtual void SetName(GPUResource* pResource, const char* name) = 0;

        virtual CommandList& BeginCommandList() = 0;
        virtual void SubmitCommandLists() = 0;

        virtual void WaitForGPU() = 0;
        virtual void ClearPipelineStateCache(){};

        inline bool GetVSyncEnabled() const
        {
            return verticalSync;
        }
        virtual void SetVSyncEnabled(bool value)
        {
            verticalSync = value;
        }
        inline uint64_t GetFrameCount() const
        {
            return FRAMECOUNT;
        }
        inline uint64_t GetFrameIndex() const
        {
            return GetFrameCount() % BACKBUFFER_COUNT;
        }

        // Returns native resolution width of back buffer in pixels:
        inline uint32_t GetResolutionWidth() const
        {
            return backbufferWidth;
        }
        // Returns native resolution height of back buffer in pixels:
        inline uint32_t GetResolutionHeight() const
        {
            return backbufferHeight;
        }

        // Returns the width of the screen with DPI scaling applied (subpixel size):
        float GetScreenWidth() const;
        // Returns the height of the screen with DPI scaling applied (subpixel size):
        float GetScreenHeight() const;

        virtual void Resize(uint32_t width, uint32_t height) = 0;

        virtual Texture GetBackBuffer() = 0;

        bool CheckCapability(GRAPHICSDEVICE_CAPABILITY capability) const;

        bool IsFormatUnorm(PixelFormat value) const;
        bool IsFormatBlockCompressed(PixelFormat value) const;
        bool IsFormatStencilSupport(PixelFormat value) const;

        inline XMMATRIX GetScreenProjection() const
        {
            return XMMatrixOrthographicOffCenterLH(0, (float) GetScreenWidth(), (float) GetScreenHeight(), 0, -1, 1);
        }
        inline PixelFormat GetBackBufferFormat() const
        {
            return BACKBUFFER_FORMAT;
        }
        static constexpr uint32_t GetBackBufferCount()
        {
            return BACKBUFFER_COUNT;
        }

        inline size_t GetShaderIdentifierSize() const
        {
            return SHADER_IDENTIFIER_SIZE;
        }
        inline size_t GetTopLevelAccelerationStructureInstanceSize() const
        {
            return TOPLEVEL_ACCELERATION_STRUCTURE_INSTANCE_SIZE;
        }
        inline uint32_t GetVariableRateShadingTileSize() const
        {
            return VARIABLE_RATE_SHADING_TILE_SIZE;
        }

    protected:
        virtual bool CreateRenderPipelineCore(const RenderPipelineDescriptor* descriptor, RenderPipeline** pipeline) = 0;

        uint32_t backbufferWidth;
        uint32_t backbufferHeight;
        bool verticalSync = true;
    };

    bool StencilTestEnabled(const DepthStencilStateDescriptor* descriptor);
}
