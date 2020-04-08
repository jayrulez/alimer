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
#include "D3D12Texture.h"

#include "math/math.h"
#if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
#include <windows.ui.xaml.media.dxinterop.h>
#endif

namespace alimer
{
    D3D12SwapChain::D3D12SwapChain(D3D12GraphicsDevice* device_, IDXGIFactory4* factory, GraphicsSurface* surface, uint32_t backBufferCount_)
        : device(device_)
        , backBufferCount(backBufferCount_)
    {
        // Determine the render target size in pixels.
        const uint32_t backBufferWidth = max<uint32_t>(surface->GetSize().width, 1u);
        const uint32_t backBufferHeight = max<uint32_t>(surface->GetSize().height, 1u);
        const DXGI_FORMAT backBufferFormat = ToDXGISwapChainFormat(PixelFormat::BGRA8UNorm);

        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = backBufferWidth;
        swapChainDesc.Height = backBufferHeight;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = backBufferCount;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        //swapChainDesc.Flags = (m_options & c_AllowTearing) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u;

        // Create a swap chain for the window.
        IDXGISwapChain1* tempSwapChain;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
        if (surface->GetType() == GraphicsSurfaceType::Win32)
        {
            DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
            fsSwapChainDesc.Windowed = TRUE;

            HWND hwnd = reinterpret_cast<HWND>(surface->GetHandle());
            ThrowIfFailed(factory->CreateSwapChainForHwnd(
                device->GetD3D12GraphicsQueue(),
                hwnd,
                &swapChainDesc,
                &fsSwapChainDesc,
                nullptr,
                &tempSwapChain
            ));

            // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
            ThrowIfFailed(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
        }
#else
        if (surface->GetType() == GraphicsSurfaceType::UwpCoreWindow)
        {
            IUnknown* window = reinterpret_cast<IUnknown*>(surface->GetHandle());

            ThrowIfFailed(factory->CreateSwapChainForCoreWindow(
                device->GetD3D12GraphicsQueue(),
                window,
                &swapChainDesc,
                nullptr,
                &tempSwapChain
            ));
        }
        else if (surface->GetType() == GraphicsSurfaceType::UwpSwapChainPanel)
        {
            IInspectable* swapChainPanel = reinterpret_cast<IInspectable*>(surface->GetHandle());
            ThrowIfFailed(factory->CreateSwapChainForComposition(
                device->GetD3D12GraphicsQueue(),
                &swapChainDesc,
                nullptr,
                &tempSwapChain
            ));

            ISwapChainPanelNative* panelNative;
            ThrowIfFailed(swapChainPanel->QueryInterface(__uuidof(ISwapChainPanelNative), (void**)panelNative));
            ThrowIfFailed(panelNative->SetSwapChain(tempSwapChain));
        }
#endif

        ThrowIfFailed(tempSwapChain->QueryInterface(IID_PPV_ARGS(&handle)));
        SafeRelease(tempSwapChain);
        AfterReset();
    }

    D3D12SwapChain::~D3D12SwapChain()
    {
        Destroy();
    }

    void D3D12SwapChain::Destroy()
    {
        if (handle != nullptr)
        {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            handle->SetFullscreenState(FALSE, nullptr);
#endif
            handle->Release();
            handle = nullptr;
        }
    }

    void D3D12SwapChain::AfterReset()
    {
        for (uint32_t i = 0; i < backBufferCount; i++)
        {
            //backBuffers[i].RTV = DX12::RTVDescriptorHeap.AllocatePersistent().Handles[0];
            ID3D12Resource* backbuffer;
            ThrowIfFailed(handle->GetBuffer(i, __uuidof(ID3D12Resource), (void**)&backbuffer));

#if defined(_DEBUG)
            wchar_t name[25] = {};
            swprintf_s(name, L"Render target %u", i);
            backbuffer->SetName(name);
#endif

            renderTargets[i] = new D3D12Texture(device, PixelFormat::BGRA8UNorm, backbuffer, D3D12_RESOURCE_STATE_PRESENT);
        }

        backBufferIndex = handle->GetCurrentBackBufferIndex();
    }

    void D3D12SwapChain::Present()
    {
        HRESULT hr = handle->Present(1, 0);

        if (hr == DXGI_ERROR_DEVICE_REMOVED ||
            hr == DXGI_ERROR_DEVICE_RESET)
        {
        }
        else
        {
            ThrowIfFailed(hr);

            backBufferIndex = handle->GetCurrentBackBufferIndex();
        }
    }
}

