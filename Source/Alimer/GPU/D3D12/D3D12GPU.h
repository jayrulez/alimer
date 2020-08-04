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

#include "GPU/GPU.h"
#include "D3D12Backend.h"

namespace alimer
{
    class D3D12GPU;

    class D3D12GPUAdapter : public GPUAdapter
    {
    public:
        D3D12GPUAdapter(D3D12GPU* gpu, ComPtr<IDXGIAdapter1> adapter);
        bool Initialize();

    private:
        ComPtr<IDXGIAdapter1> adapter;
        ComPtr<ID3D12Device> d3dDevice;
    };

    class D3D12GPU final
    {
    public:
        static bool IsAvailable();
        static D3D12GPU* Get();
        eastl::unique_ptr<GPUAdapter> RequestAdapter(PowerPreference powerPreference);

        IDXGIFactory4* GetFactory() const;

    private:
        D3D12GPU();
        ~D3D12GPU();

        DWORD dxgiFactoryFlags = 0;
        ComPtr<IDXGIFactory4> factory;
        DXGIFactoryCaps dxgiFactoryCaps = DXGIFactoryCaps::FlipPresent | DXGIFactoryCaps::HDR;
        D3D_FEATURE_LEVEL minFeatureLevel{ D3D_FEATURE_LEVEL_11_0 };
    };
}
