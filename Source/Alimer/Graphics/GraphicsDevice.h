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

#include "Core/Ptr.h"
#include "graphics/GraphicsResource.h"
#include "os/os.h"
#include <memory>

namespace alimer
{
    class GraphicsPresenter;

    /// Defines the logical graphics device class.
    class ALIMER_API GraphicsDevice : public std::enable_shared_from_this<GraphicsDevice>
    {
    public:
        
        virtual ~GraphicsDevice() = default;

        static std::unique_ptr<GraphicsDevice> Create(FeatureLevel minFeatureLevel = FeatureLevel::Level11_0, bool enableDebugLayer = false);

        virtual RefPtr<GraphicsPresenter> CreateSwapChainGraphicsPresenter(void* windowHandle, const PresentationParameters& presentationParameters) = 0;

        //virtual RefPtr<Texture> CreateTexture(const TextureDesc* pDesc, const void* initialData) = 0;

        const GraphicsDeviceCaps& GetCaps() const;

    protected:
        GraphicsDevice();

        GraphicsDeviceCaps caps{};

    private:
        ALIMER_DISABLE_COPY_MOVE(GraphicsDevice);
    };
}
