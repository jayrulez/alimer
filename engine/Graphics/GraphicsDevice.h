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
#include "Graphics/CommandBuffer.h"
#include "Graphics/GPUBuffer.h"
#include "Graphics/SwapChain.h"
#include <memory>
#include <mutex>

namespace Alimer
{
    struct DeviceApiData;

    /// Defines the graphics subsystem.
    class ALIMER_API GraphicsDevice final
    {
    public:
        using SharedPtr = std::shared_ptr<GraphicsDevice>;

        /// Constructor.
        GraphicsDevice(GraphicsDebugFlags flags = GraphicsDebugFlags::None, PhysicalDevicePreference adapterPreference = PhysicalDevicePreference::HighPerformance);

        /// Destructor.
        ~GraphicsDevice();

        /// Wait for GPU to finish pending operation and become idle.
        void WaitForGPU();

        /// End current rendering frame and present swap chain on screen.
        void Frame();

        /// Get the native API handle.
        DeviceHandle GetApiHandle() const;

        /// Get the native physical device/adapter.
        PhysicalDevice GetPhysicalDevice() const;

        /// Gets the device backend type.
        GPUBackendType GetBackendType() const { return caps.backendType; }

        /// Get the device caps.
        const GraphicsDeviceCaps& GetCaps() const;

        inline uint64_t GetFrameCount() const { return frameCount; }

        /// Add a GPU object to keep track of. Called by GraphicsResource.
        void AddGraphicsResource(GraphicsResource* resource);
        /// Remove a GPU object. Called by GraphicsResource.
        void RemoveGraphicsResource(GraphicsResource* resource);

#ifdef _DEBUG
        static void ReportLeaks();
#endif

    private:
        void Shutdown();
        void InitCapabilities();

        GraphicsDeviceCaps caps{};

        uint64_t frameCount{ 0 };

    private:
        DeviceApiData* apiData = nullptr;

        /// Mutex for accessing the GPU objects vector from several threads.
        std::mutex gpuObjectMutex;

        /// GPU objects.
        std::vector<GraphicsResource*> gpuObjects;

        ALIMER_DISABLE_COPY_MOVE(GraphicsDevice);
    };
}

