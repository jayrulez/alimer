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

#include "D3D11SwapChain.h"
#include "D3D11Texture.h"
#include "D3D11RHI.h"
#include "Platform/Window.h"

namespace Alimer
{
    D3D11SwapChain::D3D11SwapChain(D3D11RHIDevice* device_)
        : device(device_)
    {

    }

    D3D11SwapChain::~D3D11SwapChain()
    {
        Destroy();
    }

    void D3D11SwapChain::Destroy()
    {
        if (!handle)
            return;

        SafeRelease(handle);
    }

    bool D3D11SwapChain::CreateOrResize()
    {
        drawableSize = window->GetSize();

        UINT swapChainFlags = 0;

        if (device->IsTearingSupported())
            swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        bool needRecreate = handle == nullptr || window->GetNativeHandle() != windowHandle;
        if (needRecreate)
        {
            Destroy();

            // Create a descriptor for the swap chain.
            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
            swapChainDesc.Width = UINT(drawableSize.width);
            swapChainDesc.Height = UINT(drawableSize.height);
            swapChainDesc.Format = ToDXGIFormat(SRGBToLinearFormat(colorFormat));
            swapChainDesc.Stereo = FALSE;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = kBufferCount;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
#else
            swapChainDesc.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
#endif
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
            swapChainDesc.Flags = swapChainFlags;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            windowHandle = (HWND)window->GetNativeHandle();
            ALIMER_ASSERT(IsWindow(windowHandle));

            //DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
            //fsSwapChainDesc.Windowed = TRUE; // !window->IsFullscreen();

            // Create a swap chain for the window.
            ThrowIfFailed(device->GetDXGIFactory()->CreateSwapChainForHwnd(
                device->GetD3DDevice(),
                windowHandle,
                &swapChainDesc,
                nullptr, // &fsSwapChainDesc,
                nullptr,
                &handle
            ));

            // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
            ThrowIfFailed(device->GetDXGIFactory()->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER));
#else
            windowHandle = (IUnknown*)window->GetNativeHandle();
            ThrowIfFailed(device->GetDXGIFactory()->CreateSwapChainForCoreWindow(
                device->d3dDevice,
                windowHandle,
                &swapChainDesc,
                nullptr,
                &handle
            ));
#endif
        }
        else
        {

        }

        // Update present data
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

        AfterReset();

        return true;
    }

    void D3D11SwapChain::AfterReset()
    {
        colorTexture.Reset();

        ComPtr<ID3D11Texture2D> backbufferTexture;
        ThrowIfFailed(handle->GetBuffer(0, IID_PPV_ARGS(&backbufferTexture)));
        colorTexture = MakeRefPtr<D3D11Texture>(device, backbufferTexture.Get(), colorFormat);

        if (depthStencilFormat != PixelFormat::Invalid)
        {
            //GPUTextureDescription depthTextureDesc = GPUTextureDescription::New2D(depthStencilFormat, extent.width, extent.height, false, TextureUsage::RenderTarget);
            //depthStencilTexture.Reset(new D3D11Texture(device, depthTextureDesc, nullptr));
        }
    }

    Texture* D3D11SwapChain::GetCurrentTexture() const
    {
        return colorTexture.Get();
    }
}

