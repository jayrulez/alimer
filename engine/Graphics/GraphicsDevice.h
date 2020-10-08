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
#include "Graphics/Types.h"
#include <memory>
#include <mutex>

namespace Alimer
{
    class ALIMER_API GraphicsDevice : public RefCounted
    {
    public:
        /// Destructor
        virtual ~GraphicsDevice() = default;

        static RefPtr<GraphicsDevice> Create(const PresentationParameters& presentationParameters, GraphicsBackendType preferredBackendType, GraphicsDeviceFlags flags = GraphicsDeviceFlags::None);

        /// Get whether device is lost.
        virtual bool IsDeviceLost() const = 0;

        /// Wait for GPU to finish pending operation and become idle.
        virtual void WaitForGPU() = 0;

        virtual bool BeginFrame() = 0;
        virtual void EndFrame() = 0;

        virtual CommandContext* GetImmediateContext() const = 0;

        virtual RefPtr<ResourceUploadBatch> CreateResourceUploadBatch() = 0;

        virtual SwapChain* CreateSwapChain() = 0;

        virtual GraphicsBuffer* CreateBuffer(BufferUsage usage, uint32_t count, uint32_t stride, const char* label = nullptr) = 0;
        virtual GraphicsBuffer* CreateStaticBuffer(ResourceUploadBatch* batch, BufferUsage usage, const void* data, uint32_t count, uint32_t stride, const char* label = nullptr) = 0;

        /// Gets the device backend type.
        GraphicsBackendType GetBackendType() const { return caps.backendType; }

        /// Get the device caps.
        const GraphicsDeviceCaps& GetCaps() const { return caps; }

    protected:
        GraphicsDevice(const PresentationParameters& presentationParameters);

        GraphicsDeviceCaps caps{};
        uint32_t backbufferWidth;
        uint32_t backbufferHeight;

        /// Add a GPU object to keep track of. Called by GraphicsResource.
        //void AddGraphicsResource(GraphicsResource* resource);
        /// Remove a GPU object. Called by GraphicsResource.
        //void RemoveGraphicsResource(GraphicsResource* resource);
        /// Mutex for accessing the GPU objects vector from several threads.
        //std::mutex gpuObjectMutex;
        /// GPU objects.
        //std::vector<GraphicsResource*> gpuObjects;
    };


    /* Helper methods */
    inline uint32_t RHICalculateMipLevels(uint32_t width, uint32_t height, uint32_t depth = 1u)
    {
        uint32_t mipLevels = 0;
        uint32_t size = Max(Max(width, height), depth);
        while (1u << mipLevels <= size) {
            ++mipLevels;
        }

        if (1u << mipLevels < size) {
            ++mipLevels;
        }

        return mipLevels;
    }
}

