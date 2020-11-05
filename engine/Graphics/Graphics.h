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
#include "Graphics/Texture.h"
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

    class ALIMER_API Sampler : public RefCounted
    {
    public:
    protected:
        Sampler() = default;
    };

    class ALIMER_API RenderPipeline : public RefCounted
    {
    protected:
        RenderPipeline() = default;
    };

    struct GraphicsSettings final
    {
        std::string applicationName = "Alimer";
        GraphicsDeviceFlags flags = GraphicsDeviceFlags::None;
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
        static RefPtr<Graphics> Create(WindowHandle windowHandle, const GraphicsSettings& desc, GraphicsBackendType backendType = GraphicsBackendType::Count);

        virtual RefPtr<GraphicsBuffer> CreateBuffer(const GPUBufferDesc& desc, const void* initialData = nullptr) = 0;
        virtual RefPtr<Texture> CreateTexture(const TextureDescription* description, const SubresourceData* initialData);
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
        virtual void WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest) {}
        virtual void WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest) {}
        virtual void WriteDescriptor(const DescriptorTable* table, uint32_t rangeIndex, uint32_t arrayIndex, const GPUResource* resource, int subresource = -1, uint64_t offset = 0) {}
        virtual void WriteDescriptor(const DescriptorTable* table, uint32_t rangeIndex, uint32_t arrayIndex, const Sampler* sampler) {}

        virtual void Map(const GPUResource* resource, Mapping* mapping) = 0;
        virtual void Unmap(const GPUResource* resource) = 0;
        virtual bool QueryRead(const GPUQuery* query, GPUQueryResult* result) = 0;

        virtual void SetName(GPUResource* pResource, const char* name) = 0;

        virtual CommandBuffer& BeginCommandBuffer() = 0;
        virtual void SubmitCommandLists() = 0;

        virtual void WaitForGPU() = 0;

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

        inline uint32_t GetFrameIndex() const
        {
            return GetFrameCount() % kMaxInflightFrames;
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

        virtual RefPtr<Texture> GetBackBuffer() = 0;

        bool CheckCapability(GRAPHICSDEVICE_CAPABILITY capability) const;

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
        virtual bool CreateTextureCore(const TextureDescription* description, const SubresourceData* initialData, Texture** texture) = 0;
        virtual bool CreateRenderPipelineCore(const RenderPipelineDescriptor* descriptor, RenderPipeline** pipeline) = 0;

        uint32_t backbufferWidth;
        uint32_t backbufferHeight;
        bool verticalSync = true;
    };

    bool StencilTestEnabled(const DepthStencilStateDescriptor* descriptor);
}
