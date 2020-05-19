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

#include "core/Ptr.h"
#include "graphics/GraphicsResource.h"
#include "os/os.h"
#include <memory>
#include <set>

namespace alimer
{
    struct TextureDesc;
    class Texture;

    /// Desribes a GraphicsDevice
    struct GraphicsDeviceDesc
    {
        const char* applicationName = "";
        GraphicsDeviceFlags flags = GraphicsDeviceFlags::None;
    };

    /// Defines the logical graphics device class.
    class ALIMER_API GraphicsDevice
    {
    public:
        GraphicsDevice() = default;
        virtual ~GraphicsDevice() = default;

        static std::set<GraphicsAPI> GetAvailableGraphicsAPI();

        /**
        * Create new device.
        */
        static std::unique_ptr<GraphicsDevice> Create(GraphicsAPI api, window_t* window, const GraphicsDeviceDesc& desc);

        virtual RefPtr<Texture> CreateTexture(const TextureDesc* pDesc, const void* initialData) = 0;

    private:
    };
}
