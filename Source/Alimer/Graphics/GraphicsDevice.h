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

#include "graphics/GraphicsResource.h"
#include <memory>

namespace alimer
{
    struct DeviceApiData;
    class Texture;
    class GraphicsContext;

    /// Defines the logical graphics device class.
    class ALIMER_API GraphicsDevice
    {
    public:
        struct Desc
        {
            BackendType backendType = BackendType::Count;
            std::string applicationName;
            bool enableDebugLayer = false;
            bool headless = false;
        };

        virtual ~GraphicsDevice() = default;

        /// Create new context.
        virtual GraphicsContext* CreateContext(const GraphicsContextDescription& desc) = 0;
        virtual Texture* CreateTexture(const TextureDescription& desc, const void* initialData) = 0;

        const GraphicsDeviceCaps& GetCaps() const;

    protected:
        GraphicsDevice() = default;

        GraphicsDeviceCaps caps{};

    private:
        ALIMER_DISABLE_COPY_MOVE(GraphicsDevice);
    };
}
