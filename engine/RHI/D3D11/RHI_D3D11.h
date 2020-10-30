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
#include "RHI/D3D/RHI_D3D.h"
#include "Core/Containers.h"

#define D3D11_NO_HELPERS
#include <d3d11_3.h>
#include <atomic>

namespace Alimer
{
    class D3D11_CommandList;

    class GraphicsDevice_DX11 : public GraphicsDevice
    {
        friend class D3D11_CommandList;

    private:
        D3D11_CommandList* commandLists[kCommandListCount] = {};

        void CreateBackBufferResources();

        std::atomic_uint32_t commandListsCount{ 0 };

        UnorderedMap<size_t, ComPtr<ID3D11BlendState1>> blendStateCache;
        UnorderedMap<size_t, ComPtr<ID3D11RasterizerState>> rasterizerStateCache;
        UnorderedMap<size_t, ComPtr<ID3D11DepthStencilState>> depthStencilStateCache;
        UnorderedMap<size_t, ComPtr<ID3D11SamplerState>> samplerCache;

        struct EmptyResourceHandle {}; // only care about control-block
        std::shared_ptr<EmptyResourceHandle> emptyresource;

    public:
        static bool IsAvailable();
        GraphicsDevice_DX11(WindowHandle window, const Desc& desc);
        ~GraphicsDevice_DX11() override;

        RefPtr<GraphicsBuffer> CreateBuffer(const GPUBufferDesc& desc, const void* initialData) override;
        bool CreateTexture(const TextureDesc* pDesc, const SubresourceData* pInitialData, Texture* pTexture) override;
        bool CreateShader(ShaderStage stage, const void* pShaderBytecode, size_t BytecodeLength, Shader* pShader) override;
        bool CreateShader(ShaderStage stage, const char* source, const char* entryPoint, Shader* pShader) override;
        ID3D11DepthStencilState* GetDepthStencilState(const DepthStencilStateDescriptor& descriptor);
        ID3D11RasterizerState* GetRasterizerState(const RasterizationStateDescriptor& descriptor, uint32_t sampleCount);
        ID3D11BlendState1* GetBlendState(const RenderPipelineDescriptor* descriptor);

        RefPtr<Sampler> CreateSampler(const SamplerDescriptor* descriptor) override;
        bool CreateQuery(const GPUQueryDesc* pDesc, GPUQuery* pQuery) override;
        bool CreateRenderPipelineCore(const RenderPipelineDescriptor* descriptor, RenderPipeline** renderPipeline) override;
        bool CreateRenderPass(const RenderPassDesc* pDesc, RenderPass* renderpass) override;

        int CreateSubresource(Texture* texture, SUBRESOURCE_TYPE type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount) override;
        int CreateSubresource(GraphicsBuffer* buffer, SUBRESOURCE_TYPE type, uint64_t offset, uint64_t size = ~0) override;

        void Map(const GPUResource* resource, Mapping* mapping) override;
        void Unmap(const GPUResource* resource) override;
        bool QueryRead(const GPUQuery* query, GPUQueryResult* result) override;

        void SetName(GPUResource* pResource, const char* name) override;

        void WaitForGPU() override;

        CommandList& BeginCommandList() override;
        void SubmitCommandLists() override;
        void PresentEnd();

        void Resize(uint32_t width, uint32_t height) override;

        Texture GetBackBuffer() override;

        ID3D11Device1* GetD3DDevice() const { return device; }

        ///////////////Thread-sensitive////////////////////////
    private:
        void CreateFactory();
        void GetAdapter(IDXGIAdapter1** ppAdapter);

        IDXGIFactory2* dxgiFactory2 = nullptr;
        ID3D11Device1* device = nullptr;
        ID3D11DeviceContext1* immediateContext = nullptr;
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

        IDXGISwapChain1* swapChain = nullptr;
        ID3D11Texture2D* backBufferTexture = nullptr;
        ID3D11RenderTargetView* renderTargetView = nullptr;
        
    };
}
