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

#include "graphics/GraphicsSurface.h"
#include "graphics/SwapChain.h"
#include "graphics/CommandContext.h"
#include <memory>
#include <set>

namespace alimer
{
    class Swapchain;

    struct GraphicsDeviceDescriptor
    {
        BackendType preferredBackend = BackendType::Count;
        GPUVendorId preferredVendorId;

        /// Enable device for debuging.
        bool debug;

        /// Enable device for profiling.
        bool profile;

        PresentationParameters presentationParameters;
    };

    /// Defines the logical graphics device class.
    class ALIMER_API GraphicsDevice
    {
    public:
        /// Get set of available graphics backends.
        static std::set<BackendType> GetAvailableBackends();

        /// Create new GraphicsDevice with given preferred backend, fallback to supported one.
        GraphicsDevice(const GraphicsDeviceDescriptor& desc_);

        /// Destructor.
        virtual ~GraphicsDevice();

        /**
        * Gets the single instance of the graphics device.
        *
        * @return The single instance of the device.
        */
        static GraphicsDevice* GetInstance();

        static std::unique_ptr<GraphicsDevice> Create(const GraphicsDeviceDescriptor& desc);

        /// Waits for the device to become idle.
        virtual void WaitForIdle() = 0;

        /// Present the main swap chain on screen.
        void Present();
        //uint64_t PresentFrame(const std::vector<Swapchain*>& swapchains);

        /// Get the default main graphics context.
        GraphicsContext* GetGraphicsContext() const { return graphicsContext.get(); }

        /// Get the features.
        inline const GraphicsDeviceCaps& GetCaps() const { return caps; }

    private:
        virtual void Present(const std::vector<Swapchain*>& swapchains) = 0;

    protected:
        GraphicsDeviceDescriptor desc;
        GraphicsDeviceCaps caps;
        std::shared_ptr<GraphicsContext> graphicsContext;
        SharedPtr<Swapchain> mainSwapchain;

    private:
        static SharedPtr<GraphicsDevice> instance;

        ALIMER_DISABLE_COPY_MOVE(GraphicsDevice);
    };
}
