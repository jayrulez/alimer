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

#include "Graphics/SwapChain.h"
#include "D3D11Backend.h"
#include "Core/Ptr.h"

namespace Alimer
{
    class D3D11Texture;

    class D3D11SwapChain final : public SwapChain
    {
        friend class D3D11GraphicsDevice;

    public:
        D3D11SwapChain(D3D11GraphicsDevice* device);
        ~D3D11SwapChain() override;
        void Destroy();


        bool CreateOrResize() override;
        Texture* GetCurrentTexture() const override;

        void AfterReset();

    private:
        static constexpr uint32 kBufferCount = 2u;

        D3D11GraphicsDevice* device;
        uint32_t syncInterval = 1;
        uint32_t presentFlags = 0;

#if ALIMER_PLATFORM_WINDOWS
        HWND windowHandle = nullptr;
#else
        IUnknown* windowHandle = nullptr;
#endif

        IDXGISwapChain1* handle = nullptr;

        DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_IDENTITY;
        RefPtr<D3D11Texture> colorTexture;
    };
}
