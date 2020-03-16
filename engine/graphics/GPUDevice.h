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
#include <memory>

namespace alimer
{
    class Texture;
    class GraphicsBuffer;

    enum class GPUDeviceFlags : uint32_t
    {
        None = 0,
        /// Enable vsync
        VSync = 0x01,
        /// Enable validation (debug layer).
        Validation = 0x02,
        /// Enable headless mode.
        Headless = 0x04,
    };
    ALIMER_DEFINE_ENUM_BITWISE_OPERATORS(GPUDeviceFlags);
 
    struct DeviceDesc
    {
        /// Application name.
        const char* application_name;

        /// GPU device power preference.
        DevicePowerPreference powerPreference;

        /// Device flags.
        GPUDeviceFlags flags = GPUDeviceFlags::VSync;

        /// The backbuffer width.
        uint32_t backbuffer_width;
        /// The backbuffer height.
        uint32_t backbuffer_height;

        /// Native display type.
        void* native_display;
        /// Native window handle.
        void* native_window_handle;
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
        static GPUDevice* Create(GPUBackend preferred_backend = GPUBackend::Count);

        /// Init device with description
        bool Init(const DeviceDesc& desc);

        /// Called by validation layer.
        void NotifyValidationError(const char* message);

        virtual void WaitIdle() = 0;
        virtual bool begin_frame() { return true; }
        virtual void end_frame() {}

        std::shared_ptr<Framebuffer> createFramebuffer(const SwapChainDescriptor* descriptor);

        /// Query device features.
        inline const GPUDeviceInfo& QueryInfo() const { return info; }

        /// Query device features.
        inline const GPUDeviceFeatures& QueryFeatures() const { return features; }

        /// Query device limits.
        inline const GPUDeviceLimits& QueryLimits() const { return limits; }

        bool IsVSyncEnabled() const { return vsync; }

    private:
        virtual bool BackendInit(const DeviceDesc& desc) = 0;
        virtual void BackendShutdown() = 0;
        virtual std::shared_ptr<Framebuffer> createFramebufferCore(const SwapChainDescriptor* descriptor) = 0;

    protected:
        GPUDeviceInfo info;
        GPUDeviceFeatures features{};
        GPUDeviceLimits limits{};
        bool vsync{ false };
       
    private:
        ALIMER_DISABLE_COPY_MOVE(GPUDevice);
    };
}
