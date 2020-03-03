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
#include <algorithm>

namespace Alimer
{
    D3D12SwapChain::D3D12SwapChain(D3D12GraphicsDevice* device, void* nativeHandle, const SwapChainDescriptor* descriptor)
        : SwapChain(descriptor)
        , device{ device }
        , backBufferCount(2)
    {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        hwnd = reinterpret_cast<HWND>(nativeHandle);
        if (!IsWindow(hwnd))
        {
            ALIMER_LOGERROR("Invalid HWND handle");
        }
#else
        window = reinterpret_cast<IUnknown*>(nativeHandle);
#endif

        BackendResize();
    }

    D3D12SwapChain::~D3D12SwapChain()
    {
        Destroy();
    }

    void D3D12SwapChain::Destroy()
    {

    }

    void D3D12SwapChain::BackendResize()
    {
        // Determine the render target size in pixels.
        UINT backBufferWidth = std::max<UINT>(static_cast<UINT>(width), 1u);
        UINT backBufferHeight = std::max<UINT>(static_cast<UINT>(height), 1u);
        DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;

        if (handle)
        {
            // If the swap chain already exists, resize it.
            UINT swapChainFlags = 0;
            if (!vsync && device->IsTearingSupported())
            {
                swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
            }

            HRESULT hr = handle->ResizeBuffers(
                backBufferCount,
                backBufferWidth,
                backBufferHeight,
                backBufferFormat,
                swapChainFlags
            );

            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
#ifdef _DEBUG
                char buff[64] = {};
                sprintf_s(buff, "Device Lost on ResizeBuffers: Reason code 0x%08X\n", (hr == DXGI_ERROR_DEVICE_REMOVED) ? device->GetD3DDevice()->GetDeviceRemovedReason() : hr);
                OutputDebugStringA(buff);
#endif
                // If the device was removed for any reason, a new device and swap chain will need to be created.
                //device->HandleDeviceLost();

                // Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method
                // and correctly set up the new device.
                return;
            }
            else
            {
                ThrowIfFailed(hr);
            }
        }
        else
        {
            // Create a descriptor for the swap chain.
            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
            swapChainDesc.Width = backBufferWidth;
            swapChainDesc.Height = backBufferHeight;
            swapChainDesc.Format = backBufferFormat;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = backBufferCount;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
#else
            swapChainDesc.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
#endif
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

            swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
            if (!vsync && device->IsTearingSupported())
            {
                swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
            }

            // Create a swap chain for the window.
            ComPtr<IDXGISwapChain1> tempSwapChain;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDesc = {};
            swapChainFullscreenDesc.Windowed = TRUE;

            ThrowIfFailed(device->GetDXGIFactory()->CreateSwapChainForHwnd(
                device->GetD3DCommandQueue(CommandQueueType::Graphics),
                hwnd,
                &swapChainDesc,
                &swapChainFullscreenDesc,
                nullptr,
                tempSwapChain.GetAddressOf()
            ));

            // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
            ThrowIfFailed(device->GetDXGIFactory()->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
#else
            ThrowIfFailed(device->GetDXGIFactory()->CreateSwapChainForCoreWindow(
                device->GetD3DCommandQueue(CommandQueueType::Graphics),
                window,
                &swapChainDesc,
                nullptr,
                tempSwapChain.GetAddressOf()
            ));
#endif
            ThrowIfFailed(tempSwapChain.As(&handle));
        }
    }

    bool D3D12SwapChain::BackendPresent()
    {
        UINT syncInterval = vsync ? 1 : 0;
        UINT presentFlags = (device->IsTearingSupported() && !vsync) ? DXGI_PRESENT_ALLOW_TEARING : 0;

        HRESULT hr = handle->Present(syncInterval, presentFlags);
        if (hr == DXGI_ERROR_DEVICE_REMOVED
            || hr == DXGI_ERROR_DEVICE_HUNG
            || hr == DXGI_ERROR_DEVICE_RESET
            || hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR
            || hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
        {
            return false;
        }

        return true;
    }
}
