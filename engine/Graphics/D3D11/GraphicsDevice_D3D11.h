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
#include "Math/size.h"
#include <d3d11_1.h>
#include <wrl/client.h>

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

        IDXGIFactory2* GetDXGIFactory() const { return dxgiFactory.Get(); }
        bool IsTearingSupported() const { return tearingSupported; }

        ID3D11Device1* GetD3DDevice() const { return d3dDevice; }
        ID3D11DeviceContext1* GetD3DDeviceContext() const { return deviceContexts[0].Get(); }

    private:
        bool Initialize(const PresentationParameters& presentationParameters) override;
        void Shutdown() override;

        void CreateFactory();
        void GetHardwareAdapter(IDXGIAdapter1** ppAdapter);
        void CreateDeviceResources();
        void CreateWindowSizeDependentResources();
        void InitCapabilities(IDXGIAdapter1* dxgiAdapter);
        void BeginFrame() override;
        void EndFrame() override;
        void HandleDeviceLost();

        TextureHandle CreateTexture(const TextureDesc& desc, const void* pData, bool autoGenerateMipmaps) override;
        void DestroyTexture(TextureHandle handle) override;

        Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory;
        bool flipPresentSupported = true;
        bool tearingSupported = false;

        ID3D11Device1* d3dDevice = nullptr;
        D3D_FEATURE_LEVEL d3dFeatureLevel;

        uint32_t syncInterval = 1u;
        uint32_t presentFlags = 0u;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        HWND window = nullptr;
        bool isFullscreen = false;
        IDXGISwapChain1* swapChain = nullptr;
#else
        IUnknown* window = nullptr;
        Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain;
#endif
        usize windowSize;
        uint32_t backBufferCount = 2u;
        DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
        

        static constexpr uint32_t kTotalCommandContexts = kMaxCommandLists + 1;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext1> deviceContexts[kTotalCommandContexts] = {};
        Microsoft::WRL::ComPtr<ID3DUserDefinedAnnotation> userDefinedAnnotations[kTotalCommandContexts] = {};

        /* the following arrays are used for unbinding resources, they will always contain zeroes */
        ID3D11RenderTargetView* zeroRTVS[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};

        /* Types and handles */
        struct Texture {
            enum { MAX_COUNT = 4096 };

            ID3D11Resource* handle;
            DXGI_FORMAT dxgiFormat;
        };

        struct Buffer {
            enum { MAX_COUNT = 4096 };

            ID3D11Buffer* handle;
        };

        Pool<Texture, Texture::MAX_COUNT> textures;
        Pool<Buffer, Buffer::MAX_COUNT> buffers;
    };
}
