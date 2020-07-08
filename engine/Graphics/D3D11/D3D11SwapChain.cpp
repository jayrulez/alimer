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

#include "D3D11GraphicsDevice.h"
#include "D3D11SwapChain.h"
#include "D3D11Texture.h"

namespace alimer
{
    D3D11SwapChain::D3D11SwapChain(D3D11GraphicsDevice* device, const SwapChainDescription& desc)
        : SwapChain(desc)
        , _device(device)
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        _handle = DXGICreateSwapchain(
            _device->GetDXGIFactory(),
            _device->GetDXGIFactoryCaps(),
            _device->GetD3DDevice(),
            _desc.windowHandle,
            _desc.width, _desc.height,
            _desc.colorFormat,
            kNumBackBuffers,
            _desc.isFullscreen
        );
#else
        IDXGISwapChain1* tempSwapChain = DXGICreateSwapchain(
            _device->GetDXGIFactory(),
            _device->GetDXGIFactoryCaps(),
            _device->GetD3DDevice(),
            _desc.windowHandle,
            _desc.width, _desc.height,
            _desc.colorFormat,
            kNumBackBuffers,
            _desc.isFullscreen
        );

        ThrowIfFailed(tempSwapChain->QueryInterface(IID_PPV_ARGS(&_handle)));
        SafeRelease(tempSwapChain);
#endif

        AfterReset();
    }

    D3D11SwapChain::~D3D11SwapChain()
    {
        Destroy();
    }

    void D3D11SwapChain::Destroy()
    {
        SafeRelease(_handle);
    }

    void D3D11SwapChain::Present()
    {
        UINT syncInterval = 1u;
        UINT presentFlags = 0;
        if (_desc.presentationInterval == PresentInterval::immediate)
        {
            syncInterval = 0;
            if (((_device->GetDXGIFactoryCaps() & DXGIFactoryCaps::Tearing) != DXGIFactoryCaps::None)) {
                presentFlags = DXGI_PRESENT_ALLOW_TEARING;
            }
        }

        HRESULT hr = _handle->Present(syncInterval, presentFlags);
    }

    void D3D11SwapChain::Recreate()
    {
        UINT swapChainFlags = 0;
        if (_desc.presentationInterval == PresentInterval::immediate
            && ((_device->GetDXGIFactoryCaps() & DXGIFactoryCaps::Tearing) != DXGIFactoryCaps::None)) {
            swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }

        HRESULT hr = _handle->ResizeBuffers(
            kNumBackBuffers,
            _desc.width,
            _desc.height,
            ToDXGISwapChainFormat(_desc.colorFormat),
            swapChainFlags
        );

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            _device->HandleDeviceLost(hr);

            // Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method
            // and correctly set up the new device.
            return;
        }
    }

    void D3D11SwapChain::BackendSetName()
    {
        DXGISetObjectName(_handle, name);
    }

    void D3D11SwapChain::AfterReset()
    {
        ID3D11Texture2D* resource;
        ThrowIfFailed(_handle->GetBuffer(0, IID_PPV_ARGS(&resource)));
        _backbufferTextures.Push(new D3D11Texture(_device, resource));
    }
}
