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

#include "graphics/GPUAdapter.h"
#include "graphics/SwapChain.h"
#include "graphics/CommandContext.h"
#include <memory>
#include <mutex>

namespace alimer
{
    class Swapchain;

    /// Defines the logical GPU device class.
    class ALIMER_API GPUDevice : public RefCounted
    {
    public:
        /// Constructor.
        GPUDevice(GPUAdapter* adapter);

        /// Destructor.
        virtual ~GPUDevice() = default;

        /// Waits for the device to become idle.
        virtual void WaitForIdle() = 0;

        /// Add a GPU resource to keep track of. Called by GPUResource.
        void AddGPUResource(GPUResource* resource);
        /// Remove a tracked GPU resource. Called by GPUResource.
        void RemoveGPUResource(GPUResource* resource);

        /// Present the main swap chain on screen.
        void Present();
        //uint64_t PresentFrame(const std::vector<Swapchain*>& swapchains);

        /// Get the default main graphics context.
        GraphicsContext* GetGraphicsContext() const { return graphicsContext.get(); }

        /// Get the features.
        inline GPUAdapter* GetAdapter() const { return _adapter.get(); }

        /// Get the features.
        inline const GPUFeatures& GetFeatures() const { return _features; }

        /// Get the limits.
        inline const GPULimits& GetLimits() const { return _limits; }

    protected:
        virtual void ReleaseTrackedResources();
        //virtual void Present(const std::vector<Swapchain*>& swapchains) = 0;

    protected:
        std::unique_ptr<GPUAdapter> _adapter;

        GPUFeatures _features{};
        GPULimits _limits{};
        std::shared_ptr<GraphicsContext> graphicsContext;

    private:
        /// Tracked gpu resource.
        std::mutex _gpuResourceMutex;
        std::vector<GPUResource*> _gpuResources;

    private:
        ALIMER_DISABLE_COPY_MOVE(GPUDevice);
    };
}
