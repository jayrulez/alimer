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

#include "D3D12Backend.h"
#include "graphics/SwapChain.h"
#include "graphics/GPUDevice.h"
#include "graphics/CommandQueue.h"

namespace alimer
{
    namespace
    {
        inline UINT GetSyncInterval(PresentMode interval)
        {
            switch (interval)
            {
            case PresentMode::Fifo:
                return 1;
            case PresentMode::Mailbox:
                return 2;
            case PresentMode::Immediate:
                return 0;
            default:
                ALIMER_UNREACHABLE();
                return (UINT)-1;
                break;
            }
        }
    }

    void SwapChain::Destroy()
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

    SwapChain::ResizeResult SwapChain::ApiResize()
    {
        HRESULT hr = S_OK;

        auto dxgiFactory = GetDXGIFactory();
        DXGI_FORMAT dxgiColorFormat = ToDXGISwapChainFormat(colorFormat);

        UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        if (presentMode == PresentMode::Immediate
            && IsDXGITearingSupported())
        {
            //presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
            swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }

        if (handle != nullptr)
        {
            hr = handle->ResizeBuffers(
                imageCount,
                extent.width, extent.height,
                dxgiColorFormat, swapChainFlags);
        }
        else
        {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            HWND window = reinterpret_cast<HWND>(windowHandle);
            ALIMER_ASSERT(IsWindow(window));
#else
            IUnknown* window = reinterpret_cast<IUnknown*>(descriptor->platformData.windowHandle);
#endif

            // Create a descriptor for the swap chain.
            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
            swapChainDesc.Width = extent.width;
            swapChainDesc.Height = extent.height;
            swapChainDesc.Format = dxgiColorFormat;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = imageCount;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
            swapChainDesc.Flags = swapChainFlags;

            auto d3dCommandQueue = device.GetCommandQueue(CommandQueueType::Graphics)->GetHandle();
            IDXGISwapChain1* tempSwapChain;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
            fsSwapChainDesc.Windowed = TRUE;

            // Create a swap chain for the window.
            ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
                d3dCommandQueue,
                window,
                &swapChainDesc,
                &fsSwapChainDesc,
                nullptr,
                &tempSwapChain
            ));

            // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
            ThrowIfFailed(dxgiFactory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));
#else
            ThrowIfFailed(dxgiFactory->CreateSwapChainForCoreWindow(
                d3dCommandQueue,
                window,
                &swapChainDesc,
                nullptr,
                &tempSwapChain
            ));
#endif
            ThrowIfFailed(tempSwapChain->QueryInterface(IID_PPV_ARGS(&handle)));
            SAFE_RELEASE(tempSwapChain);
        }

        return ResizeResult::Success;
    }
}

