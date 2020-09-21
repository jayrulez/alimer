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

#include "Core/Log.h"
#include "D3D11SwapChain.h"
#include "D3D11Texture.h"
#include "D3D11GraphicsDevice.h"
#include "Platform/Window.h"

using Microsoft::WRL::ComPtr;

namespace Alimer
{
    D3D11SwapChain::D3D11SwapChain(D3D11GraphicsDevice* device, Window* window, bool srgb, bool verticalSync)
        : device{ device }
        , colorFormat(srgb ? PixelFormat::BGRA8UnormSrgb : PixelFormat::BGRA8Unorm)
    {
        if (!verticalSync)
        {
            syncInterval = 0u;
            // Recommended to always use tearing if supported when using a sync interval of 0.
            presentFlags = device->IsTearingSupported() ? DXGI_PRESENT_ALLOW_TEARING : 0u;
        }
        else
        {
            syncInterval = 1u;
            presentFlags = 0u;
        }

        /*auto bounds = window->GetBounds();

        handle = DXGICreateSwapChain(
            device->GetDXGIFactory(), device->GetDXGIFactoryCaps(),
            device->GetD3DDevice(),
            window->GetNativeHandle(),
            static_cast<uint32_t>(bounds.width), static_cast<uint32_t>(bounds.height),
            DXGI_FORMAT_B8G8R8A8_UNORM,
            backBufferCount,
            window->IsFullscreen());
        AfterReset();*/
    }

    D3D11SwapChain::~D3D11SwapChain()
    {
    }

    void D3D11SwapChain::AfterReset()
    {
        colorTexture.Reset();

        ComPtr<ID3D11Texture2D> backbufferTexture;
        ThrowIfFailed(handle->GetBuffer(0, IID_PPV_ARGS(&backbufferTexture)));
        colorTexture = MakeRefPtr<D3D11Texture>(device, backbufferTexture.Get(), colorFormat);

        /*colorTextures.clear();
        depthStencilTexture.Reset();

        // Create a render target view of the swap chain back buffer.
        ID3D11Texture2D* backbufferTexture;
        ThrowIfFailed(swapChain->GetBuffer(0, IID_PPV_ARGS(&backbufferTexture)));
        colorTextures.push_back(MakeRefPtr<D3D11Texture>(device, backbufferTexture, colorFormat));
        backbufferTexture->Release();

        if (depthStencilFormat != PixelFormat::Invalid)
        {
            //GPUTextureDescription depthTextureDesc = GPUTextureDescription::New2D(depthStencilFormat, extent.width, extent.height, false, TextureUsage::RenderTarget);
            //depthStencilTexture.Reset(new D3D11Texture(device, depthTextureDesc, nullptr));
        }*/
    }

    HRESULT D3D11SwapChain::Present()
    {
        return handle->Present(syncInterval, presentFlags);
    }
}
