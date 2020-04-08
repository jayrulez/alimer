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

#include "D3D12CommandContext.h"
#include "D3D12CommandQueue.h"
#include "D3D12GraphicsDevice.h"
#include <algorithm>

namespace alimer
{
    D3D12GraphicsContext::D3D12GraphicsContext(D3D12GraphicsDevice* device_, D3D12_COMMAND_LIST_TYPE type_, uint32_t commandAllocatorsCount_)
        : GraphicsContext(device_)
        , type(type_)
        , commandAllocatorsCount(commandAllocatorsCount_)
    {
        for (uint32_t i = 0; i < commandAllocatorsCount_; ++i)
        {
            ThrowIfFailed(device_->GetD3DDevice()->CreateCommandAllocator(type_, IID_PPV_ARGS(&commandAllocators[i])));
        }

        ThrowIfFailed(device_->GetD3DDevice()->CreateCommandList(0, type_, commandAllocators[0], nullptr, IID_PPV_ARGS(&commandList)));
        ThrowIfFailed(commandList->Close());
    }

    D3D12GraphicsContext::~D3D12GraphicsContext()
    {
        Destroy();
    }

    void D3D12GraphicsContext::Destroy()
    {
        for (uint32_t i = 0; i < commandAllocatorsCount; ++i)
        {
            SafeRelease(commandAllocators[i]);
        }

        SafeRelease(commandList);
    }

    void D3D12GraphicsContext::Begin(const char* name, bool profile)
    {
        isProfiling = profile;

        ThrowIfFailed(commandAllocators[frameIndex]->Reset());
        ThrowIfFailed(commandList->Reset(commandAllocators[frameIndex], nullptr));

        /*ID3D12DescriptorHeap* heaps[] = { context->m_Device->m_SRVHeap.m_DescHeap, context->m_Device->m_SamplerHeap.m_DescHeap };
        commandList->SetDescriptorHeaps(2, heaps);*/

        BeginMarker(name);
    }

    void D3D12GraphicsContext::End()
    {
        EndMarker();

        commandList->Close();

        frameIndex = (frameIndex+1) % commandAllocatorsCount;
    }

    void D3D12GraphicsContext::BeginMarker(const char* name)
    {
        PIXBeginEvent(commandList, 0, name);

        if (isProfiling)
        {
        }
    }

    void D3D12GraphicsContext::EndMarker()
    {
        if (isProfiling)
        {
        }

        PIXEndEvent(commandList);
    }
}
