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

#include "Graphics/CommandContext.h"
#include "Graphics/SwapChain.h"
#include "Graphics/Buffer.h"
#include <vector>
#include <mutex>
#include <set>

namespace alimer
{
    class ALIMER_API GraphicsDeviceEvents
    {
    public:
        virtual void OnDeviceLost() = 0;
        virtual void OnDeviceRestored() = 0;

    protected:
        ~GraphicsDeviceEvents() = default;
    };

    class Window;

    /// Defines the logical graphics subsystem.
    class ALIMER_API GraphicsDevice : public Object
    {
        friend class GraphicsResource;

        ALIMER_OBJECT(GraphicsDevice, Object);

    public:
        GraphicsDevice() = default;
        virtual ~GraphicsDevice() = default;

        static RendererType GetDefaultRenderer(RendererType preferredBackend);
        static std::set<RendererType> GetAvailableRenderDrivers();

        static SharedPtr<GraphicsDevice> CreateSystemDefault(RendererType preferredBackend = RendererType::Count, GPUFlags flags = GPUFlags::None);

        virtual bool BeginFrame() = 0;
        virtual void EndFrame() = 0;

        /// Get the device capabilities.
        const GraphicsDeviceCapabilities& GetCaps() const { return caps; }

        virtual SharedPtr<SwapChain> CreateSwapChain(const SwapChainDescriptor& descriptor) = 0;

        void TrackResource(GraphicsResource* resource);
        void UntrackResource(GraphicsResource* resource);

    protected:
        void ReleaseTrackedResources();

        GraphicsDeviceCapabilities caps{};

        std::mutex trackedResourcesMutex;
        std::vector<GraphicsResource*> trackedResources;
        GraphicsDeviceEvents* events = nullptr;

    private:
        ALIMER_DISABLE_COPY_MOVE(GraphicsDevice);
    };
}

