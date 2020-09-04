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

namespace Alimer
{
    D3D12SwapChain::D3D12SwapChain(D3D12GraphicsDevice* device, const SwapChainDescription& desc)
        : SwapChain(device, desc)
        , isTearingSupported(device->IsTearingSupported())
    {
        IDXGISwapChain1* tempSwapChain = DXGICreateSwapChain(
            device->GetDXGIFactory(),
            device->GetDXGIFactoryCaps(),
            device->GetGraphicsQueue(),
            desc);
        ThrowIfFailed(tempSwapChain->QueryInterface(IID_PPV_ARGS(&handle)));
        SafeRelease(tempSwapChain);

        // Frame fence.
        ThrowIfFailed(device->GetD3DDevice()->CreateFence(fenceValues[frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frameFence)));
        fenceValues[frameIndex]++;

        frameFenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
    }

    D3D12SwapChain::~D3D12SwapChain()
    {
        Destroy();
    }

    void D3D12SwapChain::Destroy()
    {
        CloseHandle(frameFenceEvent);
        SafeRelease(frameFence);
        SafeRelease(handle);
    }

    void D3D12SwapChain::BackendSetName()
    {
        DXGISetObjectName(handle, name);
    }

    bool D3D12SwapChain::Present(bool verticalSync)
    {
        UINT syncInterval = verticalSync ? 1 : 0;
        UINT presentFlags = 0;

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
            static_cast<D3D12GraphicsDevice*>(device.Get())->HandleDeviceLost();
            return false;
        }

        /*auto d3dDevice = static_cast<D3D12GraphicsDevice*>(device.Get());

        // Schedule a signal so that we know when the just-presented frame
        // has finished rendering. This will be checked in GetNextRenderTarget.
        uint64_t currentFenceValue = fenceValues[frameIndex];
        d3dDevice->GetGraphicsQueue()->Signal(frameFence, currentFenceValue);

        // Advance the frame index.
        frameIndex = handle->GetCurrentBackBufferIndex();

        // Check to see if the next frame is ready to start.
        LOGI("CPU Frame: %llu - GPU Frame: %llu", fenceValues[frameIndex], frameFence->GetCompletedValue());

        if (frameFence->GetCompletedValue() < fenceValues[frameIndex])
        {
            LOGI("Waiting on fence value");
            frameFence->SetEventOnCompletion(fenceValues[frameIndex], frameFenceEvent);
            WaitForSingleObjectEx(frameFenceEvent, INFINITE, FALSE);
        }

        // Set the fence value for the next frame.
        fenceValues[frameIndex] = currentFenceValue + 1;*/

        return SUCCEEDED(result);
    }
}
