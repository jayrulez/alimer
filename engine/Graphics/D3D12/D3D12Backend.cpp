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

namespace alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    PFN_D3D12_CREATE_DEVICE D3D12CreateDevice;
    PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface;
    PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignature;
    PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER D3D12CreateRootSignatureDeserializer;
    PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE D3D12SerializeVersionedRootSignature;
    PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER D3D12CreateVersionedRootSignatureDeserializer;
#endif

    D3D12Fence::D3D12Fence()
        : d3dFence(nullptr)
        , fenceEvent(INVALID_HANDLE_VALUE)
    {

    }

    D3D12Fence::~D3D12Fence()
    {
        Shutdown();
    }

    void D3D12Fence::Init(ID3D12Device* device, uint64 initialValue)
    {
        ThrowIfFailed(device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d3dFence)));
        fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
        ALIMER_ASSERT(fenceEvent != 0);
    }

    void D3D12Fence::Shutdown()
    {
        if (d3dFence == nullptr) {
            return;
        }

        CloseHandle(fenceEvent);
        SafeRelease(d3dFence);
    }

    void D3D12Fence::Signal(ID3D12CommandQueue* queue, uint64 fenceValue)
    {
        ALIMER_ASSERT(d3dFence != nullptr);
        ThrowIfFailed(queue->Signal(d3dFence, fenceValue));
    }

    void D3D12Fence::Wait(uint64 fenceValue)
    {
        ALIMER_ASSERT(d3dFence != nullptr);
        if (d3dFence->GetCompletedValue() < fenceValue)
        {
            ThrowIfFailed(d3dFence->SetEventOnCompletion(fenceValue, fenceEvent));
            WaitForSingleObject(fenceEvent, INFINITE);
        }
    }

    bool D3D12Fence::IsSignaled(uint64 fenceValue)
    {
        ALIMER_ASSERT(d3dFence != nullptr);
        return d3dFence->GetCompletedValue() >= fenceValue;
    }

    void D3D12Fence::Clear(uint64 fenceValue)
    {
        ALIMER_ASSERT(d3dFence != nullptr);
        ThrowIfFailed(d3dFence->Signal(fenceValue));
    }
}
