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

#include "Graphics/Types.h"
#include "Graphics/CommandQueue.h"
#include "Graphics/GPUBuffer.h"
#include "Graphics/SwapChain.h"
#include <EASTL/set.h>
#include <memory>
#include <mutex>

namespace Alimer
{
    /// Defines the graphics subsystem.
    class ALIMER_API GraphicsDevice : public Object
    {
        ALIMER_OBJECT(GraphicsDevice, Object);

    public:
        virtual ~GraphicsDevice() = default;

        static eastl::set<GPUBackendType> GetAvailableBackends();
        static RefPtr<GraphicsDevice> Create(GPUBackendType preferredBackend, const GraphicsDeviceDescription& desc);

        /// Wait for GPU to finish pending operation and become idle.
        virtual void WaitForGPU() = 0;

        /// Begin rendering frame.
        virtual bool BeginFrame() = 0;

        /// End current rendering frame.
        virtual void EndFrame() = 0;

        /// Gets the device backend type.
        GPUBackendType GetBackendType() const { return caps.backendType; }

        /// Get the device caps.
        const GraphicsDeviceCaps& GetCaps() const;

        /// Get the main/primary SwapChain.
        SwapChain* GetPrimarySwapChain() const;

        /// Add a GPU object to keep track of. Called by GraphicsResource.
        void AddGraphicsResource(GraphicsResource* resource);
        /// Remove a GPU object. Called by GraphicsResource.
        void RemoveGraphicsResource(GraphicsResource* resource);

        /**
        * Get a command queue. Valid types are:
        * - Graphics    : Can be used for draw, dispatch, or copy commands.
        * - Compute     : Can be used for dispatch or copy commands.
        * - Copy        : Can be used for copy commands.
        */
        CommandQueue* GetCommandQueue(CommandQueueType type = CommandQueueType::Graphics) const;

        virtual CommandBuffer* GetCommandBuffer() = 0;

        uint64_t GetFrameCount() const { return frameCount;  }

    protected:
        GraphicsDevice();

        GraphicsDeviceCaps caps{};
        RefPtr<SwapChain> primarySwapChain;
        std::shared_ptr<CommandQueue> graphicsCommandQueue;
        std::shared_ptr<CommandQueue> computeCommandQueue;
        std::shared_ptr<CommandQueue> copyCommandQueue;

        uint64_t frameCount{ 0 };

    private:
        /// Mutex for accessing the GPU objects vector from several threads.
        std::mutex gpuObjectMutex;

        /// GPU objects.
        eastl::vector<GraphicsResource*> gpuObjects;

        ALIMER_DISABLE_COPY_MOVE(GraphicsDevice);
    };
}

