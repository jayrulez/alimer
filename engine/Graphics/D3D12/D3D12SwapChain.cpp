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
#include "D3D12Texture.h"
#include "D3D12GraphicsDevice.h"

namespace Alimer
{
    D3D12SwapChain::D3D12SwapChain(D3D12GraphicsDevice* device_, const SwapChainDescription& desc)
        : SwapChain(desc)
        , device(device_)
        , isTearingSupported(device->IsTearingSupported())
        , syncInterval(desc.vsync ? 1 : 0)
    {
        ComPtr<IDXGISwapChain1> swapChain1 = DXGICreateSwapChain(
            device->GetDXGIFactory(),
            device->GetDXGIFactoryCaps(),
            device->GetGraphicsQueue(),
            kBackBufferCount,
            desc);
        ThrowIfFailed(swapChain1.As(&handle));

        AfterReset();
    }

    D3D12SwapChain::~D3D12SwapChain()
    {
        //CloseHandle(swapChainEvent);
    }

    void D3D12SwapChain::AfterReset()
    {
        colorTextures.clear();
        currentBackBufferIndex = handle->GetCurrentBackBufferIndex();

        for (uint32_t i = 0; i < kBackBufferCount; ++i)
        {
            ComPtr<ID3D12Resource> backBuffer;
            ThrowIfFailed(handle->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

            colorTextures.push_back(MakeRefPtr<D3D12Texture>(device, backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT));
        }
    }

    HRESULT D3D12SwapChain::Present()
    {
        uint32_t presentFlags = 0;

        // Recommended to always use tearing if supported when using a sync interval of 0.
        // Note this will fail if in true 'fullscreen' mode.
        if (!syncInterval && !isFullscreen && isTearingSupported)
        {
            presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
        }

        HRESULT hr = handle->Present(syncInterval, presentFlags);
        if (SUCCEEDED(hr))
        {
            currentBackBufferIndex = handle->GetCurrentBackBufferIndex();
        }

        return hr;
    }
}
