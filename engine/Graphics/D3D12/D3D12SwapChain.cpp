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

#if TODO
#include "D3D12GraphicsDevice.h"
#include "D3D12CommandQueue.h"
#include "D3D12SwapChain.h"
#include "D3D12Texture.h"

namespace alimer
{
    D3D12SwapChain::D3D12SwapChain(D3D12GraphicsDevice* device, CommandQueue* commandQueue, const SwapChainDescriptor* descriptor)
        : SwapChain(*device, commandQueue, descriptor)
        , device(device)
    {
        // Flip mode doesn't support SRGB formats
        dxgiColorFormat = ToDXGIFormat(srgbToLinearFormat(descriptor->colorFormat));

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.Format = dxgiColorFormat;
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = kNumBackBuffers;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        if (device->GetProvider()->IsTearingSupported())
        {
            swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = !descriptor->isFullscreen;

        auto d3d12CommandQueue = static_cast<D3D12CommandQueue*>(commandQueue);

        IDXGISwapChain1* tempSwapChain;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        HWND hwnd = (HWND)descriptor->handle;
        if (!IsWindow(hwnd)) {
            LOG_ERROR("Invalid HWND handle");
            return;
        }

        VHR(device->GetProvider()->GetDXGIFactory()->CreateSwapChainForHwnd(
            d3d12CommandQueue->GetHandle(),
            hwnd,
            &swapChainDesc,
            &fsSwapChainDesc,
            NULL,
            &tempSwapChain
        ));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        VHR(device->GetProvider()->GetDXGIFactory()->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
#else
        VHR(device->GetProvider()->GetDXGIFactory()->CreateSwapChainForCoreWindow(
            d3d12CommandQueue->GetHandle(),
            (IUnknown*)descriptor->handle,
            &swapChainDesc,
            nullptr,
            &tempSwapChain
        ));
#endif

        VHR(tempSwapChain->QueryInterface(IID_PPV_ARGS(&swapChain)));
        tempSwapChain->Release();

        CreateRenderTargets();
    }

    D3D12SwapChain::~D3D12SwapChain()
    {
        Destroy();
    }

    void D3D12SwapChain::Destroy()
    {
        if (swapChain == nullptr) {
            return;
        }

        commandQueue->WaitIdle();

        for (uint32_t i = 0; i < kNumBackBuffers; ++i)
        {
            SafeDelete(colorTextures[i]);
        }

        SAFE_RELEASE(swapChain);
    }

    void D3D12SwapChain::Present()
    {
        HRESULT hr = swapChain->Present(syncInterval, presentFlags);

        // If the device was reset we must completely reinitialize the renderer.
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            device->HandleDeviceLost();
        }

        VHR(hr);

        auto d3d12CommandQueue = static_cast<D3D12CommandQueue*>(commandQueue);
        d3d12CommandQueue->Signal();
        backbufferIndex = swapChain->GetCurrentBackBufferIndex();
    }

    Texture* D3D12SwapChain::GetCurrentColorTexture() const
    {
        return colorTextures[backbufferIndex];
    }

    void D3D12SwapChain::CreateRenderTargets()
    {
        for (uint32_t index = 0; index < kNumBackBuffers; ++index)
        {
            ID3D12Resource* backbuffer;
            VHR(swapChain->GetBuffer(index, IID_PPV_ARGS(&backbuffer)));
            colorTextures[index] = D3D12Texture::CreateFromExternal(device, backbuffer, colorFormat, D3D12_RESOURCE_STATE_PRESENT);
        }

        backbufferIndex = swapChain->GetCurrentBackBufferIndex();
    }
}

#endif // TODO

