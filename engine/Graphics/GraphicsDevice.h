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

#include "Core/Vector.h"
#include "Math/Rect.h"
#include "Math/Viewport.h"
#include "Graphics/SwapChain.h"
#include <set>
#include <mutex>

namespace alimer
{
    class GraphicsBuffer;
    class Texture;
    class SwapChain;

    class ALIMER_API GraphicsDeviceEvents
    {
    public:
        virtual void OnDeviceLost() = 0;
        virtual void OnDeviceRestored() = 0;

    protected:
        ~GraphicsDeviceEvents() = default;
    };

    /// Defines the logical graphics device class.
    class ALIMER_API GraphicsDevice 
    {
        friend class GraphicsResource;

    public:
        static BackendType PreferredBackendType;

        /// Get the singleton instance of GraphicsDevice.
        static GraphicsDevice* Instance;

        static std::set<BackendType> GetAvailableBackends();
        static void Initialize();
        static void Shutdown();

        virtual void WaitForGPU() = 0;
        void BeginFrame();
        void EndFrame();

        void Resize(uint32_t width, uint32_t height);

        // Resource creation methods.
        virtual RefPtr<SwapChain> CreateSwapChain(const SwapChainDescription& desc) = 0;
        virtual RefPtr<Texture> CreateTexture(const TextureDescription& desc, const void* initialData = nullptr) = 0;

        // CommandList
        virtual CommandList BeginCommandList(const char* name) = 0;
        virtual void InsertDebugMarker(CommandList commandList, const char* name) = 0;
        virtual void PushDebugGroup(CommandList commandList, const char* name) = 0;
        virtual void PopDebugGroup(CommandList commandList) = 0;

        virtual void SetScissorRect(CommandList commandList, const Rect& scissorRect) = 0;
        virtual void SetScissorRects(CommandList commandList, const Rect* scissorRects, uint32_t count) = 0;
        virtual void SetViewport(CommandList commandList, const Viewport& viewport) = 0;
        virtual void SetViewports(CommandList commandList, const Viewport* viewports, uint32_t count) = 0;
        virtual void SetBlendColor(CommandList commandList, const Color& color) = 0;

        virtual void BindBuffer(CommandList commandList, uint32_t slot, GraphicsBuffer* buffer) = 0;
        virtual void BindBufferData(CommandList commandList, uint32_t slot, const void* data, uint32_t size) = 0;

        /// Get the device capabilities.
        const GraphicsDeviceCaps& GetCaps() const { return caps; }

    private:
        virtual void BackendShutdown() = 0;
        virtual bool BeginFrameImpl() { return true; }
        virtual void EndFrameImpl() = 0;

        void TrackResource(GraphicsResource* resource);
        void UntrackResource(GraphicsResource* resource);

    protected:
        GraphicsDevice() = default;
        virtual ~GraphicsDevice() = default;

        void ReleaseTrackedResources();

        bool headless = false;
        GraphicsDeviceCaps caps{};
        uint32_t backbufferWidth = 0;
        uint32_t backbufferHeight = 0;
        float dpiScale = 1.0f;

        std::mutex trackedResourcesMutex;
        Vector<GraphicsResource*> trackedResources;
        GraphicsDeviceEvents* events = nullptr;

        /// Current active frame index
        uint32_t frameIndex{ 0 };

        /// Whether a frame is active or not
        bool frameActive{ false };

    private:
        ALIMER_DISABLE_COPY_MOVE(GraphicsDevice);
    };
}
