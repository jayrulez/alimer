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
#include "D3D12CommandBuffer.h"
#include "D3D12Texture.h"
#include "D3D12GraphicsDevice.h"

namespace Alimer
{
    D3D12SwapChain::D3D12SwapChain(D3D12GraphicsDevice* device, const SwapChainDescription& desc)
        : SwapChain(desc)
        , device{ device }
       // , isTearingSupported(device->IsTearingSupported())
        , syncInterval(desc.vsync ? 1 : 0)
    {
        UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        if (isTearingSupported)
        {
            swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }

        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = desc.width;
        swapChainDesc.Height = desc.height;
        swapChainDesc.Format = ToDXGIFormat(SRGBToLinearFormat(desc.colorFormat));
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = kBackBufferCount;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
#else
        swapChainDesc.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
#endif
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapChainDesc.Flags = swapChainFlags;

        ComPtr<IDXGISwapChain1> swapChain1;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        HWND window = (HWND)desc.windowHandle;
        ALIMER_ASSERT(IsWindow(window));

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = !desc.fullscreen;

        // Create a swap chain for the window.
        /*ThrowIfFailed(device->GetDXGIFactory()->CreateSwapChainForHwnd(
            device->GetGraphicsQueue(),
            window,
            &swapChainDesc,
            &fsSwapChainDesc,
            nullptr,
            swapChain1.GetAddressOf()
        ));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        ThrowIfFailed(device->GetDXGIFactory()->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));*/
#else
        IUnknown* window = (IUnknown*)desc.windowHandle;
        ThrowIfFailed(device->GetDXGIFactory()->CreateSwapChainForCoreWindow(
            device->GetGraphicsQueue(),
            window,
            &swapChainDesc,
            nullptr,
            swapChain1.GetAddressOf()
        ));
#endif

        ThrowIfFailed(swapChain1->QueryInterface(&dxgiSwapChain3));

        commandBuffer = new D3D12CommandBuffer(device);
        AfterReset();
    }

    D3D12SwapChain::~D3D12SwapChain()
    {
        Destroy();
    }

    void D3D12SwapChain::Destroy()
    {
        SafeDelete(commandBuffer);
        SafeRelease(dxgiSwapChain3);
    }

    void D3D12SwapChain::AfterReset()
    {
        colorTextures.clear();
        currentBackBufferIndex = dxgiSwapChain3->GetCurrentBackBufferIndex();

        for (uint32_t i = 0; i < kBackBufferCount; ++i)
        {
            ComPtr<ID3D12Resource> backBuffer;
            ThrowIfFailed(dxgiSwapChain3->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

            colorTextures.push_back(MakeRefPtr<D3D12Texture>(device, backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT));
        }
    }

    void D3D12SwapChain::BeginFrame()
    {
        currentBackBufferIndex = dxgiSwapChain3->GetCurrentBackBufferIndex();

        commandBuffer->Reset(currentBackBufferIndex);

        // Indicate that the back buffer will be used as a render target.
        StaticCast<D3D12Texture>(colorTextures[currentBackBufferIndex])->TransitionBarrier(commandBuffer->GetCommandList(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    }

   /* FrameOpResult D3D12SwapChain::EndFrame(ID3D12CommandQueue* queue, EndFrameFlags flags)
    {
        // Indicate that the back buffer will now be used to present.
        StaticCast<D3D12Texture>(colorTextures[currentBackBufferIndex])->TransitionBarrier(commandBuffer->GetCommandList(), D3D12_RESOURCE_STATE_PRESENT);

        ThrowIfFailed(commandBuffer->GetCommandList()->Close());

        ID3D12CommandList* commandLists[] = { commandBuffer->GetCommandList() };
        queue->ExecuteCommandLists(1, commandLists);


        // Present swap chains.
        HRESULT hr = S_OK;
        if (!any(flags & EndFrameFlags::SkipPresent))
        {
            uint32_t presentFlags = 0;

            // Recommended to always use tearing if supported when using a sync interval of 0.
            // Note this will fail if in true 'fullscreen' mode.
            if (!syncInterval && !isFullscreen && isTearingSupported)
            {
                presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
            }

            hr = dxgiSwapChain3->Present(syncInterval, presentFlags);

            // If the device was reset we must completely reinitialize the renderer.
            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
                device->HandleDeviceLost();
                return FrameOpResult::DeviceLost;
            }
            else if (FAILED(hr))
            {
                return FrameOpResult::Error;
            }
        }

        return FrameOpResult::Success;
    }*/

    CommandBuffer* D3D12SwapChain::CurrentFrameCommandBuffer()
    {
        return commandBuffer;
    }
}
