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
#include "Math/Color.h"
#include <mutex>

namespace alimer
{
#if !defined(NDEBUG) || defined(DEBUG) || defined(_DEBUG)
#   define DEFAULT_ENABLE_DEBUG_LAYER true
#else
#   define DEFAULT_ENABLE_DEBUG_LAYER false
#endif

    class Texture;
    using CommandList = uint8_t;

    class ALIMER_API GraphicsDeviceEvents
    {
    public:
        virtual void OnDeviceLost() = 0;
        virtual void OnDeviceRestored() = 0;

    protected:
        ~GraphicsDeviceEvents() = default;
    };

    class GraphicsImpl;

    /// Defines the logical graphics device class.
    class ALIMER_API GraphicsDevice final
    {
        friend class GraphicsResource;

    public:
        GraphicsDevice(bool enableValidationLayer = DEFAULT_ENABLE_DEBUG_LAYER, PowerPreference powerPreference = PowerPreference::Default);
        ~GraphicsDevice();

        /// Return graphics implementation, which holds the actual API-specific resources.
        GraphicsImpl* GetImpl() const { return impl; }

        void BeginFrame();
        void EndFrame();

        const GraphicsDeviceCaps& GetCaps() const { return caps; }

    private:
        void Shutdown();

        void TrackResource(GraphicsResource* resource);
        void UntrackResource(GraphicsResource* resource);

    protected:
        
        void ReleaseTrackedResources();

        GraphicsImpl* impl;
        GraphicsDeviceCaps caps{};
        std::mutex trackedResourcesMutex;
        Vector<GraphicsResource*> trackedResources;
        GraphicsDeviceEvents* events = nullptr;
        RefPtr<SwapChain> mainSwapChain;

    private:
        ALIMER_DISABLE_COPY_MOVE(GraphicsDevice);
    };
}
