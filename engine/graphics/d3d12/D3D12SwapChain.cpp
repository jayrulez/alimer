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
#include "D3D12GPUDevice.h"

namespace alimer
{
    D3D12SwapChain::D3D12SwapChain(D3D12GPUDevice* device, const SwapChainDescriptor* descriptor)
        : D3DSwapChain(device, device->GetDXGIFactory(), device->GetD3DCommandQueue(), 2u, descriptor)
    {
    }

    D3D12SwapChain::~D3D12SwapChain()
    {
        Destroy();
    }

    void D3D12SwapChain::Destroy()
    {
        D3DSwapChain::Destroy();
    }

    void D3DSwapChain::AfterReset()
    {
        for (uint32_t i = 0; i < backBufferCount; i++)
        {
            //backBuffers[i].RTV = DX12::RTVDescriptorHeap.AllocatePersistent().Handles[0];
            ID3D12Resource* backbuffer;
            ThrowIfFailed(handle->GetBuffer(i, __uuidof(ID3D12Resource), (void**)&backbuffer));

#if defined(_DEBUG)
            wchar_t name[25] = {};
            swprintf_s(name, L"Render target %u", i);
            backbuffer->SetName(name);
#endif
        }

        //m_backBufferIndex = handle->GetCurrentBackBufferIndex();
    }

    void D3D12SwapChain::AfterReset()
    {

    }
}
