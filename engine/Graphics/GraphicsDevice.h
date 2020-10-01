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

#include "Graphics/CommandQueue.h"
#include "Graphics/GraphicsBuffer.h"
#include "Graphics/SwapChain.h"
#include <memory>
#include <mutex>

namespace Alimer
{
    class GraphicsDeviceImpl;

    /// Defines the graphics subsystem.
    class ALIMER_API GraphicsDevice final
    {
        friend class GraphicsResource;
        friend class SwapChain;

    public:
        enum class DebugFlags : uint32_t
        {
            None = 0,
            DebugRuntime = 1 << 0,
            GPUBasedValidation = 1 << 2,
            RenderDoc = 1 << 3,
        };

        /// Constructor.
        GraphicsDevice(FeatureLevel minFeatureLevel = FeatureLevel::Level_11_0, DebugFlags debugFlags = DebugFlags::None);

        /// Destructor.
        ~GraphicsDevice();

        /// Wait for GPU to finish pending operation and become idle.
        void WaitForGPU();

        void BeginFrame();
        void EndFrame();

        CommandQueue* GetCommandQueue(CommandQueueType type = CommandQueueType::Graphics) const;

        /// Gets the device backend type.
        //GPUBackendType GetBackendType() const { return caps.backendType; }

        /// Get the device caps.
        //const GraphicsDeviceCaps& GetCaps() const { return caps; }

        /// Get the device feature level.
        FeatureLevel GetFeatureLevel() const;

        /// Return graphics implementation, which holds the actual API-specific resources.
        GraphicsDeviceImpl* GetImpl() const { return impl; }

        /// Get the API handle.
        DeviceHandle GetHandle() const;

    private:
        /// Add a GPU object to keep track of. Called by GraphicsResource.
        void AddGraphicsResource(GraphicsResource* resource);
        /// Remove a GPU object. Called by GraphicsResource.
        void RemoveGraphicsResource(GraphicsResource* resource);
        /// Set device in lost state (called by SwapChain).
        void SetDeviceLost();

        /// Implementation.
        GraphicsDeviceImpl* impl;

        std::unique_ptr<CommandQueue> graphicsQueue;
        std::unique_ptr<CommandQueue> computeQueue;
        std::unique_ptr<CommandQueue> copyQueue;

        /// Mutex for accessing the GPU objects vector from several threads.
        std::mutex gpuObjectMutex;

        /// GPU objects.
        std::vector<GraphicsResource*> gpuObjects;
    };


    ALIMER_DEFINE_ENUM_FLAG_OPERATORS(GraphicsDevice::DebugFlags, uint32_t);
}

