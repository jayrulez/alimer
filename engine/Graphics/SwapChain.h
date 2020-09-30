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

#include "Graphics/BackendTypes.h"
#include "Graphics/Texture.h"
#include <vector>

namespace Alimer
{
    class GraphicsDevice;

    struct PresentationParameters
    {
        uint32 backBufferWidth = 0;
        uint32 backBufferHeight = 0;
        PixelFormat backBufferFormat = PixelFormat::BGRA8UnormSrgb;
        PixelFormat depthStencilFormat = PixelFormat::Depth32Float;
        bool isFullscreen = false;
        bool verticalSync = true;
    };

    class ALIMER_API SwapChain final 
    {
    public:
        /// Constructor.
        SwapChain(GraphicsDevice* device, void* windowHandle, const PresentationParameters& presentationParameters);

        void Present();

        GraphicsDevice* GetDevice() const;
        Texture* GetCurrentTexture() const;

        /// Get the API handle.
        SwapChainHandle GetHandle() const;

    protected:
        static constexpr uint32 kBufferCount = 2u;

        GraphicsDevice* device;
        PresentationParameters presentationParameters;
        SwapChainHandle handle;

        uint32_t backBufferIndex = 0;
        std::vector<RefPtr<Texture>> colorTextures;
    };
}
