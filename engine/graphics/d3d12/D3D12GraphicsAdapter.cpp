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

#include "D3D12GraphicsAdapter.h"
#include "D3D12GraphicsProvider.h"
#include "D3D12GraphicsDevice.h"
#include "core/Assert.h"
#include "core/String.h"

namespace alimer
{
    D3D12GraphicsAdapter::D3D12GraphicsAdapter(D3D12GraphicsProvider* provider_, ComPtr<IDXGIAdapter1> adapter_)
        : GraphicsAdapter(provider_, BackendType::Direct3D12)
        , adapter(adapter_)
    {
    }

    SharedPtr<GraphicsDevice> D3D12GraphicsAdapter::CreateDevice(GraphicsSurface* surface)
    {
        return SharedPtr<GraphicsDevice>(new D3D12GraphicsDevice(this, surface));
    }

    bool D3D12GraphicsAdapter::Initialize()
    {
        // Create the DX12 API device object.
        ComPtr<ID3D12Device> d3dDevice;
        HRESULT hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(d3dDevice.GetAddressOf()));
        if (FAILED(hr))
        {
            return false;
        }

        DXGI_ADAPTER_DESC1 desc;
        ThrowIfFailed(adapter->GetDesc1(&desc));

        vendorId = desc.VendorId;
        deviceId = desc.DeviceId;

        std::wstring deviceName(desc.Description);
        name = alimer::ToUtf8(deviceName);

        D3D12_FEATURE_DATA_ARCHITECTURE arch = {};
        hr = d3dDevice->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &arch, sizeof(arch));
        if (FAILED(hr)) {
            return false;
        }
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            adapterType = GraphicsAdapterType::CPU;
        }
        else
        {
            adapterType = arch.UMA ? GraphicsAdapterType::IntegratedGPU : GraphicsAdapterType::DiscreteGPU;
        }

        // Determine maximum supported feature level for this device
        static const D3D_FEATURE_LEVEL s_featureLevels[] =
        {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
        };

        D3D12_FEATURE_DATA_FEATURE_LEVELS featLevels =
        {
            _countof(s_featureLevels), s_featureLevels, D3D_FEATURE_LEVEL_11_0
        };

        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        hr = d3dDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
        if (SUCCEEDED(hr))
        {
            featureLevel = featLevels.MaxSupportedFeatureLevel;
        }

        InitCapabilities(featureLevel);
        return true;
    }

    void D3D12GraphicsAdapter::InitCapabilities(D3D_FEATURE_LEVEL featureLevel)
    {
    }
}
