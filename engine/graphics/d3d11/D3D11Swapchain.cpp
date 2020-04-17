//
// Copyright (c) -2020 Amer Koleci and contributors.
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

#include "D3D11Swapchain.h"
#include "D3D11GraphicsDevice.h"
#include "D3D11Texture.h"
#include "core/Log.h"
#include "core/Assert.h"

namespace alimer
{
    D3D11Swapchain::D3D11Swapchain(D3D11GraphicsDevice* device, const PresentationParameters& parameters, uint32_t backBufferCount_)
        : Swapchain(*device, parameters)
        , factory(device->GetDXGIFactory())
        , deviceOrCommandQueue(device->GetD3DDevice())
        , dxgiColorFormat(ToDXGISwapChainFormat(parameters.colorFormat))
        , backBufferCount(backBufferCount_)
        , syncInterval(GetSyncInterval(parameters.presentationInterval))
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        window = reinterpret_cast<HWND>(parameters.platformData.windowHandle);
        ALIMER_ASSERT(IsWindow(window));
#else
        window = reinterpret_cast<IUnknown*>(parameters.platformData.windowHandle);
#endif

        swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        if (!syncInterval
            && device->IsTearingSupported())
        {
            presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
            swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }

        ResizeImpl(parameters.extent.width, parameters.extent.height);
    }

    D3D11Swapchain::~D3D11Swapchain()
    {
        Destroy();
    }

    void D3D11Swapchain::Destroy()
    {
        if (handle)
        {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            handle->SetFullscreenState(FALSE, nullptr);
#endif
            handle->Release();
            handle = nullptr;
        }
    }

    Swapchain::ResizeResult D3D11Swapchain::ResizeImpl(uint32_t width, uint32_t height)
    {
        HRESULT hr = S_OK;

        if (handle != nullptr)
        {
            HRESULT hr = handle->ResizeBuffers(
                backBufferCount,
                width, height,
                dxgiColorFormat, swapChainFlags);
        }
        else
        {
            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
            swapChainDesc.Width = width;
            swapChainDesc.Height = height;
            swapChainDesc.Format = dxgiColorFormat;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = backBufferCount;
            swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
            swapChainDesc.Flags = swapChainFlags;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
            fsSwapChainDesc.Windowed = TRUE;

            hr = factory->CreateSwapChainForHwnd(deviceOrCommandQueue,
                window,
                &swapChainDesc,
                &fsSwapChainDesc,
                nullptr,
                &handle
            );

            // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
            ThrowIfFailed(factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));
#else
            swapChainDesc.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;

            hr = factory->CreateSwapChainForCoreWindow(deviceOrCommandQueue,
                window,
                &swapChainDesc,
                nullptr,
                (IDXGISwapChain1**)&_handle
            );
#endif
        }

        AfterReset();

        return ResizeResult::Success;
    }

    void D3D11Swapchain::AfterReset()
    {
        ComPtr<ID3D11Texture2D> renderTarget;
        ThrowIfFailed(handle->GetBuffer(0, IID_PPV_ARGS(renderTarget.GetAddressOf())));
        TextureDescriptor textureDesc = {};
        textureDesc.usage = TextureUsage::RenderTarget | TextureUsage::Sampled;
        textureDesc.extent.width =  extent.width;
        textureDesc.extent.height = extent.height;
        textureDesc.format = colorFormat;
        textureDesc.externalHandle = (void*)renderTarget.Get();
        textures.push_back(new D3D11Texture(&device, &textureDesc));
    }

    HRESULT D3D11Swapchain::Present()
    {
        return handle->Present(syncInterval, presentFlags);
    }
}
