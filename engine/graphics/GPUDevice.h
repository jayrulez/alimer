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
#include "graphics/SwapChain.h"
#include "graphics/GPUResource.h"
#include "graphics/CommandContext.h"
#include <memory>
#include <mutex>

namespace alimer
{
#ifdef _DEBUG
#   define DEFAULT_ENABLE_VALIDATION true
#else
#   define DEFAULT_ENABLE_VALIDATION false
#endif

    class Window;
    class CommandQueue;
    struct GPUDeviceApiData;

    /// Defines the logical GPU device class.
    class ALIMER_API GPUDevice final : public RefCounted
    {
    public:
        /**
        * Device configuration
        */
        struct Desc
        {
            bool validation = DEFAULT_ENABLE_VALIDATION;
            GPUPowerPreference powerPreference = GPUPowerPreference::HighPerformance;
            PixelFormat colorFormat = PixelFormat::BGRA8UnormSrgb;
            PixelFormat depthStencilFormat = PixelFormat::D32Float;
        };

        /// Destructor.
        ~GPUDevice();

        /// Create new GPUDevice.
        static RefPtr<GPUDevice> Create(Window* window, const Desc& desc);

        /// Waits for the device to become idle.
        void WaitForIdle();

        /**
        * Get a command queue. Valid types are:
        * - Graphics    : Can be used for draw, dispatch, or copy commands.
        * - Compute     : Can be used for dispatch or copy commands.
        * - Copy        : Can be used for copy commands.
        */
        std::shared_ptr<CommandQueue> GetCommandQueue(CommandQueueType type = CommandQueueType::Graphics) const;

        /// Add a GPU resource to keep track of. Called by GPUResource.
        void AddGPUResource(GPUResource* resource);
        /// Remove a tracked GPU resource. Called by GPUResource.
        void RemoveGPUResource(GPUResource* resource);

        /// Get the features.
        inline const GPUDeviceCaps& GetCaps() const { return caps; }

        /**
        * Get the native API handle.
        */
        DeviceHandle GetHandle() const;

    protected:
        virtual void ReleaseTrackedResources();
        //virtual void Present(const std::vector<Swapchain*>& swapchains) = 0;

    private:
        GPUDevice(Window* window, const Desc& desc);

        bool Init();

        /* Backend methods */
        bool ApiInit();
        void ApiDestroy();

    protected:
        GPUDeviceCaps caps{};
        std::unique_ptr<SwapChain> mainSwapChain;
        std::shared_ptr<CommandQueue> graphicsCommandQueue;
        std::shared_ptr<CommandQueue> computeCommandQueue;
        std::shared_ptr<CommandQueue> copyCommandQueue;

    private:
        Window* window;
        Desc desc;
        GPUDeviceApiData* apiData = nullptr;

        /// Tracked gpu resource.
        std::mutex _gpuResourceMutex;
        std::vector<GPUResource*> _gpuResources;

    private:
        ALIMER_DISABLE_COPY_MOVE(GPUDevice);
    };
}
