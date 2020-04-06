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
#include "core/Assert.h"
#include "core/String.h"

namespace alimer
{
    D3D12GraphicsAdapter::D3D12GraphicsAdapter(D3D12GraphicsProvider* provider_, IDXGIAdapter1* adapter_)
        : GraphicsAdapter(provider_)
        , adapter(adapter_)
    {
        DXGI_ADAPTER_DESC1 desc;
        ThrowIfFailed(adapter->GetDesc1(&desc));

        vendorId = desc.VendorId;
        deviceId = desc.DeviceId;

        std::wstring deviceName(desc.Description);
        name = alimer::ToUtf8(deviceName);
    }

    D3D12GraphicsAdapter::~D3D12GraphicsAdapter()
    {
        SafeRelease(adapter);
    }

    uint32_t D3D12GraphicsAdapter::GetVendorId() const
    {
        return vendorId;
    }

    uint32_t D3D12GraphicsAdapter::GetDeviceId() const
    {
        return deviceId;
    }

    GraphicsAdapterType D3D12GraphicsAdapter::GetType() const
    {
        return type;
    }

    const std::string& D3D12GraphicsAdapter::GetName() const
    {
        return name;
    }
}
