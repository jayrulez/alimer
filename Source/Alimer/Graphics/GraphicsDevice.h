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

#include "Graphics/CommandBuffer.h"
#include "Graphics/Texture.h"
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
        static bool Initialize(Window* window, GPUFlags flags = GPUFlags::None);
        static void Shutdown();
        virtual void WaitForGPU() = 0;
        virtual void Frame() = 0;

        /// Get the current backbuffer texture.
        virtual Texture* GetBackbufferTexture() const = 0;

        /// Get the device capabilities.
        const GraphicsDeviceCapabilities& GetCaps() const { return caps; }

        virtual CommandBuffer& BeginCommandBuffer(const std::string_view id) = 0;

    private:
        void TrackResource(GraphicsResource* resource);
        void UntrackResource(GraphicsResource* resource);

    protected:
        GraphicsDevice(Window* window);
        void ReleaseTrackedResources();

        GraphicsDeviceCapabilities caps{};

        std::mutex trackedResourcesMutex;
        std::vector<GraphicsResource*> trackedResources;
        GraphicsDeviceEvents* events = nullptr;

        uint32_t backbufferWidth = 0;
        uint32_t backbufferHeight = 0;
        bool vSync = true;

    private:
        static GPUBackendType preferredBackend;

        ALIMER_DISABLE_COPY_MOVE(GraphicsDevice);
    };

    extern ALIMER_API GraphicsDevice* GPU;

    /// Register Graphics related object factories and attributes.
    ALIMER_API void RegisterGraphicsLibrary();


    ALIMER_FORCEINLINE SharedPtr<Texture> GPUCreateTexture2D()
    {
        return nullptr;
        //return GPU->CreateTexture();
    }
}

