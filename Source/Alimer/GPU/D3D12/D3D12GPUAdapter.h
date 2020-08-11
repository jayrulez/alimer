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

#include "GPU/GPUAdapter.h"
#include "D3D12Backend.h"

namespace alimer
{
    class D3D12GPUAdapter final : public GPUAdapter
    {
    public:
        D3D12GPUAdapter(const ComPtr<IDXGIAdapter1>& dxgiAdapter);
        ~D3D12GPUAdapter() override;

        bool Initialize();

        IDXGIAdapter3* Gethandle() const { return handle.Get(); }

    private:
        RefPtr<GPUDevice> CreateDevice(const GPUDeviceDescriptor* descriptor) override;

        ComPtr<IDXGIAdapter3> handle;
        ComPtr<ID3D12Device> d3dDevice;

        static constexpr D3D_FEATURE_LEVEL kMinFeatureLevel{ D3D_FEATURE_LEVEL_11_0 };
    };
}
