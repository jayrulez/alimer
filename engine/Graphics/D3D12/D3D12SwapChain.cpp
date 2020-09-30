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

#include "Graphics/SwapChain.h"
#include "D3D12CommandContext.h"
#include "D3D12Texture.h"
#include "D3D12GraphicsDevice.h"

namespace Alimer
{
    SwapChain::SwapChain(GraphicsDevice* device, void* windowHandle, const PresentationParameters& presentationParameters)
        : device{ device }
        , presentationParameters{ presentationParameters }
    {
        UINT swapChainFlags = 0;

        if (device->GetImpl()->IsTearingSupported())
            swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = presentationParameters.backBufferWidth;
        swapChainDesc.Height = presentationParameters.backBufferHeight;
        swapChainDesc.Format = ToDXGIFormat(SRGBToLinearFormat(presentationParameters.backBufferFormat));
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

        IDXGISwapChain1* tempSwapChain;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        HWND window = (HWND)windowHandle;
        ALIMER_ASSERT(IsWindow(window));

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = !presentationParameters.isFullscreen;

        // Create a swap chain for the window.
        ThrowIfFailed(device->GetImpl()->GetDXGIFactory()->CreateSwapChainForHwnd(
            device->GetCommandQueue()->GetHandle(),
            window,
            &swapChainDesc,
            &fsSwapChainDesc,
            nullptr,
            &tempSwapChain
        ));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        ThrowIfFailed(device->GetImpl()->GetDXGIFactory()->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));
#else
        IUnknown* window = (IUnknown*)desc.windowHandle;
        ThrowIfFailed(device->GetDXGIFactory()->CreateSwapChainForCoreWindow(
            device->GetD3D12GraphicsQueue(),
            window,
            &swapChainDesc,
            nullptr,
            swapChain1.GetAddressOf()
        ));
#endif

        ThrowIfFailed(tempSwapChain->QueryInterface(&handle));
        tempSwapChain->Release();
    }

    void SwapChain::Present()
    {
        UINT syncInterval = 1;
        UINT presentFlags = 0;

        if (!presentationParameters.verticalSync)
        {
            syncInterval = 0;

            if (!presentationParameters.isFullscreen && device->GetImpl()->IsTearingSupported())
            {
                presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
            }
        }

        HRESULT hr = handle->Present(syncInterval, presentFlags);

        // Handle device lost result.
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET || hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR)
        {
            //device->SetDeviceLost();
        }

        ThrowIfFailed(hr);

        backBufferIndex = handle->GetCurrentBackBufferIndex();
    }

    SwapChainHandle SwapChain::GetHandle() const
    {
        return handle;
    }

#if TODO

    D3D12SwapChain::D3D12SwapChain(D3D12GraphicsDevice* device, void* windowHandle, const SwapChainDesc& desc, uint32_t bufferCount)
        : SwapChain(desc)
        , device{ device }
        , isTearingSupported(device->IsTearingSupported())
    {


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

        colorTextures.resize(swapChainDesc.BufferCount);
        for (uint32 index = 0; index < swapChainDesc.BufferCount; ++index)
        {
            ID3D12Resource* backBuffer;
            ThrowIfFailed(handle->GetBuffer(index, IID_PPV_ARGS(&backBuffer)));
            colorTextures[index].Reset(new D3D12Texture(device, backBuffer, TextureLayout::Present));
        }

        backBufferIndex = handle->GetCurrentBackBufferIndex();
    }

    void D3D12SwapChain::Present(bool verticalSync)
    {
        auto d3dContext = static_cast<D3D12CommandContext*>(device->GetImmediateContext());
        d3dContext->TransitionResource(StaticCast<D3D12Texture>(colorTextures[backBufferIndex]).Get(), TextureLayout::Present);

        device->GetImmediateContext()->Flush();

        

        if (isPrimary)
        {
            device->FinishFrame();
        }
    }

    GraphicsDevice& D3D12SwapChain::GetDevice()
    {
        return *device;
    }
#endif // TODO
}

