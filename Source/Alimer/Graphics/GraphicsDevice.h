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

#include "Graphics/CommandContext.h"
#include "Graphics/Swapchain.h"
#include "Graphics/Buffer.h"
#include <vector>
#include <mutex>

namespace alimer
{
    class ALIMER_API GraphicsDeviceEvents
    {
    public:
        virtual void OnDeviceLost() = 0;
        virtual void OnDeviceRestored() = 0;

    protected:
        ~GraphicsDeviceEvents() = default;
    };

    class Window;

    /// Defines the logical graphics subsystem.
    class ALIMER_API GraphicsDevice
    {
        friend class GraphicsResource;

    public:
        virtual ~GraphicsDevice() = default;

        static void SetPreferredBackend(GPUBackendType backend);
        static bool Initialize(const std::string& applicationName, Window* window, GPUFlags flags = GPUFlags::None);
        static void Shutdown();
        virtual void WaitForGPU() = 0;
        virtual bool BeginFrame() = 0;
        virtual void EndFrame() = 0;

        /// Get the device capabilities.
        const GraphicsDeviceCapabilities& GetCaps() const { return caps; }

        /// Get the main swapchain created from window.
        virtual Swapchain* GetMainSwapchain() const = 0;

        /**
        * Get the immediate command context.
        * The default context is managed completely by the device.
        * The user should just queue commands into it, the device will take care of allocation, submission and synchronization.
        */
        virtual CommandContext* GetImmediateContext() const = 0;

        /* Resource creation methods */
        virtual SharedPtr<Swapchain> CreateSwapchain(const SwapchainDescription& description) = 0;

    private:
        void TrackResource(GraphicsResource* resource);
        void UntrackResource(GraphicsResource* resource);

    protected:
        GraphicsDevice() = default;
        void ReleaseTrackedResources();

        GraphicsDeviceCapabilities caps{};

        std::mutex trackedResourcesMutex;
        std::vector<GraphicsResource*> trackedResources;
        GraphicsDeviceEvents* events = nullptr;


    private:
        static GPUBackendType preferredBackend;

        ALIMER_DISABLE_COPY_MOVE(GraphicsDevice);
    };

    extern ALIMER_API GraphicsDevice* Graphics;

    ALIMER_FORCEINLINE SharedPtr<Swapchain> CreateSwapchain(const SwapchainDescription& description)
    {
        return Graphics->CreateSwapchain(description);
    }

    ALIMER_FORCEINLINE SharedPtr<Texture> GPUCreateTexture2D()
    {
        return nullptr;
        //return GPU->CreateTexture();
    }
}

