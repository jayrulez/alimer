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

#include "Core/Ptr.h"
#include "Core/Containers.h"
#include "Graphics/Types.h"
#include "Platform/WindowHandle.h"
#include <mutex>

namespace Alimer
{
    class GraphicsResource;
    class ResourceUploadBatch;
    class GraphicsBuffer;
    class Texture;
    class SwapChain;

    class ALIMER_API GraphicsDevice 
    {
    public:
        /// Destructor
        virtual ~GraphicsDevice() = default;

        static Set<GraphicsBackendType> GetAvailableBackends();

        static std::shared_ptr<GraphicsDevice> Create(GraphicsBackendType preferredBackendType = GraphicsBackendType::Count, GraphicsDeviceFlags flags = GraphicsDeviceFlags::None);

        /// Get whether device is lost.
        virtual bool IsDeviceLost() const = 0;

        /// Wait for GPU to finish pending operation and become idle.
        virtual void WaitForGPU() = 0;

        virtual bool BeginFrame() = 0;
        virtual void EndFrame() = 0;

        /// Get the native handle (ID3D12Device, ID3D11Device1, VkDevice)
        virtual void* GetNativeHandle() const = 0;

        /// Gets the device backend type.
        GraphicsBackendType GetBackendType() const { return caps.backendType; }

        /// Get the device caps.
        const GraphicsDeviceCaps& GetCaps() const { return caps; }

        virtual RefPtr<SwapChain> CreateSwapChain(WindowHandle windowHandle, PixelFormat backbufferFormat = PixelFormat::BGRA8Unorm) = 0;

    protected:
        GraphicsDevice() = default;

        static constexpr uint32_t kRenderLatency = 2u;

        GraphicsDeviceCaps caps{};
        uint32_t backbufferWidth = 0;
        uint32_t backbufferHeight = 0;

        uint64 frameCount = 0;

        /// Add a GPU object to keep track of. Called by GraphicsResource.
        //void AddGraphicsResource(GraphicsResource* resource);
        /// Remove a GPU object. Called by GraphicsResource.
        //void RemoveGraphicsResource(GraphicsResource* resource);
        /// Mutex for accessing the GPU objects vector from several threads.
        //std::mutex gpuObjectMutex;
        /// GPU objects.
        //std::vector<GraphicsResource*> gpuObjects;
    };
}

