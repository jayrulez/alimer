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

#include "D3D12CommandQueue.h"
#include "D3D12GraphicsImpl.h"

namespace alimer
{
    D3D12CommandQueue::D3D12CommandQueue(D3D12GraphicsImpl* device_, D3D12_COMMAND_LIST_TYPE type_)
        : device(device_)
        , type(type_)
        , nextFenceValue((uint64_t)type_ << 56 | 1)
        , lastCompletedFenceValue((uint64_t)type_ << 56)
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = type;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.NodeMask = 1;
        ThrowIfFailed(device->GetD3DDevice()->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue)));

        // Create fence and event
        fence.Init(device->GetD3DDevice(), 0);
        fence.Clear((uint64_t)type << 56);

        switch (type_)
        {
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
            commandQueue->SetName(L"Direct Command Queue");
            fence.GetD3DFence()->SetName(L"Direct Command Queue Fence");
            break;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            commandQueue->SetName(L"Compute Command Queue");
            fence.GetD3DFence()->SetName(L"Compute Command Queue Fence");
            break;
        case D3D12_COMMAND_LIST_TYPE_COPY:
            commandQueue->SetName(L"Copy Command Queue");
            fence.GetD3DFence()->SetName(L"Copy Command Queue Fence");
            break;
        }

    }

    D3D12CommandQueue::~D3D12CommandQueue()
    {
        fence.Shutdown();
        SafeRelease(commandQueue);
    }
}
