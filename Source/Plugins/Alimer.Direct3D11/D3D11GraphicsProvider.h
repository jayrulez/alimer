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
#include "D3D11Backend.h"

namespace Alimer
{
    class D3D11GraphicsProvider final : public GraphicsProvider
    {
    public:
        static bool IsAvailable();

        D3D11GraphicsProvider(bool validation_);
        ~D3D11GraphicsProvider() override;

        std::vector<std::shared_ptr<GraphicsAdapter>> EnumerateGraphicsAdapters() override;
        std::shared_ptr<GraphicsDevice> CreateDevice(const std::shared_ptr<GraphicsAdapter>& adapter) override;

        IDXGIFactory2* GetDXGIFactory() const { return dxgiFactory; }
        bool IsTearingSupported() const { return isTearingSupported; }
        bool IsValidationEnabled() const { return validation; }

    private:
        UINT dxgiFactoryFlags = 0;
        IDXGIFactory2* dxgiFactory = nullptr;
        bool isTearingSupported = false;
        bool validation;
    };

    class D3D11GraphicsProviderFactory final : public GraphicsProviderFactory
    {
    public:
        BackendType GetBackendType() const override { return BackendType::Direct3D11; }
        std::unique_ptr<GraphicsProvider> CreateProvider(bool validation) override;
    };
}
