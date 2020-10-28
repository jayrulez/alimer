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
#include "D3D12GraphicsDevice.h"

namespace Alimer
{
    D3D12CommandQueue::D3D12CommandQueue(D3D12GraphicsDevice& device, D3D12_COMMAND_LIST_TYPE type)
        : device{ device }
        , type{ type }
        , nextFenceValue((uint64_t)type << 56 | 1)
        , lastCompletedFenceValue((uint64_t)type << 56)
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = type;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 0;

        ThrowIfFailed(device.GetD3DDevice()->CreateCommandQueue(&desc, IID_PPV_ARGS(&handle)));
        ThrowIfFailed(device.GetD3DDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
        fence->Signal(lastCompletedFenceValue);

        switch (type)
        {
        case D3D12_COMMAND_LIST_TYPE_COPY:
            handle->SetName(L"Copy Command Queue");
            fence->SetName(L"Copy Command Queue Fence");
            break;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            handle->SetName(L"Compute Command Queue");
            fence->SetName(L"Compute Command Queue Fence");
            break;
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
            handle->SetName(L"Direct Command Queue");
            fence->SetName(L"Direct Command Queue Fence");
            break;
        }
    }

    D3D12CommandQueue::~D3D12CommandQueue()
    {
    }
}
