//
// Copyright (c) 2019-2020 Amer Koleci and contributors.
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

#pragma once

#include "core/Utils.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "graphics/Types.h"
#include "../d3d/D3DCommon.h"

#include <d3d12.h>

// To use graphics and CPU markup events with the latest version of PIX, change this to include <pix3.h>
// then add the NuGet package WinPixEventRuntime to the project.
#include <pix.h>

namespace alimer
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) 
    // D3D12 functions.
    extern PFN_D3D12_CREATE_DEVICE D3D12CreateDevice;
    extern PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface;
    extern PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignature;
    extern PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER D3D12CreateRootSignatureDeserializer;
    extern PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE D3D12SerializeVersionedRootSignature;
    extern PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER D3D12CreateVersionedRootSignatureDeserializer;
#endif

    class D3D12GPUDevice;

    static inline D3D12_COMMAND_LIST_TYPE GetD3D12CommandListType(CommandQueueType queueType)
    {
        switch (queueType)
        {
        case CommandQueueType::Compute:
            return D3D12_COMMAND_LIST_TYPE_COMPUTE;
        case CommandQueueType::Copy:
            return D3D12_COMMAND_LIST_TYPE_COPY;
        default:
            return D3D12_COMMAND_LIST_TYPE_DIRECT;
        }
    }
}
