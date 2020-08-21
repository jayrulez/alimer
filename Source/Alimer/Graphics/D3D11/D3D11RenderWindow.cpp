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
#include "D3D11RenderWindow.h"
#include "D3D11Texture.h"
#include "D3D11GPUDevice.h"

namespace alimer
{
    D3D11RenderWindow::D3D11RenderWindow(D3D11GPUDevice* device_, const RenderWindowDescription& desc)
        : RenderWindow(desc)
        , device(device_)
    {
        SetVerticalSync(desc.verticalSync);

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
        swapchainDesc.Width = GetSize().width;
        swapchainDesc.Height = GetSize().height;
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
        HWND window = (HWND)GetNativeHandle();
        ALIMER_ASSERT(IsWindow(window));

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapchainDesc = {};
        fsSwapchainDesc.Windowed = !fullscreen;

        // Create a SwapChain from a Win32 window.
        ThrowIfFailed(device->GetDXGIFactory()->CreateSwapChainForHwnd(
            device->GetD3DDevice(),
            window,
            &swapchainDesc,
            &fsSwapchainDesc,
            nullptr,
            &swapChain
        ));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        ThrowIfFailed(device->GetDXGIFactory()->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));
#else
        IDXGISwapChain1* tempSwapChain;
        ThrowIfFailed(device->GetDXGIFactory()->CreateSwapChainForCoreWindow(
            device->GetD3DDevice(),
            window,
            &swapchainDesc,
            nullptr,
            &tempSwapChain
        ));
        ThrowIfFailed(tempSwapChain->QueryInterface(IID_PPV_ARGS(&swapChain)));
        SafeRelease(tempSwapChain);
#endif
    }

    D3D11RenderWindow::~D3D11RenderWindow()
    {
        Destroy();
    }

    void D3D11RenderWindow::Destroy()
    {
        SafeRelease(swapChain);
        RenderWindow::Destroy();
    }

    void D3D11RenderWindow::SetVerticalSync(bool value)
    {
        if (!value)
        {
            syncInterval = 0u;
            presentFlags = device->IsTearingSupported() ? DXGI_PRESENT_ALLOW_TEARING : 0u;
        }
        else
        {
            syncInterval = 1u;
            presentFlags = 0u;
        }
    }

    void D3D11RenderWindow::Present()
    {
        HRESULT hr = swapChain->Present(syncInterval, presentFlags);
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            device->HandleDeviceLost(hr);
        }
    }
}

