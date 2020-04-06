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

#include "graphics/Framebuffer.h"
#include "graphics/CommandContext.h"
#include <vector>
#include <set>
#include <memory>

namespace alimer
{
    class Window;
    class GPUBuffer;
    class Texture;
    class SwapChain;
    class GraphicsDevice;
    using GraphicsDevicePtr = std::shared_ptr<GraphicsDevice>;

    /* TODO: Expose GraphicsContext */
    /* TODO: Expose resource creation context */

    /// Defines the logical graphics device class.
    class ALIMER_API GraphicsDevice
    {
    public:
        /// Device descriptor.
        struct Desc
        {
            bool colorSrgb = true;
            uint32_t sampleCount = 1;
        };

        /// Destructor.
        virtual ~GraphicsDevice() = default;

        /// Get set of available GPU backend supported implementation.
        static std::set<GPUBackend> GetAvailableBackends();

        /// Create new GPUDevice with given preferred backend, fallback to supported one.
        static GraphicsDevicePtr Create(Window* window, const Desc& desc);

        void Shutdown();

        /// Called by validation layer.
        void NotifyValidationError(const char* message);

        virtual void Commit() {}

        /// Get the backend type.
        GPUBackend GetBackendType() const { return info.backend; }

        /// Get device features.
        inline const GPUDeviceInfo& GetInfo() const { return info; }

        /// Query device features.
        inline const GPUDeviceFeatures& GetFeatures() const { return features; }

        /// Query device limits.
        inline const GPUDeviceLimits& GetLimits() const { return limits; }

    private:
        bool Initialize();
        virtual bool BackendInit() = 0;
        virtual void BackendShutdown() = 0;

    protected:
        /// Constructor.
        GraphicsDevice(Window* window_, const Desc& desc_);

        Window* window;
        Desc desc;
        GPUDeviceInfo info;
        GPUDeviceFeatures features{};
        GPUDeviceLimits limits{};
       
    private:
        ALIMER_DISABLE_COPY_MOVE(GraphicsDevice);
    };
}
