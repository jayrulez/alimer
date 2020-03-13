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
#include "Diagnostics/Log.h"
#include "Diagnostics/Assert.h"

namespace alimer
{
    D3DSwapChain::D3DSwapChain(IDXGIFactory2* factory, IUnknown* deviceOrCommandQueue, void* nativeWindow, const SwapChainDescriptor& desc)
        : SwapChain(desc)
        , factory{ factory }
        , deviceOrCommandQueue{ deviceOrCommandQueue }
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        window = (HWND)nativeWindow;
        ALIMER_ASSERT(IsWindow(window));

        RECT rect;
        BOOL success = GetClientRect(window, &rect);
        ALIMER_ASSERT(success);
        extent.width = rect.right - rect.left;
        extent.height = rect.bottom - rect.top;
#else
        window = (IUnknown*)nativeWindow;
#endif

        Resize(extent.width, extent.height);
    }

    D3DSwapChain::~D3DSwapChain()
    {
        if (_handle) {
            _handle->Release();
            _handle = nullptr;
        }
    }

    SwapChainResizeResult D3DSwapChain::Resize(uint32_t newWidth, uint32_t newHeight)
    {
        HRESULT hr = S_OK;
        if (_handle)
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

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            IDXGIFactory4* factory4;
            if (FAILED(factory->QueryInterface(&factory4)))
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
            if (!vsync)
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
                    }
                    factory5->Release();
                }

                if (tearingSupported)
                {
                    swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
                }
            }

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            DXGI_SWAP_CHAIN_FULLSCREEN_DESC swap_chain_fullscreen_desc = {};
            memset(&swap_chain_fullscreen_desc, 0, sizeof(swap_chain_fullscreen_desc));
            swap_chain_fullscreen_desc.Windowed = TRUE;

            hr = factory->CreateSwapChainForHwnd(deviceOrCommandQueue,
                window,
                &swapChainDesc,
                &swap_chain_fullscreen_desc,
                nullptr,
                (IDXGISwapChain1**)&_handle
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
        UINT syncInterval = vsync ? 1 : 0;
        UINT presentFlags = (tearingSupported && !vsync) ? DXGI_PRESENT_ALLOW_TEARING : 0;

        HRESULT hr = _handle->Present(syncInterval, presentFlags);
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
