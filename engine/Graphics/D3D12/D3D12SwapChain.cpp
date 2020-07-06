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

namespace alimer
{
    D3D12SwapChain::D3D12SwapChain(D3D12GraphicsDevice* device, const SwapChainDescription& desc)
        : SwapChain(desc)
        , _device(device)
    {
        DXGIFactoryCaps factoryCaps = DXGIFactoryCaps::FlipPresent;
        if (device->IsTearingSupported()) {
            factoryCaps |= DXGIFactoryCaps::Tearing;
        }

        IDXGISwapChain1* tempSwapChain = DXGICreateSwapchain(
            device->GetDXGIFactory(),
            factoryCaps,
            device->GetGraphicsQueue(),
            desc.windowHandle,
            desc.width, desc.height,
            desc.colorFormat,
            2u,
            desc.isFullscreen
        );

        ThrowIfFailed(tempSwapChain->QueryInterface(IID_PPV_ARGS(&handle)));
        SafeRelease(tempSwapChain);
    }

    D3D12SwapChain::~D3D12SwapChain()
    {
        Destroy();
    }

    void D3D12SwapChain::Destroy()
    {
        SafeRelease(handle);
    }

    void D3D12SwapChain::BackendSetName()
    {
        //auto wideName = ToUtf16(name);
        //handle->SetName(wideName.c_str());
        DXGISetObjectName(handle, name.c_str());
    }
}
