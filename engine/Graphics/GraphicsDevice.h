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
#include <set>
#include <memory>
#include <mutex>

namespace Alimer
{
    /// Defines the graphics subsystem.
    class ALIMER_API GraphicsDevice : public Object
    {
        ALIMER_OBJECT(GraphicsDevice, Object);

    public:
        virtual ~GraphicsDevice() = default;

        static std::set<GPUBackendType> GetAvailableBackends();
        static GraphicsDevice* Create(GPUBackendType preferredBackend, const GraphicsDeviceDescription& desc);

        /// Wait for GPU to finish pending operation and become idle.
        virtual void WaitForGPU() = 0;

        /// Begin rendering frame.
        virtual bool BeginFrame() = 0;

        /// End current rendering frame and present swap chain on screen.
        void EndFrame(SwapChain* swapChain);

        /// End current rendering frame and present swap chains on screen.
        virtual void EndFrame(const std::vector<SwapChain*>& swapChains) = 0;

        /// Gets the device backend type.
        GPUBackendType GetBackendType() const { return caps.backendType; }

        /// Get the device caps.
        const GraphicsDeviceCaps& GetCaps() const;

        /// Get the main/primary SwapChain.
        SwapChain* GetPrimarySwapChain() const;

        inline uint64_t GetFrameCount() const { return frameCount;  }

        /// Add a GPU object to keep track of. Called by GraphicsResource.
        void AddGraphicsResource(GraphicsResource* resource);
        /// Remove a GPU object. Called by GraphicsResource.
        void RemoveGraphicsResource(GraphicsResource* resource);

        // Resource creation
        virtual RefPtr<GPUBuffer> CreateBuffer(const BufferDescription& desc, const void* initialData = nullptr);

        /// Begin command list recording (Check GraphicsDeviceCaps.features.commandLists for support, if false this method will always return 0).
        virtual CommandList BeginCommandList() = 0;

        virtual void PushDebugGroup(CommandList commandList, const char* name) = 0;
        virtual void PopDebugGroup(CommandList commandList) = 0;
        virtual void InsertDebugMarker(CommandList commandList, const char* name) = 0;

        virtual void BeginRenderPass(CommandList commandList, const RenderPassDescription* renderPass) = 0;
        virtual void EndRenderPass(CommandList commandList) = 0;

        virtual void SetScissorRect(CommandList commandList, const RectI& scissorRect) = 0;
        virtual void SetScissorRects(CommandList commandList, const RectI* scissorRects, uint32_t count) = 0;
        virtual void SetViewport(CommandList commandList, const Viewport& viewport) = 0;
        virtual void SetViewports(CommandList commandList, const Viewport* viewports, uint32_t count) = 0;
        virtual void SetBlendColor(CommandList commandList, const Color& color) = 0;

    private:
        virtual RefPtr<GPUBuffer> CreateBufferCore(const BufferDescription& desc, const void* initialData) = 0;

    protected:
        GraphicsDevice();

        GraphicsDeviceCaps caps{};
        RefPtr<SwapChain> primarySwapChain;

        uint64_t frameCount{ 0 };

    private:
        /// Mutex for accessing the GPU objects vector from several threads.
        std::mutex gpuObjectMutex;

        /// GPU objects.
        std::vector<GraphicsResource*> gpuObjects;

        ALIMER_DISABLE_COPY_MOVE(GraphicsDevice);
    };
}

