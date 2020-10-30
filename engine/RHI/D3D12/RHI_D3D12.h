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


#include "AlimerConfig.h"
#include "RHI/RHI.h"
#include "RHI/D3D/RHI_D3D.h"
#include "RHI/RHI_Internal.h"

// Forward declare memory allocator classes
namespace D3D12MA
{
    class Allocator;
    class Allocation;
};

#include <d3d12.h>
#include <wrl/client.h> // ComPtr

#include <unordered_map>
#include <deque>
#include <atomic>
#include <mutex>

namespace Alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    using PFN_DXC_CREATE_INSTANCE = HRESULT(WINAPI*)(REFCLSID rclsid, REFIID riid, _COM_Outptr_ void** ppCompiler);

    extern PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface;
    extern PFN_D3D12_CREATE_DEVICE D3D12CreateDevice;
    extern PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignature;
    extern PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER D3D12CreateRootSignatureDeserializer;
    extern PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE D3D12SerializeVersionedRootSignature;
    extern PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER D3D12CreateVersionedRootSignatureDeserializer;
    extern PFN_DXC_CREATE_INSTANCE DxcCreateInstance;
#endif

    class D3D12_CommandList;
    class GraphicsDevice_DX12;

    class GraphicsDevice_DX12 final : public GraphicsDevice
    {
        friend class D3D12_CommandList;

    private:
        D3D12_FEATURE_DATA_D3D12_OPTIONS features_0;
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 features_5;
        D3D12_FEATURE_DATA_D3D12_OPTIONS6 features_6;
        D3D12_FEATURE_DATA_D3D12_OPTIONS7 features_7;

        std::mutex copyQueueLock;
        bool copyQueueUse = false;
        ID3D12Fence* copyFence = nullptr; // GPU only


        struct FrameResources
        {
            ID3D12CommandQueue* copyQueue = nullptr;
            ID3D12CommandAllocator* copyAllocator = nullptr;
            ID3D12GraphicsCommandList* copyCommandList = nullptr;

            struct DescriptorTableFrameAllocator
            {
                GraphicsDevice_DX12* device = nullptr;
                struct DescriptorHeap
                {
                    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
                    ID3D12DescriptorHeap* heap_GPU;
                    D3D12_CPU_DESCRIPTOR_HANDLE start_cpu = {};
                    D3D12_GPU_DESCRIPTOR_HANDLE start_gpu = {};
                    uint32_t ringOffset = 0;
                };
                std::vector<DescriptorHeap> heaps_resource;
                std::vector<DescriptorHeap> heaps_sampler;
                uint32_t current_resource_heap = 0;
                uint32_t current_sampler_heap = 0;
                bool heaps_bound = false;
                bool dirty = false;

                const GraphicsBuffer* CBV[GPU_RESOURCE_HEAP_CBV_COUNT];
                const GPUResource* SRV[GPU_RESOURCE_HEAP_SRV_COUNT];
                int SRV_index[GPU_RESOURCE_HEAP_SRV_COUNT];
                const GPUResource* UAV[GPU_RESOURCE_HEAP_UAV_COUNT];
                int UAV_index[GPU_RESOURCE_HEAP_UAV_COUNT];
                const Sampler* SAM[GPU_SAMPLER_HEAP_COUNT];

                struct DescriptorHandles
                {
                    D3D12_GPU_DESCRIPTOR_HANDLE sampler_handle = {};
                    D3D12_GPU_DESCRIPTOR_HANDLE resource_handle = {};
                };

                void init(GraphicsDevice_DX12* device);
                void shutdown();

                void reset();
                void request_heaps(uint32_t resources, uint32_t samplers, D3D12_CommandList* cmd);
                void validate(bool graphics, D3D12_CommandList* cmd);
                DescriptorHandles commit(const DescriptorTable* table, D3D12_CommandList* cmd);
            };
            DescriptorTableFrameAllocator descriptors[kCommandListCount];

            struct ResourceFrameAllocator
            {
                GraphicsDevice_DX12* device = nullptr;
                RefPtr<GraphicsBuffer>buffer;
                uint8_t* dataBegin = nullptr;
                uint8_t* dataCur = nullptr;
                uint8_t* dataEnd = nullptr;

                void init(GraphicsDevice_DX12* device, size_t size);

                uint8_t* allocate(size_t dataSize, size_t alignment);
                void clear();
                uint64_t calculateOffset(uint8_t* address);
            };
            ResourceFrameAllocator resourceBuffer[kCommandListCount];
        };
        FrameResources frames[BACKBUFFER_COUNT];
        FrameResources& GetFrameResources() { return frames[GetFrameCount() % BACKBUFFER_COUNT]; }

        D3D12_CommandList* commandLists[kCommandListCount] = {};

        //void predispatch(CommandList cmd);
        //void preraytrace(CommandList cmd);

        struct Query_Resolve
        {
            GPU_QUERY_TYPE type;
            UINT index;
        };
        std::vector<Query_Resolve> query_resolves[kCommandListCount] = {};

        std::atomic_uint32_t commandListsCount{ 0 };

    public:
        static bool IsAvailable();
        GraphicsDevice_DX12(WindowHandle window, const Desc& desc, D3D_FEATURE_LEVEL minFeatureLevel = D3D_FEATURE_LEVEL_11_0);
        ~GraphicsDevice_DX12() override;

        void GetAdapter(IDXGIAdapter1** ppAdapter);
        RefPtr<GraphicsBuffer> CreateBuffer(const GPUBufferDesc& desc, const void* initialData) override;
        bool CreateTexture(const TextureDesc* pDesc, const SubresourceData* pInitialData, Texture* pTexture) override;
        bool CreateShader(ShaderStage stage, const void* pShaderBytecode, size_t BytecodeLength, Shader* pShader) override;
        bool CreateShader(ShaderStage stage, const char* source, const char* entryPoint, Shader* pShader) override;
        RefPtr<Sampler> CreateSampler(const SamplerDescriptor* descriptor) override;
        bool CreateQuery(const GPUQueryDesc* pDesc, GPUQuery* pQuery) override;
        bool CreateRenderPipelineCore(const RenderPipelineDescriptor* descriptor, RenderPipeline** renderPipeline) override;
        bool CreateRenderPass(const RenderPassDesc* pDesc, RenderPass* renderpass) override;
        bool CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* pDesc, RaytracingAccelerationStructure* bvh) override;
        bool CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* pDesc, RaytracingPipelineState* rtpso) override;
        bool CreateDescriptorTable(DescriptorTable* table) override;
        bool CreateRootSignature(RootSignature* rootsig) override;

        int CreateSubresource(Texture* texture, SUBRESOURCE_TYPE type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount) override;
        int CreateSubresource(GraphicsBuffer* buffer, SUBRESOURCE_TYPE type, uint64_t offset, uint64_t size = ~0) override;

        void WriteShadingRateValue(ShadingRate rate, void* dest) override;
        void WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest) override;
        void WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest) override;
        void WriteDescriptor(const DescriptorTable* table, uint32_t rangeIndex, uint32_t arrayIndex, const GPUResource* resource, int subresource = -1, uint64_t offset = 0) override;
        void WriteDescriptor(const DescriptorTable* table, uint32_t rangeIndex, uint32_t arrayIndex, const Sampler* sampler) override;

        void Map(const GPUResource* resource, Mapping* mapping) override;
        void Unmap(const GPUResource* resource) override;
        bool QueryRead(const GPUQuery* query, GPUQueryResult* result) override;

        void SetName(GPUResource* pResource, const char* name) override;

        CommandList& BeginCommandList() override;
        void SubmitCommandLists() override;

        void WaitForGPU() override;
        void ClearPipelineStateCache() override;

        void Resize(uint32_t width, uint32_t height) override;

        void PresentBegin(ID3D12GraphicsCommandList6* commandList);
        void PresentEnd(ID3D12GraphicsCommandList6* commandList);

        Texture GetBackBuffer() override;

        ///////////////Thread-sensitive////////////////////////

#if TODO
        
        void Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd) override;
        void DispatchIndirect(const GraphicsBuffer* args, uint32_t args_offset, CommandList cmd) override;
        void DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd) override;
        void DispatchMeshIndirect(const GraphicsBuffer* args, uint32_t args_offset, CommandList cmd) override;
        void CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd) override;
        void QueryBegin(const GPUQuery* query, CommandList cmd) override;
        void QueryEnd(const GPUQuery* query, CommandList cmd) override;
        void Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd) override;
        void BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructure* dst, CommandList cmd, const RaytracingAccelerationStructure* src = nullptr) override;
        void BindRaytracingPipelineState(const RaytracingPipelineState* rtpso, CommandList cmd) override;
        void DispatchRays(const DispatchRaysDesc* desc, CommandList cmd) override;

        void BindDescriptorTable(BINDPOINT bindpoint, uint32_t space, const DescriptorTable* table, CommandList cmd) override;
        void BindRootDescriptor(BINDPOINT bindpoint, uint32_t index, const GraphicsBuffer* buffer, uint32_t offset, CommandList cmd) override;
        void BindRootConstants(BINDPOINT bindpoint, uint32_t index, const void* srcdata, CommandList cmd) override;

#endif // TODO


    private:
        D3D_FEATURE_LEVEL minFeatureLevel;

        UINT dxgiFactoryFlags = 0;
        ComPtr<IDXGIFactory4> dxgiFactory4;
        bool isTearingSupported = false;

        ID3D12Device5* device = nullptr;
        ID3D12CommandQueue* directQueue;
        ID3D12Fence* frameFence;
        HANDLE frameFenceEvent;

        IDXGISwapChain3* swapChain;
        uint32_t backbufferIndex = 0;
        ID3D12Resource* backBuffers[BACKBUFFER_COUNT];

        ID3D12CommandSignature* dispatchIndirectCommandSignature = nullptr;
        ID3D12CommandSignature* drawInstancedIndirectCommandSignature = nullptr;
        ID3D12CommandSignature* drawIndexedInstancedIndirectCommandSignature = nullptr;
        ID3D12CommandSignature* dispatchMeshIndirectCommandSignature = nullptr;

        static constexpr size_t timestamp_query_count = 1024;
        static constexpr size_t occlusion_query_count = 1024;
        ID3D12QueryHeap* querypool_timestamp = nullptr;
        ID3D12QueryHeap* querypool_occlusion = nullptr;
        ID3D12Resource* querypool_timestamp_readback = nullptr;
        ID3D12Resource* querypool_occlusion_readback = nullptr;
        D3D12MA::Allocation* allocation_querypool_timestamp_readback = nullptr;
        D3D12MA::Allocation* allocation_querypool_occlusion_readback = nullptr;

        uint32_t rtv_descriptor_size = 0;
        uint32_t dsv_descriptor_size = 0;
        uint32_t resource_descriptor_size = 0;
        uint32_t sampler_descriptor_size = 0;

        ID3D12DescriptorHeap* descriptorheap_RTV = nullptr;
        ID3D12DescriptorHeap* descriptorheap_DSV = nullptr;
        D3D12_CPU_DESCRIPTOR_HANDLE rtv_descriptor_heap_start = {};
        D3D12_CPU_DESCRIPTOR_HANDLE dsv_descriptor_heap_start = {};

    public:
        // TODO:
        struct AllocationHandler
        {
            D3D12MA::Allocator* allocator = nullptr;
            ID3D12Device* device;
            uint64_t framecount = 0;
            std::mutex destroylocker;
            std::deque<std::pair<D3D12MA::Allocation*, uint64_t>> destroyer_allocations;
            std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, uint64_t>> destroyer_resources;
            std::deque<std::pair<uint32_t, uint64_t>> destroyer_queries_timestamp;
            std::deque<std::pair<uint32_t, uint64_t>> destroyer_queries_occlusion;
            std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12PipelineState>, uint64_t>> destroyer_pipelines;
            std::deque<std::pair<ID3D12RootSignature*, uint64_t>> destroyer_rootSignatures;
            std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12StateObject>, uint64_t>> destroyer_stateobjects;
            std::deque<std::pair<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>, uint64_t>> destroyer_descriptorHeaps;

            ThreadSafeRingBuffer<uint32_t, timestamp_query_count> free_timestampqueries;
            ThreadSafeRingBuffer<uint32_t, occlusion_query_count> free_occlusionqueries;

            // Deferred destroy of resources that the GPU is already finished with:
            void Update(uint64_t FRAMECOUNT, uint32_t BACKBUFFER_COUNT);
        };

        std::shared_ptr<AllocationHandler> allocationhandler;
    };
}
