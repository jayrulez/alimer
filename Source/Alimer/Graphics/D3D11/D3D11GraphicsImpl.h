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

#include "Graphics/GraphicsImpl.h"
#include "D3D11Backend.h"
#include <mutex>

namespace alimer
{
    class D3D11CommandContext;

    struct D3D11Texture
    {
        enum { MAX_COUNT = 4096 };

        union {
            ID3D11Resource* handle;
            ID3D11Texture2D* tex2D;
            ID3D11Texture3D* tex3D;
        };

        ID3D11RenderTargetView* rtv;
    };

    struct D3D11Buffer
    {
        enum { MAX_COUNT = 4096 };

        ID3D11Buffer* handle;
    };

    class D3D11GraphicsImpl final : public GraphicsImpl
    {
    public:
        static bool IsAvailable();
        D3D11GraphicsImpl();
        ~D3D11GraphicsImpl() override;

        void Shutdown();

        bool Initialize(WindowHandle windowHandle, uint32_t width, uint32_t height, bool isFullscreen) override;
        bool BeginFrame() override;
        void EndFrame(uint64_t frameIndex) override;
        void HandleDeviceLost();

        IDXGIFactory2* GetDXGIFactory() const { return dxgiFactory.Get(); }
        DXGIFactoryCaps GetDXGIFactoryCaps() const { return dxgiFactoryCaps; }
        ID3D11Device1* GetD3DDevice() const { return d3dDevice; }

        /* Resource creation methods */
        TextureHandle AllocTextureHandle();
        TextureHandle CreateTexture(TextureDimension dimension, uint32_t width, uint32_t height, const void* data, void* externalHandle) override;
        TextureHandle CreateTexture2D(uint32_t width, uint32_t height, const void* data);
        void Destroy(TextureHandle handle) override;
        void SetName(TextureHandle handle, const char* name) override;

        BufferHandle AllocBufferHandle();
        BufferHandle CreateBuffer(BufferUsage usage, uint32_t size, uint32_t stride, const void* data) override;
        void Destroy(BufferHandle handle) override;
        void SetName(BufferHandle handle, const char* name) override;

        /* Commands */
        void PushDebugGroup(const String& name, CommandList commandList) override;
        void PopDebugGroup(CommandList commandList) override;
        void InsertDebugMarker(const String& name, CommandList commandList) override;

        void BeginRenderPass(CommandList commandList, uint32_t numColorAttachments, const RenderPassColorAttachment* colorAttachments, const RenderPassDepthStencilAttachment* depthStencil) override;
        void EndRenderPass(CommandList commandList) override;

    private:
        void CreateFactory();
        void InitCapabilities(IDXGIAdapter1* dxgiAdapter);
        void UpdateSwapChain();

        static constexpr uint64_t kRenderLatency = 2;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        HMODULE dxgiLib;
#endif

        ComPtr<IDXGIFactory2> dxgiFactory;
        bool isTearingSupported = false;
        DXGIFactoryCaps dxgiFactoryCaps = DXGIFactoryCaps::None;

        ID3D11Device1*              d3dDevice = nullptr;
        ID3D11DeviceContext1*       d3dContexts[kMaxCommandLists + 1] = {};
        ID3DUserDefinedAnnotation*  d3dAnnotations[kMaxCommandLists + 1] = {};

        D3D_FEATURE_LEVEL d3dFeatureLevel = D3D_FEATURE_LEVEL_9_1;
        bool isLost = false;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        bool isFullscreen = false;
        HWND window;
        IDXGISwapChain1* swapChain = nullptr;
#else
        IUnknown* window;
        IDXGISwapChain3* swapChain = nullptr;
#endif
        DXGI_MODE_ROTATION rotation{ DXGI_MODE_ROTATION_IDENTITY };
        RefPtr<Texture> backbufferTexture;

        /* Resource pools */
        std::mutex handle_mutex;
        GPUResourcePool<D3D11Texture, D3D11Texture::MAX_COUNT> textures;
        GPUResourcePool<D3D11Buffer, D3D11Buffer::MAX_COUNT> buffers;

        ID3D11RenderTargetView* zeroRTVS[kMaxColorAttachments] = {};
    };
}
