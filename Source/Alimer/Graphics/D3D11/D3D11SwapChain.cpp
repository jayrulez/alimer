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
#include "Core/Window.h"

namespace alimer
{
    D3D11SwapChain::D3D11SwapChain(D3D11GraphicsDevice* device_, Window* window_, PixelFormat colorFormat_, PixelFormat depthStencilFormat_, bool vSync)
        : device(device_)
        , window(window_)
        , colorFormat(colorFormat_)
        , depthStencilFormat(depthStencilFormat_)
        , syncInterval(1)
        , presentFlags(0)
    {

        

        AfterReset();
    }

    D3D11SwapChain::~D3D11SwapChain()
    {
        Destroy();
    }

    void D3D11SwapChain::Destroy()
    {
        SafeRelease(handle);
    }

    bool D3D11SwapChain::Present()
    {
        HRESULT hr = handle->Present(syncInterval, presentFlags);
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            return false;
        }

        ThrowIfFailed(hr);
        return true;
    }

    /*void D3D11SwapChain::Recreate()
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
        memset(&swapChainDesc, 0, sizeof(swapChainDesc));
        _handle->GetDesc1(&swapChainDesc);

        HRESULT hr = _handle->ResizeBuffers(swapChainDesc.BufferCount,
            width, height,
            swapChainDesc.Format,
            swapChainDesc.Flags
        );

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            _device->HandleDeviceLost(hr);

            // Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method
            // and correctly set up the new device.
            return;
        }
    }*/

    void D3D11SwapChain::AfterReset()
    {
        /*colorTexture.Reset();
        depthStencilTexture.Reset();

        ID3D11Texture2D* resource;
        ThrowIfFailed(handle->GetBuffer(0, IID_PPV_ARGS(&resource)));
        colorTexture = new D3D11Texture(device, resource, colorFormat);

        if (depthStencilFormat != PixelFormat::Invalid)
        {

        }*/
    }
}
