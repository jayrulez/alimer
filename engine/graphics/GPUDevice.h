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
    class Texture;
    class GraphicsBuffer;

    /// Defines the GPU device class.
    class ALIMER_API GPUDevice
    {
    public:
        /// Constructor.
        GPUDevice() = default;

        /// Destructor.
        virtual ~GPUDevice() = default;

        /// Get set of available GPU backend supported implementation.
        static std::set<GPUBackend> getAvailableBackends();

        /// Create new GPUDevice with given preferred backend, fallback to supported one.
        static std::unique_ptr<GPUDevice> create(GPUBackend preferredBackend = GPUBackend::Count, bool validation = false, bool headless = false);

        /// Called by validation layer.
        void notifyValidationError(const char* message);

        virtual void waitIdle() = 0;
        virtual bool begin_frame() { return true; }
        virtual void end_frame() {}

        std::shared_ptr<Framebuffer> createFramebuffer(const SwapChainDescriptor* descriptor);

        /// Query device features.
        inline const GPUDeviceInfo& QueryInfo() const { return info; }

        /// Query device features.
        inline const GPUDeviceFeatures& QueryFeatures() const { return features; }

        /// Query device limits.
        inline const GPUDeviceLimits& QueryLimits() const { return limits; }

    private:
        virtual void backendShutdown() = 0;
        virtual std::shared_ptr<Framebuffer> createFramebufferCore(const SwapChainDescriptor* descriptor) = 0;

    protected:
        GPUDeviceInfo info;
        GPUDeviceFeatures features{};
        GPUDeviceLimits limits{};
       
    private:
        ALIMER_DISABLE_COPY_MOVE(GPUDevice);
    };
}
