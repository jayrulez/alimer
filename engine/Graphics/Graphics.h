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
    class ALIMER_API Graphics
    {
        friend class GraphicsResource;

    public:
        virtual ~Graphics() = default;

        static bool Initialize(Window* window, GPUFlags flags = GPUFlags::None);
        static void Shutdown();

        virtual void WaitForGPU() = 0;
        virtual bool BeginFrame() = 0;
        virtual void EndFrame() = 0;

        /// Get the current backbuffer texture.
        virtual Texture* GetBackbufferTexture() const = 0;

        /// Get the device capabilities.
        const GPU::Capabilities& GetCaps() const { return caps; }

    private:
        void TrackResource(GraphicsResource* resource);
        void UntrackResource(GraphicsResource* resource);

    protected:
        Graphics(Window* window);
        void ReleaseTrackedResources();

        static constexpr uint64_t kBackbufferCount = 2u;
        static constexpr uint64_t kMaxBackbufferCount = 3u;

        GPU::Capabilities caps{};

        std::mutex trackedResourcesMutex;
        std::vector<GraphicsResource*> trackedResources;
        GraphicsDeviceEvents* events = nullptr;
        uint32 backbufferWidth = 0;
        uint32 backbufferHeight = 0;
        uint32 backbufferCount = kBackbufferCount;
        bool vSync = true;

    private:
        ALIMER_DISABLE_COPY_MOVE(Graphics);
    };

    extern ALIMER_API Graphics* GPU;

}

namespace GPU
{
    ALIMER_API void SetPreferredBackend(BackendType backend);

    ALIMER_FORCEINLINE void Shutdown()
    {
        alimer::GPU->Shutdown();
    }

    ALIMER_FORCEINLINE bool BeginFrame()
    {
        return alimer::GPU->BeginFrame();
    }

    ALIMER_FORCEINLINE void EndFrame()
    {
        alimer::GPU->EndFrame();
    }

    ALIMER_FORCEINLINE const GPU::Capabilities& GetCaps()
    {
        return alimer::GPU->GetCaps();
    }
}
