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

#include "RHI/Types.h"
#include "RHI/GraphicsResource.h"
#include "RHI/Texture.h"
#include "Math/Size.h"
#include <memory>
#include <mutex>

namespace Alimer
{
    class Window;
    class RHIResource;
    class RHIBuffer;
    class RHISwapChain;

    enum class MemoryUsage
    {
        GpuOnly,
        CpuOnly,
        CpuToGpu,
        GpuToCpu
    };

    enum class FrameOpResult : uint32_t {
        Success = 0,
        Error,
        SwapChainOutOfDate,
        DeviceLost
    };

    enum class BeginFrameFlags : uint32_t {
        None = 0
    };
    ALIMER_DEFINE_ENUM_FLAG_OPERATORS(BeginFrameFlags, uint32_t);

    enum class EndFrameFlags : uint32_t {
        None = 0,
        SkipPresent = 1 << 0
    };
    ALIMER_DEFINE_ENUM_FLAG_OPERATORS(EndFrameFlags, uint32_t);

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

        /// Get the resource memory usage.
        ALIMER_FORCE_INLINE MemoryUsage GetMemoryUsage() const { return memoryUsage; }

        /// Set the resource name.
        virtual void SetName(const std::string& newName) { name = newName; }

        /// Get the resource name
        const std::string& GetName() const { return name; }

    protected:
        RHIResource(Type type_, MemoryUsage memoryUsage_ = MemoryUsage::GpuOnly)
            : type(type_)
            , memoryUsage(memoryUsage_)
        {

        }

    protected:
        Type type;
        MemoryUsage memoryUsage;
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
        RHIBuffer(Usage usage_, uint64_t size_, MemoryUsage memoryUsage_)
            : RHIResource(Type::Buffer, memoryUsage_)
            , usage(usage_)
            , size(size_)
        {

        }

        Usage usage;
        uint64_t size;
    };

    class ALIMER_API RHISwapChain : public RHIResource
    {
    public:
        Window* GetWindow() const { return window; }
        void SetWindow(Window* newWindow) { window = newWindow; }

        bool GetAutoResizeDrawable() const noexcept { return autoResizeDrawable; }
        void SetAutoResizeDrawable(bool value) noexcept { autoResizeDrawable = value; }

        SizeI GetDrawableSize() const noexcept { return drawableSize; }
        void SetDrawableSize(const SizeI& value) noexcept { drawableSize = value; }

        PixelFormat GetColorFormat() const noexcept { return colorFormat; }
        void SetColorFormat(PixelFormat value) noexcept { colorFormat = value; }

        PixelFormat GetDepthStencilFormat() const noexcept { return depthStencilFormat; }
        void SetDepthStencilFormat(PixelFormat value) noexcept { depthStencilFormat = value; }

        uint32_t GetSampleCount() const noexcept { return sampleCount; }
        void SetSampleCount(uint32_t value) noexcept { sampleCount = value; }

        virtual bool CreateOrResize() = 0;

        virtual Texture* GetCurrentTexture() const = 0;

    protected:
        /// Constructor.
        RHISwapChain()
            : RHIResource(Type::SwapChain)
        {

        }

        Window* window = nullptr;
        bool autoResizeDrawable = true;
        SizeI drawableSize;
        PixelFormat colorFormat = PixelFormat::BGRA8Unorm;
        PixelFormat depthStencilFormat = PixelFormat::Depth32Float;
        uint32_t sampleCount = 1u;
        bool verticalSync = true;
    };

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

        virtual FrameOpResult BeginFrame(RHISwapChain* swapChain, BeginFrameFlags flags = BeginFrameFlags::None) = 0;
        virtual FrameOpResult EndFrame(RHISwapChain* swapChain, EndFrameFlags flags = EndFrameFlags::None) = 0;

        virtual RHISwapChain* CreateSwapChain() = 0;

        virtual RHIBuffer* CreateBuffer(RHIBuffer::Usage usage, uint64_t size, MemoryUsage memoryUsage = MemoryUsage::GpuOnly) = 0;
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

    /* Enum flags operators */
    ALIMER_DEFINE_ENUM_FLAG_OPERATORS(RHIBuffer::Usage, uint32_t);

    /* Helper methods */
    ALIMER_API const char* ToString(FrameOpResult value);
}

