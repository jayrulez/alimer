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

#include "D3D12SwapChain.h"
#include "D3D12GraphicsDevice.h"

namespace Alimer
{
    D3D12SwapChain::D3D12SwapChain(D3D12GraphicsDevice* device, WindowHandle windowHandle, uint32_t bufferCount)
        : device{ device }
    {
        UINT swapChainFlags = 0;

        if (device->IsTearingSupported())
            swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = 0;
        swapChainDesc.Height = 0;
        swapChainDesc.Format = ToDXGIFormat(SRGBToLinearFormat(PixelFormat::BGRA8Unorm));
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = bufferCount;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
#else
        swapChainDesc.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
#endif
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapChainDesc.Flags = swapChainFlags;

        IDXGISwapChain1* tempSwapChain;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        ALIMER_ASSERT(IsWindow(windowHandle));

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = TRUE;

        // Create a swap chain for the window.
        ThrowIfFailed(device->GetDXGIFactory()->CreateSwapChainForHwnd(
            device->GetGraphicsQueue(),
            windowHandle,
            &swapChainDesc,
            &fsSwapChainDesc,
            nullptr,
            &tempSwapChain
        ));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        ThrowIfFailed(device->GetDXGIFactory()->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER));
#else
        IUnknown* window = presentationParameters.handle;
        ThrowIfFailed(device->GetDXGIFactory()->CreateSwapChainForCoreWindow(
            device->GetGraphicsQueue(),
            windowHandle,
            &swapChainDesc,
            nullptr,
            &tempSwapChain
        ));
#endif

        ThrowIfFailed(tempSwapChain->QueryInterface(&handle));
        tempSwapChain->Release();
        AfterReset();
    }

    D3D12SwapChain::~D3D12SwapChain()
    {
        Destroy();
    }

    void D3D12SwapChain::Destroy()
    {
        SafeRelease(handle);
    }

    void D3D12SwapChain::AfterReset()
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
        ThrowIfFailed(handle->GetDesc1(&swapChainDesc));

        width = swapChainDesc.Width;
        height = swapChainDesc.Height;

        //colorTextures.resize(swapChainDesc.BufferCount);
        for (uint32 index = 0; index < swapChainDesc.BufferCount; ++index)
        {
            //ID3D12Resource* backBuffer;
            //ThrowIfFailed(handle->GetBuffer(index, IID_PPV_ARGS(&backBuffer)));
            //colorTextures[index].Reset(new D3D12Texture(device, backBuffer, TextureLayout::Present));
        }

        backBufferIndex = handle->GetCurrentBackBufferIndex();
    }

    void D3D12SwapChain::Present(bool verticalSync)
    {
        //auto d3dContext = static_cast<D3D12CommandContext*>(device->GetImmediateContext());
        //d3dContext->TransitionResource(StaticCast<D3D12Texture>(colorTextures[backBufferIndex]).Get(), TextureLayout::Present);
        //device->GetImmediateContext()->Flush();


        UINT syncInterval = 1;
        UINT presentFlags = 0;

        if (!verticalSync)
        {
            syncInterval = 0;

            if (!isFullscreen && device->IsTearingSupported())
            {
                presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
            }
        }

        HRESULT hr = handle->Present(syncInterval, presentFlags);

        // Handle device lost result.
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET || hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR)
        {
            device->SetDeviceLost();
        }

        ThrowIfFailed(hr);
        backBufferIndex = handle->GetCurrentBackBufferIndex();
    }
}
