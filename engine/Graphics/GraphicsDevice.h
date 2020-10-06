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
#include "Graphics/SwapChain.h"
#include <memory>
#include <mutex>

namespace Alimer
{
    enum class HeapType
    {
        Default,
        Upload,
        Readback
    };

    /// Defines a base RHI resource class.
    class ALIMER_API RHIResource
    {
    public:
        enum class Type
        {
            Buffer,
            Texture,
            Sampler,
            SwapChain
        };

        virtual ~RHIResource() = default;

        /// Unconditionally destroy the GPU resource.
        virtual void Destroy() = 0;

        /// Get the resource type.
        ALIMER_FORCE_INLINE Type GetType() const { return type; }

        /// Get the resource heap type.
        ALIMER_FORCE_INLINE HeapType GetHeapType() const { return heapType; }

        /// Set the resource name.
        virtual void SetName(const std::string& newName) { name = newName; }

        /// Get the resource name
        const std::string& GetName() const { return name; }

    protected:
        RHIResource(Type type_, HeapType heapType_)
            : type(type_)
            , heapType(heapType_)
        {

        }

    protected:
        Type type;
        HeapType heapType;
        std::string name;
    };

    class RHIBuffer : public RHIResource
    {
    public:
        enum class Usage : uint32_t
        {
            None = 0,
            Vertex = 1 << 0,
            Index = 1 << 1,
            Uniform = 1 << 2,
            Storage = 1 << 3,
            Indirect = 1 << 4
        };

        /// Gets buffer usage.
        ALIMER_FORCE_INLINE Usage GetUsage() const{ return usage; }

        /// Gets buffer size in bytes.
        ALIMER_FORCE_INLINE uint64_t GetSize() const { return size; }

    protected:
        /// Constructor
        RHIBuffer(Usage usage_, uint64_t size_, HeapType heapType_)
            : RHIResource(Type::Buffer, heapType_)
            , usage(usage_)
            , size(size_)
        {

        }

        Usage usage;
        uint64_t size;
    };

    ALIMER_DEFINE_ENUM_FLAG_OPERATORS(RHIBuffer::Usage, uint32_t);

    /// Batch used for inizializing resource data.
    class ALIMER_API RHIResourceUploadBatch
    {
    public:
        virtual ~RHIResourceUploadBatch() = default;

    protected:
        RHIResourceUploadBatch()
        {

        }
    };

    /// Defines the graphics subsystem.
    class ALIMER_API GraphicsDevice
    {
        friend class GraphicsResource;

    public:
        /// The single instance of the graphics device.
        static GraphicsDevice* Instance;

        static bool Initialize(const std::string& applicationName, GraphicsBackendType preferredBackendType, GraphicsDeviceFlags flags = GraphicsDeviceFlags::None);
        static void Shutdown();

        /// Get whether device is lost.
        virtual bool IsDeviceLost() const = 0;

        /// Wait for GPU to finish pending operation and become idle.
        virtual void WaitForGPU() = 0;

        virtual FrameOpResult BeginFrame(SwapChain* swapChain, BeginFrameFlags flags = BeginFrameFlags::None) = 0;
        virtual FrameOpResult EndFrame(SwapChain* swapChain, EndFrameFlags flags = EndFrameFlags::None) = 0;

        virtual SwapChain* CreateSwapChain() = 0;

        virtual RHIBuffer* CreateBuffer(RHIBuffer::Usage usage, uint64_t size, HeapType heapType = HeapType::Default) = 0;
        virtual RHIBuffer* CreateStaticBuffer(RHIResourceUploadBatch* batch, const void* initialData, RHIBuffer::Usage usage, uint64_t size) = 0;

        /// Gets the device backend type.
        GraphicsBackendType GetBackendType() const { return caps.backendType; }

        /// Get the device caps.
        const GraphicsDeviceCaps& GetCaps() const { return caps; }

    protected:
        GraphicsDevice() = default;
        virtual ~GraphicsDevice() = default;

        /// Add a GPU object to keep track of. Called by GraphicsResource.
        void AddGraphicsResource(GraphicsResource* resource);
        /// Remove a GPU object. Called by GraphicsResource.
        void RemoveGraphicsResource(GraphicsResource* resource);

        GraphicsDeviceCaps caps{};

        /// Mutex for accessing the GPU objects vector from several threads.
        std::mutex gpuObjectMutex;
        /// GPU objects.
        std::vector<GraphicsResource*> gpuObjects;
    };

}

