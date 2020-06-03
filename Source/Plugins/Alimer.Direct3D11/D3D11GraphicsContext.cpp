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

#include "D3D11GraphicsContext.h"
#include "D3D11GraphicsDevice.h"
//#include "D3D11Texture.h"
#include "core/Log.h"
#include "core/Assert.h"

namespace alimer
{
    D3D11GraphicsContext::D3D11GraphicsContext(D3D11GraphicsDevice* device_, const GraphicsContextDescription& desc)
        : GraphicsContext(*device, desc)
        , device(device_)
        , factory(device->GetDXGIFactory())
        , deviceOrCommandQueue(device->GetD3DDevice())
        , backBufferCount(2u)
    {
        if (desc.handle)
        {
            // Flip mode doesn't support SRGB formats
            //dxgiColorFormat = ToDXGIFormat(srgbToLinearFormat(desc.colorFormat));

            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
            swapChainDesc.Width = desc.width;
            swapChainDesc.Height = desc.height;
            swapChainDesc.Format = dxgiColorFormat;
            swapChainDesc.Stereo = FALSE;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = backBufferCount;
            swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
            if (device->IsTearingSupported())
            {
                swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
            }

            DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
            fsSwapChainDesc.Windowed = !desc.isFullscreen;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            HWND hwnd = (HWND)desc.handle;
            if (!IsWindow(hwnd)) {
                LOG_ERROR("Invalid HWND handle");
                return;
            }


            VHR(device->GetDXGIFactory()->CreateSwapChainForHwnd(
                device->GetD3DDevice(),
                hwnd,
                &swapChainDesc,
                &fsSwapChainDesc,
                NULL,
                swapChain.ReleaseAndGetAddressOf()
            ));

            // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
            VHR(device->GetDXGIFactory()->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
#else
            ThrowIfFailed(device->GetDXGIFactory()->CreateSwapChainForCoreWindow(
                device->GetD3DCommandQueue(),
                (IUnknown*)window,
                &swapChainDesc,
                nullptr,
                swapChain.ReleaseAndGetAddressOf()
            ));
#endif
        }
        else
        {
            dxgiColorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        }
    }

    D3D11GraphicsContext::~D3D11GraphicsContext()
    {
        Destroy();
    }

    void D3D11GraphicsContext::Destroy()
    {
        if (swapChain)
        {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            swapChain->SetFullscreenState(FALSE, nullptr);
#endif

        }
    }

#if TODO
    SwapChain::ResizeResult D3D11SwapChain::ResizeImpl(uint32_t width, uint32_t height)
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

    void D3D11SwapChain::AfterReset()
    {
        ComPtr<ID3D11Texture2D> renderTarget;
        ThrowIfFailed(handle->GetBuffer(0, IID_PPV_ARGS(renderTarget.GetAddressOf())));
        TextureDescriptor textureDesc = {};
        textureDesc.usage = TextureUsage::RenderTarget | TextureUsage::Sampled;
        textureDesc.extent.width = extent.width;
        textureDesc.extent.height = extent.height;
        textureDesc.format = colorFormat;
        textureDesc.externalHandle = (void*)renderTarget.Get();
        textures.push_back(MakeRefPtr<D3D11Texture>(_device, &textureDesc));
    }
#endif // TODO


    void D3D11GraphicsContext::Present()
    {
        HRESULT hr = swapChain->Present(syncInterval, presentFlags);
        // If the device was reset we must completely reinitialize the renderer.
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
        }

        VHR(hr);
    }
}
