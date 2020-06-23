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

#include "graphics/GraphicsDevice.h"
#include "graphics/Types.h"
#include "graphics/D3D/D3DHelpers.h"
#include "Math/Size.h"
#ifndef D3D11_NO_HELPERS
#   define D3D11_NO_HELPERS
#endif
#include <d3d11_1.h>

namespace alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    extern PFN_D3D11_CREATE_DEVICE D3D11CreateDevice;
#endif

    class GraphicsDevice_D3D11 final : public GraphicsDevice
    {
    public:
        static bool IsAvailable();

        GraphicsDevice_D3D11(const Desc& desc);
        ~GraphicsDevice_D3D11();

        IDXGIFactory2* GetDXGIFactory() const { return dxgiFactory; }
        bool IsTearingSupported() const { return tearingSupported; }

        ID3D11Device1* GetD3DDevice() const { return d3dDevice; }
        ID3D11DeviceContext1* GetD3DDeviceContext() const { return deviceContexts[0]; }

    private:
        void Shutdown() override;

        void CreateFactory();
        void CreateDeviceResources();
        void CreateWindowSizeDependentResources();
        void AfterReset(SwapChainHandle handle);
        void InitCapabilities(IDXGIAdapter1* dxgiAdapter);
        void BeginFrame() override;
        void EndFrame() override;
        void HandleDeviceLost();

        SwapChainHandle CreateSwapChain(const SwapChainDesc& desc) override;
        void DestroySwapChain(SwapChainHandle handle) override;
        uint32_t GetBackbufferCount(SwapChainHandle handle) override;
        TextureHandle GetBackbufferTexture(SwapChainHandle handle, uint32_t index) override;
        uint32_t Present(SwapChainHandle handle) override;

        TextureHandle AllocTextureHandle();
        TextureHandle CreateTexture(const TextureDesc& desc, const void* pData, bool autoGenerateMipmaps) override;
        TextureHandle CreateTexture(const TextureDesc& desc, uint64_t nativeHandle) override;
        void DestroyTexture(TextureHandle handle) override;

        void ClearState(CommandList commandList);
        void InsertDebugMarker(const char* name, CommandList commandList) override;
        void PushDebugGroup(const char* name, CommandList commandList) override;
        void PopDebugGroup(CommandList commandList) override;
        void BeginRenderPass(const RenderPassDesc& desc, CommandList commandList) override;
        void EndRenderPass(CommandList commandList) override;
        void SetBlendColor(const Color& color, CommandList commandList) override;

        IDXGIFactory2* dxgiFactory = nullptr;
        bool flipPresentSupported = true;
        bool tearingSupported = false;

        ID3D11Device1* d3dDevice = nullptr;
        D3D_FEATURE_LEVEL d3dFeatureLevel;
        bool isLost = false;

        static constexpr uint32_t kTotalCommandContexts = kMaxCommandLists + 1;
        ID3D11DeviceContext1* deviceContexts[kTotalCommandContexts] = {};
        ID3DUserDefinedAnnotation* userDefinedAnnotations[kTotalCommandContexts] = {};
        Color blendColors[kTotalCommandContexts] = {};

        /* Types and handles */
        struct SwapChainD3D11 {
            enum { MAX_COUNT = 13 };

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            IDXGISwapChain1* handle;
#else
            IDXGISwapChain3* handle;
#endif
            uint32_t backbufferCount;
            uint32_t syncInterval;
            uint32_t presentFlags;
            PixelFormat colorFormat;
            PixelFormat depthStencilFormat;
            TextureHandle backbufferTexture; 

            // HDR Support
            DXGI_COLOR_SPACE_TYPE colorSpace;
        };

        struct TextureD3D11 {
            enum { MAX_COUNT = 4096 };

            ID3D11Resource* handle;
            DXGI_FORMAT dxgiFormat;
            uint32_t mipLevels;
            Vector<ID3D11RenderTargetView*> RTVs;
        };

        struct BufferD3D11 {
            enum { MAX_COUNT = 4096 };

            ID3D11Buffer* handle;
        };

        Pool<SwapChainD3D11, SwapChainD3D11::MAX_COUNT> swapChains;
        Pool<TextureD3D11, TextureD3D11::MAX_COUNT> textures;
        Pool<BufferD3D11, BufferD3D11::MAX_COUNT> buffers;
    };
}
