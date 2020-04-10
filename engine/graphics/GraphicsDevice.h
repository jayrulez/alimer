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
    class GraphicsImpl;

    /* TODO: Expose resource creation context */

    /// Defines the logical graphics device class.
    class ALIMER_API GraphicsDevice final 
    {
    public:
        struct Desc
        {
            BackendType preferredBackend = BackendType::Count;
            std::string applicationName;
            GraphicsProviderFlags flags = GraphicsProviderFlags::None;
            GPUPowerPreference powerPreference = GPUPowerPreference::DontCare;
        };

        /// Get set of available graphics backends.
        static std::set<BackendType> GetAvailableBackends();

        /// Create new GraphicsDevice with given preferred backend, fallback to supported one.
        GraphicsDevice(const Desc& desc);

        /// Destructor.
        ~GraphicsDevice();

        /// Waits for the device to become idle.
        void WaitForIdle();

        /// Commits current frame and advance to next one.
        uint64_t PresentFrame(Swapchain* swapchain);
        uint64_t PresentFrame(const std::vector<Swapchain*>& swapchains);

        /// Get the default main graphics context.
        GraphicsContext* GetGraphicsContext() const { return graphicsContext.get(); }

        /// Get the features.
        const GraphicsDeviceCaps& GetCaps() const;

        /// Get the backend implementation.
        GraphicsImpl* GetImpl() const;

    private:
        bool Init();
        //virtual GraphicsContext* RequestContext(bool compute) = 0;

    private:
        GraphicsImpl* impl;
        std::shared_ptr<GraphicsContext> graphicsContext;

    private:
        ALIMER_DISABLE_COPY_MOVE(GraphicsDevice);
    };
}
