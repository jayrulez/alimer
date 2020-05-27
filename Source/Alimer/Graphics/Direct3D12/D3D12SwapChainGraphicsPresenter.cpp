//
// Copyright (c) 2020 Amer Koleci and contributors.
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

#include "D3D12SwapChainGraphicsPresenter.h"
#include "D3D12GraphicsDevice.h"

namespace alimer
{
    D3D12SwapChainGraphicsPresenter::D3D12SwapChainGraphicsPresenter(D3D12GraphicsDevice* device, HWND windowHandle, const PresentationParameters& presentationParameters)
        : GraphicsPresenter(*device, presentationParameters)
    {
        // Create a SwapChain from a Win32 window.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = backBufferWidth;
        swapChainDesc.Height = backBufferHeight;
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2u;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        if (device->IsTearingSupported())
        {
            swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = !presentationParameters.isFullscreen;

        IDXGISwapChain1* tempSwapChain;
        VHR(device->GetDXGIFactory()->CreateSwapChainForHwnd(
            device->GetD3DCommandQueue(),
            windowHandle,
            &swapChainDesc,
            &fsSwapChainDesc,
            NULL,
            &tempSwapChain
        ));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        VHR(device->GetDXGIFactory()->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER));

        CreateRenderTargets();
    }

    D3D12SwapChainGraphicsPresenter::~D3D12SwapChainGraphicsPresenter()
    {
        Destroy();
    }

    void D3D12SwapChainGraphicsPresenter::Destroy()
    {
        if (handle == nullptr) {
            return;
        }

        handle->Release();
        handle = nullptr;
    }

    void D3D12SwapChainGraphicsPresenter::CreateRenderTargets()
    {

    }
}

