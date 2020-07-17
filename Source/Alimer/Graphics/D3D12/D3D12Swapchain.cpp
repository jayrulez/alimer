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

#include "D3D12Swapchain.h"
#include "D3D12Texture.h"
#include "D3D12GraphicsDevice.h"

namespace alimer
{
    D3D12Swapchain::D3D12Swapchain(D3D12GraphicsDevice* device, const SwapchainDescription& description)
        : Swapchain(description)
        , device{ device }
        , backbufferCount(kInflightFrameCount)
    {
        colorFormat = SRGBToLinearFormat(description.preferredColorFormat);
        backbufferFormat = ToDXGIFormat(colorFormat);

        switch (description.presentMode)
        {
        case PresentMode::Immediate:
            syncInterval = 0;
            if (any(device->GetDXGIFactoryCaps() & DXGIFactoryCaps::Tearing)) {
                presentFlags = DXGI_PRESENT_ALLOW_TEARING;
            }
            break;

        case PresentMode::Mailbox:
            syncInterval = 2;
            presentFlags = 0;
            break;

        default:
            syncInterval = 1;
            presentFlags = 0;
            break;
        }

        IDXGISwapChain1* tempSwapChain = DXGICreateSwapchain(
            device->GetDXGIFactory(), device->GetDXGIFactoryCaps(),
            device->GetGraphicsQueue(),
            description.windowHandle,
            description.width, description.height,
            backbufferFormat,
            backbufferCount,
            description.isFullscreen
        );

        ThrowIfFailed(tempSwapChain->QueryInterface(IID_PPV_ARGS(&handle)));
        SafeRelease(tempSwapChain);
    }

    D3D12Swapchain::~D3D12Swapchain()
    {
        Destroy();
    }

    void D3D12Swapchain::Destroy()
    {
        SafeRelease(handle);
    }

    void D3D12Swapchain::Present()
    {
        HRESULT hr = handle->Present(syncInterval, presentFlags);
    }

    void D3D12Swapchain::ResizeImpl()
    {
        device->WaitForGPU();

    }

    void D3D12Swapchain::AfterReset()
    {
        for (uint32_t index = 0; index < backbufferCount; ++index)
        {
            {
                ID3D12Resource* resource;
                ThrowIfFailed(handle->GetBuffer(index, IID_PPV_ARGS(&resource)));
                //backbufferTextures[index] = new D3D12Texture(this, resource, D3D12_RESOURCE_STATE_PRESENT);
            }
        }

        backbufferIndex = handle->GetCurrentBackBufferIndex();
    }

    void D3D12Swapchain::BackendSetName()
    {
        //auto wideName = ToUtf16(name);
        //handle->SetName(wideName.c_str());
        DXGISetObjectName(handle, name.c_str());
    }
}
