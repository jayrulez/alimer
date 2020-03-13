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

#pragma once

#include "graphics/SwapChain.h"
#include "D3DCommon.h"
#include <vector>

namespace alimer
{
    class D3DSwapChain : public SwapChain
    {
    public:
        /// Constructor.
        D3DSwapChain(IDXGIFactory2* factory, IUnknown* deviceOrCommandQueue, void* nativeWindow, const SwapChainDescriptor& desc);

        // Destructor
        ~D3DSwapChain() override;

    private:
        SwapChainResizeResult Resize(uint32_t newWidth, uint32_t newHeight) override;
        void Present() override;

        IDXGIFactory2* factory;
        IUnknown* deviceOrCommandQueue;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        HWND window;
#else
        IUnknown* window;
#endif
        DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
        UINT backBufferCount = 2;
        bool flipPresentSupported = true;
        bool tearingSupported = false;
        IDXGISwapChain1* _handle = nullptr;
    };
}
