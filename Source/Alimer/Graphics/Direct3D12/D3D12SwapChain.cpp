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

#include "D3D12GraphicsDevice.h"
#include "D3D12SwapChain.h"
#include "D3D12Texture.h"

namespace alimer
{
    D3D12SwapChain::D3D12SwapChain(D3D12GraphicsDevice* device, void* window, const SwapChainDescriptor* descriptor)
        : SwapChain(*device, descriptor)
    {
        // Flip mode doesn't support SRGB formats
        dxgiColorFormat = ToDXGIFormat(srgbToLinearFormat(descriptor->colorFormat));

        // Create a SwapChain from a Win32 window.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = descriptor->width;
        swapChainDesc.Height = descriptor->height;
        swapChainDesc.Format = dxgiColorFormat;
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = kMaxFrameLatency;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        if (device->IsTearingSupported())
        {
            swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = !descriptor->isFullscreen;

        IDXGISwapChain1* tempSwapChain;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        HWND hwnd = (HWND)window;
        VHR(device->GetDXGIFactory()->CreateSwapChainForHwnd(
            device->GetD3DCommandQueue(),
            hwnd,
            &swapChainDesc,
            &fsSwapChainDesc,
            NULL,
            &tempSwapChain
        ));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        VHR(device->GetDXGIFactory()->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
#else
        ThrowIfFailed(device->GetDXGIFactory()->CreateSwapChainForCoreWindow(
            device->GetD3DCommandQueue(),
            (IUnknown*)window,
            &swapChainDesc,
            nullptr,
            &tempSwapChain
        ));
#endif

        VHR(tempSwapChain->QueryInterface(IID_PPV_ARGS(&handle)));
        tempSwapChain->Release();
        
        CreateRenderTargets();
    }

    D3D12SwapChain::~D3D12SwapChain()
    {
        Destroy();
    }

    void D3D12SwapChain::Destroy()
    {
        if (handle == nullptr) {
            return;
        }

        for (uint32_t i = 0; i < kMaxFrameLatency; ++i)
        {
            SafeDelete(colorTextures[i]);
        }

        SAFE_RELEASE(handle);
    }

    void D3D12SwapChain::Present()
    {
        HRESULT hr = handle->Present(syncInterval, presentFlags);
        // If the device was reset we must completely reinitialize the renderer.
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
        }
    }

    void D3D12SwapChain::CreateRenderTargets()
    {
        for (uint32_t index = 0; index < kMaxFrameLatency; ++index)
        {
            ID3D12Resource* backbuffer;
            VHR(handle->GetBuffer(index, IID_PPV_ARGS(&backbuffer)));
            colorTextures[index] = D3D12Texture::CreateFromExternal(device, backbuffer, GraphicsResource::State::Present);
            //Gfx.Device->CreateRenderTargetView(Gfx.SwapBuffers[Idx]->Raw, nullptr, Handle);
            //Handle.ptr += Gfx.DescriptorSizeRTV;
        }

        backbufferIndex = handle->GetCurrentBackBufferIndex();
    }
}

