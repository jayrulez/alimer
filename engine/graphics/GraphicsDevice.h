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

#include "graphics/GraphicsAdapter.h"
#include "graphics/CommandContext.h"
#include <vector>
#include <set>
#include <memory>

namespace alimer
{
    class GPUBuffer;
    class Texture;

    /* TODO: Expose GraphicsContext */
    /* TODO: Expose resource creation context */

    /// Defines the logical graphics device class.
    class ALIMER_API GraphicsDevice : public RefCounted
    {
    public:
        struct Desc
        {
            BackendType preferredBackend = BackendType::Count;
            std::string applicationName;
            GraphicsProviderFlags flags = GraphicsProviderFlags::None;
        };

        /// Get set of available graphics providers.
        static std::set<BackendType> GetAvailableBackends();

        /// Create new GraphicsDevice with given preferred backend, fallback to supported one.
        static SharedPtr<GraphicsDevice> Create(GraphicsSurface* surface, const Desc& desc);

        /// Destructor.
        virtual ~GraphicsDevice() = default;

        /// Waits for the device to become idle.
        virtual void WaitForIdle() = 0;

        /// Begin frame rendering logic.
        virtual bool BeginFrame() = 0;

        /// End current frame and present it on scree.
        virtual void PresentFrame() = 0;

        /// Get the main context created with the device.
        virtual GraphicsContext* GetMainContext() const = 0;

        /// Query device features.
        inline const GraphicsDeviceInfo& GetInfo() const { return info; }

        /// Query device features.
        inline const GPUDeviceFeatures& GetFeatures() const { return features; }

        /// Query device limits.
        inline const GPUDeviceLimits& GetLimits() const { return limits; }

    protected:
        /// Constructor.
        GraphicsDevice(GraphicsSurface* surface_, const Desc& desc_);

        GraphicsSurface* surface;
        Desc desc;
        GraphicsDeviceInfo info{};
        GPUDeviceFeatures features{};
        GPUDeviceLimits limits{};

    private:
        ALIMER_DISABLE_COPY_MOVE(GraphicsDevice);
    };
}
