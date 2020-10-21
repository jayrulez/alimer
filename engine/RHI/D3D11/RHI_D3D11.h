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

#include "RHI/RHI.h"
#include "RHI/D3DCommon/RHI_D3D.h"
#define D3D11_NO_HELPERS
#include <d3d11_3.h>

#include <atomic>

namespace Alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    extern PFN_D3D11_CREATE_DEVICE D3D11CreateDevice;
#endif

    class GraphicsDevice_DX11 : public GraphicsDevice
    {
    private:
        D3D_DRIVER_TYPE driverType;
        D3D_FEATURE_LEVEL featureLevel;
        ComPtr<ID3D11Device> device;
        ComPtr<IDXGISwapChain1> swapChain;
        ComPtr<ID3D11RenderTargetView> renderTargetView;
        ComPtr<ID3D11Texture2D> backBuffer;
        ComPtr<ID3D11DeviceContext> immediateContext;
        ComPtr<ID3D11DeviceContext> deviceContexts[kCommanstListCount];
        ComPtr<ID3D11CommandList> commandLists[kCommanstListCount];
        ComPtr<ID3DUserDefinedAnnotation> userDefinedAnnotations[kCommanstListCount];

        uint32_t	stencilRef[kCommanstListCount];
        XMFLOAT4	blendFactor[kCommanstListCount];

        ID3D11VertexShader* prev_vs[kCommanstListCount] = {};
        ID3D11PixelShader* prev_ps[kCommanstListCount] = {};
        ID3D11HullShader* prev_hs[kCommanstListCount] = {};
        ID3D11DomainShader* prev_ds[kCommanstListCount] = {};
        ID3D11GeometryShader* prev_gs[kCommanstListCount] = {};
        ID3D11ComputeShader* prev_cs[kCommanstListCount] = {};
        XMFLOAT4 prev_blendfactor[kCommanstListCount] = {};
        uint32_t prev_samplemask[kCommanstListCount] = {};
        ID3D11BlendState* prev_bs[kCommanstListCount] = {};
        ID3D11RasterizerState* prev_rs[kCommanstListCount] = {};
        uint32_t prev_stencilRef[kCommanstListCount] = {};
        ID3D11DepthStencilState* prev_dss[kCommanstListCount] = {};
        ID3D11InputLayout* prev_il[kCommanstListCount] = {};
        PrimitiveTopology prev_pt[kCommanstListCount] = {};

        const PipelineState* active_pso[kCommanstListCount] = {};
        bool dirty_pso[kCommanstListCount] = {};
        void pso_validate(CommandList cmd);

        const RenderPass* active_renderpass[kCommanstListCount] = {};

        ID3D11UnorderedAccessView* raster_uavs[kCommanstListCount][8] = {};
        uint8_t raster_uavs_slot[kCommanstListCount] = {};
        uint8_t raster_uavs_count[kCommanstListCount] = {};

        struct GPUAllocator
        {
            RefPtr<GraphicsBuffer> buffer;
            size_t byteOffset = 0;
            uint64_t residentFrame = 0;
            bool dirty = false;
        } frame_allocators[kCommanstListCount];
        void commit_allocations(CommandList cmd);

        void CreateBackBufferResources();

        std::atomic<CommandList> cmd_count{ 0 };

        std::unordered_map<size_t, ComPtr<ID3D11BlendState>> blendStateCache;
        std::unordered_map<size_t, ComPtr<ID3D11RasterizerState>> rasterizerStateCache;
        std::unordered_map<size_t, ComPtr<ID3D11DepthStencilState>> depthStencilStateCache;

        struct EmptyResourceHandle {}; // only care about control-block
        std::shared_ptr<EmptyResourceHandle> emptyresource;

    public:
        static bool IsAvailable();
        GraphicsDevice_DX11(void* window, bool fullscreen, bool enableDebugLayer_);
        ~GraphicsDevice_DX11() override;

        RefPtr<GraphicsBuffer> CreateBuffer(const GPUBufferDesc& desc, const void* initialData) override;
        bool CreateTexture(const TextureDesc* pDesc, const SubresourceData* pInitialData, Texture* pTexture) override;
        bool CreateShader(ShaderStage stage, const void* pShaderBytecode, size_t BytecodeLength, Shader* pShader) override;
        bool CreateShader(ShaderStage stage, const char* source, const char* entryPoint, Shader* pShader) override;
        bool CreateBlendState(const BlendStateDesc* pBlendStateDesc, BlendState* pBlendState) override;
        ID3D11DepthStencilState* GetDepthStencilState(const DepthStencilStateDescriptor& descriptor);
        ID3D11RasterizerState* GetRasterizerState(const RasterizationStateDescriptor& descriptor, uint32_t sampleCount);
        bool CreateSampler(const SamplerDescriptor* descriptor, Sampler* pSamplerState) override;
        bool CreateQuery(const GPUQueryDesc* pDesc, GPUQuery* pQuery) override;
        bool CreatePipelineStateCore(const PipelineStateDesc* pDesc, PipelineState* pso) override;
        bool CreateRenderPass(const RenderPassDesc* pDesc, RenderPass* renderpass) override;

        int CreateSubresource(Texture* texture, SUBRESOURCE_TYPE type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount) override;
        int CreateSubresource(GraphicsBuffer* buffer, SUBRESOURCE_TYPE type, uint64_t offset, uint64_t size = ~0) override;

        void Map(const GPUResource* resource, Mapping* mapping) override;
        void Unmap(const GPUResource* resource) override;
        bool QueryRead(const GPUQuery* query, GPUQueryResult* result) override;

        void SetName(GPUResource* pResource, const char* name) override;

        void PresentBegin(CommandList cmd) override;
        void PresentEnd(CommandList cmd) override;

        void WaitForGPU() override;

        CommandList BeginCommandList() override;
        void SubmitCommandLists() override;

        void SetResolution(int width, int height) override;

        Texture GetBackBuffer() override;

        ///////////////Thread-sensitive////////////////////////

        void RenderPassBegin(const RenderPass* renderpass, CommandList cmd) override;
        void RenderPassEnd(CommandList cmd) override;
        void BindScissorRects(uint32_t numRects, const Rect* rects, CommandList cmd) override;
        void BindViewports(uint32_t NumViewports, const Viewport* pViewports, CommandList cmd) override;
        void BindResource(ShaderStage stage, const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource = -1) override;
        void BindResources(ShaderStage stage, const GPUResource* const* resources, uint32_t slot, uint32_t count, CommandList cmd) override;
        void BindUAV(ShaderStage stage, const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource = -1) override;
        void BindUAVs(ShaderStage stage, const GPUResource* const* resources, uint32_t slot, uint32_t count, CommandList cmd) override;
        void UnbindResources(uint32_t slot, uint32_t num, CommandList cmd) override;
        void UnbindUAVs(uint32_t slot, uint32_t num, CommandList cmd) override;
        void BindSampler(ShaderStage stage, const Sampler* sampler, uint32_t slot, CommandList cmd) override;
        void BindConstantBuffer(ShaderStage stage, const GraphicsBuffer* buffer, uint32_t slot, CommandList cmd) override;
        void BindVertexBuffers(const GraphicsBuffer* const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint32_t* offsets, CommandList cmd) override;
        void BindIndexBuffer(const GraphicsBuffer* indexBuffer, IndexFormat format, uint32_t offset, CommandList cmd) override;
        void BindStencilRef(uint32_t value, CommandList cmd) override;
        void BindBlendFactor(float r, float g, float b, float a, CommandList cmd) override;
        void BindPipelineState(const PipelineState* pso, CommandList cmd) override;
        void BindComputeShader(const Shader* cs, CommandList cmd) override;
        void Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd) override;
        void DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, CommandList cmd) override;
        void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd) override;
        void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd) override;
        void DrawInstancedIndirect(const GraphicsBuffer* args, uint32_t args_offset, CommandList cmd) override;
        void DrawIndexedInstancedIndirect(const GraphicsBuffer* args, uint32_t args_offset, CommandList cmd) override;
        void Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd) override;
        void DispatchIndirect(const GraphicsBuffer* args, uint32_t args_offset, CommandList cmd) override;
        void CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd) override;
        void UpdateBuffer(CommandList cmd, GraphicsBuffer* buffer, const void* data, uint64_t size) override;
        void QueryBegin(const GPUQuery* query, CommandList cmd) override;
        void QueryEnd(const GPUQuery* query, CommandList cmd) override;
        void Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd) override {}

        GPUAllocation AllocateGPU(size_t dataSize, CommandList cmd) override;

        void PushDebugGroup(CommandList cmd, const char* name) override;
        void PopDebugGroup(CommandList cmd) override;
        void InsertDebugMarker(CommandList cmd, const char* name) override;
    };
}
