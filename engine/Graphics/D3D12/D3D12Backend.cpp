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
#include "D3D12GraphicsDevice.h"

namespace Alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface;
    PFN_D3D12_CREATE_DEVICE D3D12CreateDevice;
#endif

    void D3D12SetObjectName(ID3D12Object* obj, const std::string& name)
    {
#ifdef _DEBUG
        if (obj != nullptr)
        {
            if (!name.empty())
            {
                std::wstring wideStr = ToUtf16(name);
                obj->SetName(wideStr.c_str());
            }
            else
                obj->SetName(nullptr);
        }
#else
        ALIMER_UNUSED(obj);
        ALIMER_UNUSED(name);
#endif
    }

    // ========= Fence =========
    D3D12Fence::~D3D12Fence()
    {
        Shutdown();
    }

    void D3D12Fence::Init(D3D12GraphicsDevice* device, uint64 initialValue)
    {
        this->device = device;
        ThrowIfFailed(device->GetD3DDevice()->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d3dFence)));
        fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
        ALIMER_ASSERT(fenceEvent != 0);
    }

    void D3D12Fence::Shutdown()
    {
        if (!d3dFence)
            return;

        CloseHandle(fenceEvent);
        device->DeferredRelease(d3dFence);
    }

    void D3D12Fence::Signal(ID3D12CommandQueue* queue, uint64 fenceValue)
    {
        ALIMER_ASSERT(d3dFence != nullptr);
        ThrowIfFailed(queue->Signal(d3dFence, fenceValue));
    }

    void D3D12Fence::Wait(uint64_t fenceValue)
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
        d3dFence->Signal(fenceValue);
    }

}
