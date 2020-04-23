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
#include "graphics/GPUResource.h"
#include "graphics/CommandContext.h"
#include <memory>
#include <mutex>

namespace alimer
{
    class CommandQueue;
    class SwapChain;

    /// Defines the logical GPU device class.
    class ALIMER_API GPUDevice : public RefCounted
    {
    public:
        static bool IsEnabledValidation();
        static void SetEnableValidation(bool value);

        /// Create new GPUDevice.
        static RefPtr<GPUDevice> Create(BackendType preferredBackend = BackendType::Count, GPUPowerPreference powerPreference = GPUPowerPreference::HighPerformance);

        /// Destructor.
        virtual ~GPUDevice() = default;

        /// Waits for the device to become idle.
        virtual void WaitForIdle() = 0;

        /**
        * Get a command queue. Valid types are:
        * - Graphics    : Can be used for draw, dispatch, or copy commands.
        * - Compute     : Can be used for dispatch or copy commands.
        * - Copy        : Can be used for copy commands.
        */
        std::shared_ptr<CommandQueue> GetCommandQueue(CommandQueueType type = CommandQueueType::Graphics) const;

        /// Create new SwapChain.
        RefPtr<SwapChain> CreateSwapChain(void* windowHandle, const SwapChainDescriptor* descriptor);

        /// Add a GPU resource to keep track of. Called by GPUResource.
        void AddGPUResource(GPUResource* resource);
        /// Remove a tracked GPU resource. Called by GPUResource.
        void RemoveGPUResource(GPUResource* resource);

        /// Get the features.
        inline const GPUDeviceCaps& GetCaps() const { return caps; }

    protected:
        virtual void ReleaseTrackedResources();
        virtual SwapChain* CreateSwapChainCore(void* windowHandle, const SwapChainDescriptor* descriptor) = 0;
        //virtual void Present(const std::vector<Swapchain*>& swapchains) = 0;

    protected:
        GPUDevice() = default;

        virtual bool Init(GPUPowerPreference powerPreference) = 0;

        GPUDeviceCaps caps{};
        std::shared_ptr<CommandQueue> graphicsCommandQueue;
        std::shared_ptr<CommandQueue> computeCommandQueue;
        std::shared_ptr<CommandQueue> copyCommandQueue;

    private:
        /// Tracked gpu resource.
        std::mutex _gpuResourceMutex;
        std::vector<GPUResource*> _gpuResources;
        static bool enableValidation;
        static bool enableGPUBasedValidation;

    private:
        ALIMER_DISABLE_COPY_MOVE(GPUDevice);
    };
}
