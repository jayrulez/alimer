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
#include "D3D12GPUDevice.h"
#include "core/Assert.h"

namespace alimer
{
    D3D12GPUFence::D3D12GPUFence(D3D12GPUDevice* device_)
        : device(device_)
    {
    }

    D3D12GPUFence::~D3D12GPUFence()
    {
        Shutdown();
    }

    void D3D12GPUFence::Init(uint64_t initialValue)
    {
        ThrowIfFailed(device->GetD3DDevice()->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&handle));
        fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        ALIMER_ASSERT(fenceEvent != INVALID_HANDLE_VALUE);
    }

    void D3D12GPUFence::Shutdown()
    {
        if (handle == nullptr)
            return;

        CloseHandle(fenceEvent);
        device->DeferredRelease(handle);
    }

    void D3D12GPUFence::Signal(ID3D12CommandQueue* queue, uint64_t fenceValue)
    {
        ALIMER_ASSERT(handle != nullptr);
        ThrowIfFailed(queue->Signal(handle, fenceValue));
    }

    void D3D12GPUFence::Wait(uint64_t fenceValue)
    {
        ALIMER_ASSERT(handle != nullptr);
        if (handle->GetCompletedValue() < fenceValue)
        {
            ThrowIfFailed(handle->SetEventOnCompletion(fenceValue, fenceEvent));
            WaitForSingleObject(fenceEvent, INFINITE);
        }
    }

    bool D3D12GPUFence::IsSignaled(uint64_t fenceValue)
    {
        ALIMER_ASSERT(handle != nullptr);
        return handle->GetCompletedValue() >= fenceValue;
    }

    void D3D12GPUFence::Clear(uint64_t fenceValue)
    {
        ALIMER_ASSERT(handle != nullptr);
        handle->Signal(fenceValue);
    }
}
