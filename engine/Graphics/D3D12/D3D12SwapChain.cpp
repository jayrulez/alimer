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
#include "D3D12CommandQueue.h"
#include "D3D12GraphicsDevice.h"

namespace Alimer
{
    D3D12SwapChain::D3D12SwapChain(D3D12GraphicsDevice* device_, const SwapChainDescription& desc)
        : SwapChain(desc)
        , device(device_)
        , isTearingSupported(device->IsTearingSupported())
        , syncInterval(desc.presentMode == PresentMode::Immediate ? 0 : 1)
        , fenceValues{}
        , frameValues{}
    {
        ComPtr<IDXGISwapChain1> swapChain1 = DXGICreateSwapChain(
            device->GetDXGIFactory(),
            device->GetDXGIFactoryCaps(),
            static_cast<D3D12CommandQueue*>(device->GetCommandQueue(CommandQueueType::Graphics))->GetHandle(),
            kBackBufferCount,
            desc);
        ThrowIfFailed(swapChain1.As(&handle));
        currentBackBufferIndex = handle->GetCurrentBackBufferIndex();

        if (desc.label && strlen(desc.label))
        {
            DXGISetObjectName(handle.Get(), desc.label);
        }
    }

    bool D3D12SwapChain::Present()
    {
        auto commandQueue = static_cast<D3D12CommandQueue*>(device->GetCommandQueue(CommandQueueType::Graphics));

        uint32_t presentFlags = 0;

        // Recommended to always use tearing if supported when using a sync interval of 0.
        // Note this will fail if in true 'fullscreen' mode.
        if (!syncInterval && !isFullscreen && isTearingSupported)
        {
            presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
        }

        HRESULT result = handle->Present(syncInterval, presentFlags);

        // If the device was reset we must completely reinitialize the renderer.
        if (result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET || result == DXGI_ERROR_DRIVER_INTERNAL_ERROR)
        {
            device->HandleDeviceLost();
            return false;
        }

        fenceValues[currentBackBufferIndex] = commandQueue->Signal();
        frameValues[currentBackBufferIndex] = device->GetFrameCount();

        currentBackBufferIndex = handle->GetCurrentBackBufferIndex();
        commandQueue->WaitForFenceValue(fenceValues[currentBackBufferIndex]);

        return SUCCEEDED(result);
    }
}
