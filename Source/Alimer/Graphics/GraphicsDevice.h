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

#include "Graphics/GPUAdapter.h"
#include "Graphics/CommandBuffer.h"
#include "Graphics/GPUBuffer.h"
#include "Graphics/Texture.h"

namespace Alimer
{
    struct GraphicsDeviceSettings
    {
        /// Whether to try use sRGB backbuffer color format.
        bool colorFormatSrgb = false;
        /// The depth format.
        PixelFormat depthStencilFormat = PixelFormat::Depth32Float;
        /// Should the window wait for vertical sync before swapping buffers.
        bool verticalSync = false;
        uint32 sampleCount = 1u;
    };

    class Window;

    /// Defines the graphics subsystem.
    class ALIMER_API GraphicsDevice : public Object
    {
        ALIMER_OBJECT(GraphicsDevice, Object);

    public:
        /// Destructor.
        virtual ~GraphicsDevice() = default;

        static GraphicsDevice* Create(Window* window, const GraphicsDeviceSettings& settings);

        bool BeginFrame();
        void EndFrame();

        /// Gets the adapter device.
        virtual GPUAdapter* GetAdapter() const = 0;

        /// Gets the device backend type.
        ALIMER_FORCE_INLINE GPUBackendType GetBackendType() const
        {
            return backendType;
        }

        /// Get the device features.
        ALIMER_FORCE_INLINE const GPUFeatures& GetFeatures() const
        {
            return features;
        }

        /// Get the device limits.
        ALIMER_FORCE_INLINE const GPULimits& GetLimits() const
        {
            return limits;
        }

        virtual Texture* GetBackbufferTexture() const = 0;

        CommandBuffer& RequestCommandBuffer(const char* name = nullptr, bool profile = false);

        /* Resource creation methods. */
        GPUBuffer* CreateBuffer(const GPUBufferDescriptor& descriptor, const void* initialData = nullptr);

        /// Total number of CPU frames completed.
        uint64_t GetFrameCount() const { return frameCount; }

    protected:
        GraphicsDevice(Window* window, GPUBackendType backendType);

        virtual CommandBuffer* RequestCommandBufferCore(const char* name, bool profile) = 0;
        //virtual GPUBuffer* CreateBufferCore(const GPUBufferDescriptor& descriptor, const void* initialData) = 0;

        Window* window;
        GPUBackendType backendType;
        GPUFeatures features{};
        GPULimits limits{};

    private:
        static GPUBackendType s_backendType;

        virtual bool BeginFrameImpl() = 0;
        virtual void EndFrameImpl() = 0;

        /// Whether a frame is active or not
        bool frameActive{ false };

        /// Number of frame count
        uint64_t frameCount{ 0 };

    private:
        ALIMER_DISABLE_COPY_MOVE(GraphicsDevice);
    };
}

