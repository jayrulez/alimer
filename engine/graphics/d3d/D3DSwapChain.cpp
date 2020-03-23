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

#include "D3DSwapChain.h"
#include "core/Log.h"
#include "core/Assert.h"

namespace alimer
{
    D3DSwapChain::D3DSwapChain(GPUDevice* device, IDXGIFactory2* factory_, IUnknown* deviceOrCommandQueue_, const SwapChainDescriptor* descriptor)
        : SwapChain(device, descriptor)
        , factory(factory_)
        , deviceOrCommandQueue(deviceOrCommandQueue_)
        , syncInterval(descriptor->presentMode == PresentMode::Immediate ? 0 : 1)
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        window = reinterpret_cast<HWND>(descriptor->nativeWindowHandle);
        ALIMER_ASSERT(IsWindow(window));

        RECT rect;
        BOOL success = GetClientRect(window, &rect);
        ALIMER_ASSERT(success);
        extent.width = rect.right - rect.left;
        extent.height = rect.bottom - rect.top;
#else
        window = (IUnknown*)nativeWindow;
#endif

        if (!syncInterval)
        {
            // Determines whether tearing support is available for fullscreen borderless windows.
            IDXGIFactory5* factory5;
            HRESULT hr = factory->QueryInterface(&factory5);
            if (SUCCEEDED(hr))
            {
                BOOL allowTearing = FALSE;
                HRESULT tearingSupportHR = factory5->CheckFeatureSupport(
                    DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing)
                    );

                if (SUCCEEDED(tearingSupportHR) && allowTearing)
                {
                    tearingSupported = true;
                    presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
                    swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
                }
                factory5->Release();
            }
        }

        Resize(extent.width, extent.height);
    }

    D3DSwapChain::~D3DSwapChain()
    {
        Destroy();
    }

    void D3DSwapChain::Destroy()
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

    SwapChainResizeResult D3DSwapChain::Resize(uint32_t newWidth, uint32_t newHeight)
    {
        HRESULT hr = S_OK;
        if (handle != nullptr)
        {

        }
        else
        {
            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
            swapChainDesc.Width = newWidth;
            swapChainDesc.Height = newHeight;
            swapChainDesc.Format = backBufferFormat;
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

        return SwapChainResizeResult::Success;
    }

    void D3DSwapChain::Present()
    {
        HRESULT hr = handle->Present(syncInterval, presentFlags);
        if (hr == DXGI_ERROR_DEVICE_REMOVED
            || hr == DXGI_ERROR_DEVICE_HUNG
            || hr == DXGI_ERROR_DEVICE_RESET
            || hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR
            || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
        {
            //return false;
        }
    }
}
