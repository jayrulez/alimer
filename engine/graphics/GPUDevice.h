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

#include "graphics/Types.h"
#include "graphics/CommandContext.h"
#include <EASTL/shared_ptr.h>

namespace alimer
{
    class Texture;
    class SwapChain;
    class GraphicsBuffer;
 
    struct DeviceDesc
    {
        /// Application name.
        const char* application_name;
        /// GPU device power preference.
        DevicePowerPreference power_preference;
        /// Enable validation (debug layer).
        bool validation;
        /// Enable headless mode.
        bool headless;
    };

    /// Defines the GPU device class.
    class ALIMER_API GPUDevice
    {
    public:
        /// Constructor.
        GPUDevice() = default;

        /// Destructor.
        virtual ~GPUDevice() = default;

        /// Create new Device with given preferred backend, fallback to supported one.
        static GPUDevice* Create(GPUBackend preferredBackend = GPUBackend::Count);

        /// Init device with description
        bool Init(const DeviceDesc& desc);

        /// Called by validation layer.
        void NotifyValidationError(const char* message);

        virtual void WaitIdle() = 0;
        //bool begin_frame();
        //void end_frame();

        virtual eastl::shared_ptr<SwapChain> CreateSwapChain(void* nativeWindow, const SwapChainDescriptor& desc) = 0;

        /// Query device features.
        inline const GPUDeviceInfo& QueryInfo() const { return info; }

        /// Query device features.
        inline const GPUDeviceFeatures& QueryFeatures() const { return features; }

        /// Query device limits.
        inline const GPUDeviceLimits& QueryLimits() const { return limits; }

    private:
        virtual bool BackendInit(const DeviceDesc& desc) = 0;
        virtual void BackendShutdown() = 0;

    private:
        GPUDeviceInfo info;
        GPUDeviceFeatures features{};
        GPUDeviceLimits limits{};

        ALIMER_DISABLE_COPY_MOVE(GPUDevice);
    };
}
