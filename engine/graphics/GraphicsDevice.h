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

#include "core/Ptr.h"
#include "graphics/GraphicsResource.h"
#include "graphics/CommandContext.h"
#include "os/os.h"
#include <memory>
#include <set>
#include <mutex>

namespace alimer
{
    class CommandQueue;

    /// Defines the logical graphics device class.
    class ALIMER_API GraphicsDevice : public RefCounted
    {
    public:
        /// Get set of available graphics backends.
        static std::set<BackendType> GetAvailableBackends();

        /// Create new GraphicsDevice instance.
        static RefPtr<GraphicsDevice> Create(window_t* window, const GraphicsDeviceInfo& info);

        /// Destructor.
        virtual ~GraphicsDevice() = default;

        /// Waits for the device to become idle.
        virtual void WaitForIdle() = 0;

        /// Begin rendering frame.
        virtual void BeginFrame() = 0;
        virtual void PresentFrame() = 0;

        /// Add a GPU resource to keep track of. Called by GPUResource.
        void AddGPUResource(GraphicsResource* resource);
        /// Remove a tracked GPU resource. Called by GPUResource.
        void RemoveGPUResource(GraphicsResource* resource);

        /// Get the device capabilities.
        inline const GraphicsDeviceCaps& GetCaps() const { return caps; }

    protected:
        virtual void ReleaseTrackedResources();

    protected:
        GraphicsDevice() = default;
        virtual bool Init(window_t* window, const GraphicsDeviceInfo& info) = 0;

        GraphicsDeviceCaps caps;

    private:
        /// Tracked gpu resource.
        std::mutex _gpuResourceMutex;
        std::vector<GraphicsResource*> _gpuResources;

    private:
        ALIMER_DISABLE_COPY_MOVE(GraphicsDevice);
    };
}
