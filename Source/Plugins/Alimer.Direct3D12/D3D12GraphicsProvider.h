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

#include "graphics/GraphicsProvider.h"
#include "D3D12Backend.h"

namespace Alimer
{
    class D3D12GraphicsProvider final : public GraphicsProvider
    {
    public:
        static bool IsAvailable();

        D3D12GraphicsProvider(bool validation_);
        ~D3D12GraphicsProvider() override;

        Array<std::shared_ptr<GraphicsAdapter>> EnumerateGraphicsAdapters() override;
        std::shared_ptr<GraphicsDevice> CreateDevice(const std::shared_ptr<GraphicsAdapter>& adapter) override;

        IDXGIFactory4* GetDXGIFactory() const { return dxgiFactory; }
        bool IsTearingSupported() const { return isTearingSupported; }
        bool IsValidationEnabled() const { return validation; }
        D3D_FEATURE_LEVEL GetMinFeatureLevel() const { return minFeatureLevel; }

    private:
        bool validation;

        UINT dxgiFactoryFlags = 0;
        IDXGIFactory4* dxgiFactory = nullptr;
        bool isTearingSupported = false;
        D3D_FEATURE_LEVEL minFeatureLevel = D3D_FEATURE_LEVEL_11_0;
    };

    class D3D12GraphicsProviderFactory final : public GraphicsProviderFactory
    {
    public:
        BackendType GetBackendType() const override { return BackendType::Direct3D12; }
        std::unique_ptr<GraphicsProvider> CreateProvider(bool validation) override;
    };
}
