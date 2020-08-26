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

#include "D3D11Backend.h"

namespace Alimer
{
    class Window;

    class ALIMER_API D3D11SwapChain final
    {
    public:
        /// Constructor.
        D3D11SwapChain(D3D11GraphicsDevice* device, Window* window, bool verticalSync);
        /// Destructor
        ~D3D11SwapChain();

        HRESULT Present();

    private:
        void AfterReset();

        D3D11GraphicsDevice* device;

        uint32_t backBufferCount = 2u;
        uint32_t syncInterval;
        uint32_t presentFlags;


#if ALIMER_PLATFORM_WINDOWS
        Microsoft::WRL::ComPtr<IDXGISwapChain1> handle;
#else
        Microsoft::WRL::ComPtr<IDXGISwapChain3> handle;
#endif

        DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_IDENTITY;
    };
}
