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

#include "D3D11Framebuffer.h"
#include "D3D11GPUDevice.h"

namespace alimer
{
    D3D11Framebuffer::D3D11Framebuffer(D3D11GPUDevice* device, const SwapChainDescriptor* descriptor)
        : Framebuffer(device)
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        window = reinterpret_cast<HWND>(descriptor->nativeWindowHandle);
        ALIMER_ASSERT(IsWindow(window));
        RECT rect;
        BOOL success = GetClientRect(window, &rect);
        ALIMER_ASSERT_MSG(success, "GetWindowRect error.");
        extent.width = rect.right - rect.left;
        extent.height = rect.bottom - rect.top;
#else
        IUnknown* = reinterpret_cast<IUnknown*>(descriptor->nativeWindowHandle);
#endif

        BackendResize();
    }

    D3D11Framebuffer::~D3D11Framebuffer()
    {
    }

    FramebufferResizeResult D3D11Framebuffer::BackendResize()
    {
        auto d3d11GPUDevice = static_cast<D3D11GPUDevice*>(device);

        const DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
        HRESULT hr = S_OK;
        if (handle != nullptr)
        {

        }
        else
        {
            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
            swapChainDesc.Width = extent.width;
            swapChainDesc.Height = extent.height;
            swapChainDesc.Format = backBufferFormat;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = backBufferCount;
            swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            IDXGIFactory4* factory4;
            if (FAILED(d3d11GPUDevice->GetDXGIFactory()->QueryInterface(&factory4)))
            {
                flipPresentSupported = false;
#ifdef _DEBUG
                OutputDebugStringA("INFO: Flip swap effects not supported");
#endif
                factory4->Release();
            }

            if (!flipPresentSupported)
            {
                swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
            }
#endif

            swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
            if (!d3d11GPUDevice->IsVSyncEnabled())
            {
                if (tearingSupported)
                {
                    swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
                }
            }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            DXGI_SWAP_CHAIN_FULLSCREEN_DESC swap_chain_fullscreen_desc = {};
            memset(&swap_chain_fullscreen_desc, 0, sizeof(swap_chain_fullscreen_desc));
            swap_chain_fullscreen_desc.Windowed = TRUE;

            hr = d3d11GPUDevice->GetDXGIFactory()->CreateSwapChainForHwnd(
                d3d11GPUDevice->GetD3DDevice(),
                window,
                &swapChainDesc,
                &swap_chain_fullscreen_desc,
                nullptr,
                &handle
            );

            // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
            ThrowIfFailed(d3d11GPUDevice->GetDXGIFactory()->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));
#else
            swapChainDesc.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;

            hr = d3d11GPUDevice->GetDXGIFactory()->CreateSwapChainForCoreWindow(
                d3d11GPUDevice->GetD3DDevice(),
                window,
                &swapChainDesc,
                nullptr,
                &handle
            );
#endif
        }

        return FramebufferResizeResult::Success;
    }

    HRESULT D3D11Framebuffer::present(UINT sync_interval, UINT flags)
    {
        return handle->Present(sync_interval, flags);
    }
}
