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
#include "Graphics/GPUContext.h"
#include "Graphics/GPUBuffer.h"
#include "Graphics/GPUSwapChain.h"

namespace alimer
{
    class GraphicsImpl;

    /// Defines the graphics subsystem.
    class ALIMER_API GraphicsDevice : public Object
    {
        friend class GraphicsResource;

        ALIMER_OBJECT(GraphicsDevice, Object);

    public:
        /// Destructor.
        virtual ~GraphicsDevice() = default;

        static void EnableGPUBasedBackendValidation(bool value);
        static bool IsGPUBasedBackendValidationEnabled();

        static GraphicsDevice* Create(const String& appName, const GPUDeviceDescriptor& descriptor, GPUBackendType preferredRendererType = GPUBackendType::Count);

        /// Gets the adapter device.
        virtual GPUAdapter* GetAdapter() const = 0;

        /// Gets the main GPU context.
        virtual GPUContext* GetMainContext() const = 0;

        /// Gets the main swap chain created with the device.
        virtual GPUSwapChain* GetMainSwapChain() const = 0;

        /// Gets the device backend type.
        ALIMER_FORCE_INLINE GPUBackendType GetBackendType() const
        {
            return backendType;
        }

        /// Get the device capabilities.
        ALIMER_FORCE_INLINE const GraphicsCapabilities& GetCaps() const
        {
            return caps;
        }

        bool BeginFrame();
        void EndFrame();
        virtual void Present(GPUSwapChain* swapChain, bool verticalSync) = 0;

        /* Resource creation methods. */
        GPUSwapChain* CreateSwapChain(const GPUSwapChainDescriptor& descriptor);
        GPUBuffer* CreateBuffer(const GPUBufferDescriptor& descriptor);

        /* Commands */
        void PushDebugGroup(const String& name, CommandList commandList = 0);
        void PopDebugGroup(CommandList commandList = 0);
        void InsertDebugMarker(const String& name, CommandList commandList = 0);

        void BeginRenderPass(uint32_t numColorAttachments, const RenderPassColorAttachment* colorAttachments, const RenderPassDepthStencilAttachment* depthStencil = nullptr, CommandList commandList = 0);
        void EndRenderPass(CommandList commandList = 0);

        /// Total number of CPU frames completed.
        uint64_t GetFrameCount() const { return frameCount; }

    protected:
        GraphicsDevice(GPUBackendType backendType_);

        virtual bool BeginFrameImpl() = 0;
        virtual void EndFrameImpl() = 0;
        virtual GPUSwapChain* CreateSwapChainCore(const GPUSwapChainDescriptor& descriptor) = 0;

        GPUBackendType backendType;
        GraphicsCapabilities caps{};

    private:
        static bool enableGPUValidation;

        UInt2 resolution = UInt2::Zero;
        uint32_t sampleCount = 1;

        /// Current active frame index
        uint32_t activeFrameIndex{ 0 };

        /// Whether a frame is active or not
        bool frameActive{ false };

        /// Number of frame count
        uint64_t frameCount{ 0 };

    private:
        ALIMER_DISABLE_COPY_MOVE(GraphicsDevice);
    };
}

