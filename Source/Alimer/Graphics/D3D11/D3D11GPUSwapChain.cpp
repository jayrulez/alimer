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
#include "D3D11GPUSwapChain.h"
#include "D3D11GPUTexture.h"
#include "D3D11GraphicsDevice.h"

namespace alimer
{
    D3D11GPUSwapChain::D3D11GPUSwapChain(D3D11GraphicsDevice* device, const GPUSwapChainDescriptor& descriptor)
        : GPUSwapChain(device, descriptor)
        , rotation(DXGI_MODE_ROTATION_IDENTITY)
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        const DXGI_SCALING dxgiScaling = DXGI_SCALING_STRETCH;

        DXGI_SWAP_EFFECT swapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        if (!any(device->GetDXGIFactoryCaps() & DXGIFactoryCaps::FlipPresent))
        {
            swapEffect = DXGI_SWAP_EFFECT_DISCARD;
        }
#else
        const DXGI_SCALING dxgiScaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
        const DXGI_SWAP_EFFECT swapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
#endif

        DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
        swapchainDesc.Width = width;
        swapchainDesc.Height = height;
        swapchainDesc.Format = ToDXGIFormat(SRGBToLinearFormat(colorFormat));
        swapchainDesc.Stereo = false;
        swapchainDesc.SampleDesc.Count = 1;
        swapchainDesc.SampleDesc.Quality = 0;
        swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchainDesc.BufferCount = kInflightFrameCount;
        swapchainDesc.Scaling = dxgiScaling;
        swapchainDesc.SwapEffect = swapEffect;
        swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        if (device->IsTearingSupported())
        {
            swapchainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapchainDesc = {};
        fsSwapchainDesc.Windowed = !isFullscreen;

        // Create a SwapChain from a Win32 window.
        ThrowIfFailed(device->GetDXGIFactory()->CreateSwapChainForHwnd(
            device->GetD3DDevice(),
            descriptor.handle.hwnd,
            &swapchainDesc,
            &fsSwapchainDesc,
            nullptr,
            &handle
        ));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        ThrowIfFailed(device->GetDXGIFactory()->MakeWindowAssociation(descriptor.handle.hwnd, DXGI_MWA_NO_ALT_ENTER));
#else
        IDXGISwapChain1* tempSwapChain;
        ThrowIfFailed(device->GetDXGIFactory()->CreateSwapChainForCoreWindow(
            device->GetD3DDevice(),
            window,
            &swapchainDesc,
            nullptr,
            &tempSwapChain
        ));
        ThrowIfFailed(tempSwapChain->QueryInterface(IID_PPV_ARGS(&handle)));
        SafeRelease(tempSwapChain);
#endif

        AfterReset();
    }

    D3D11GPUSwapChain::~D3D11GPUSwapChain()
    {
        Destroy();
    }

    void D3D11GPUSwapChain::Destroy()
    {
        colorTexture.Reset();
        SafeRelease(handle);
    }

    void D3D11GPUSwapChain::AfterReset()
    {
        // Create a render target view of the swap chain back buffer.
        ID3D11Texture2D* backbufferTexture;
        ThrowIfFailed(handle->GetBuffer(0, IID_PPV_ARGS(&backbufferTexture)));
        colorTexture = new D3D11GPUTexture(static_cast<D3D11GraphicsDevice*>(device.Get()), backbufferTexture, colorFormat);
        backbufferTexture->Release();

        //backbufferTexture = Texture::CreateExternalTexture(backbufferTextureHandle, backbufferSize.x, backbufferSize.y, PixelFormat::BGRA8Unorm, false);

        /*if (depthStencilFormat != PixelFormat::Invalid)
        {
            // Create a depth stencil view for use with 3D rendering if needed.
            D3D11_TEXTURE2D_DESC depthStencilDesc = {};
            depthStencilDesc.Width = backbufferSize.x;
            depthStencilDesc.Height = backbufferSize.y;
            depthStencilDesc.MipLevels = 1;
            depthStencilDesc.ArraySize = 1;
            depthStencilDesc.Format = ToDXGIFormat(depthStencilFormat);
            depthStencilDesc.SampleDesc.Count = 1;
            depthStencilDesc.SampleDesc.Quality = 0;
            depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
            depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

            RefPtr<ID3D11Texture2D> depthStencil;
            ThrowIfFailed(d3dDevice->CreateTexture2D(
                &depthStencilDesc,
                nullptr,
                depthStencil.GetAddressOf()
            ));
        }*/
    }

    HRESULT D3D11GPUSwapChain::Present(uint32 syncInterval, uint32 presentFlags)
    {
        return handle->Present(syncInterval, presentFlags);
    }

    Texture* D3D11GPUSwapChain::GetColorTexture() const
    {
        return colorTexture.Get();
    }
}

