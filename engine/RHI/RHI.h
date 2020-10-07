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

#include "Core/Object.h"
#include "RHI/Types.h"
#include "Math/Size.h"
#include <memory>
#include <mutex>

namespace Alimer
{
    class Window;
    class RHIResource;
    class RHIBuffer;
    class RHITexture;
    class RHISwapChain;
    class RHICommandBuffer;

    enum class MemoryUsage
    {
        GpuOnly,
        CpuOnly,
        CpuToGpu,
        GpuToCpu
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

    /// A 3D rectangular region for the viewport clipping.
    class ALIMER_API RHIViewport
    {
    public:
        /// The x coordinate of the upper-left corner of the viewport.
        float x;
        /// The y coordinate of the upper-left corner of the viewport.
        float y;
        /// The width of the viewport, in pixels.
        float width;
        /// The width of the viewport, in pixels.
        float height;
        /// The z coordinate of the near clipping plane of the viewport.
        float minDepth;
        /// The z coordinate of the far clipping plane of the viewport.
        float maxDepth;

        /// Constructor.
        RHIViewport() noexcept : x(0.0f), y(0.0f), width(0.0f), height(0.0f), minDepth(0.0f), maxDepth(1.0f) {}
        constexpr RHIViewport(float x_, float y_, float width_, float ih, float iminz = 0.f, float imaxz = 1.f) noexcept
            : x(x_), y(y_), width(width_), height(ih), minDepth(iminz), maxDepth(imaxz) {}

        RHIViewport(const RHIViewport&) = default;
        RHIViewport& operator=(const RHIViewport&) = default;

        RHIViewport(RHIViewport&&) = default;
        RHIViewport& operator=(RHIViewport&&) = default;

        // Comparison operators
        bool operator == (const RHIViewport& rhs) const noexcept
        {
            return (x == rhs.x && y == rhs.y && width == rhs.width && height == rhs.height && minDepth == rhs.minDepth && maxDepth == rhs.maxDepth);
        }

        bool operator != (const RHIViewport& rhs) const noexcept
        {
            return (x != rhs.x || y != rhs.y || width != rhs.width || height != rhs.height || minDepth != rhs.minDepth || maxDepth != rhs.maxDepth);
        }

        // Viewport operations
        float AspectRatio() const;
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

    class ALIMER_API RHITexture : public Object, public RHIResource
    {
        ALIMER_OBJECT(RHITexture, Object);

    public:
        /// Defines the usage of texture resource.
        enum class Usage : uint32_t
        {
            None = 0,
            Sampled = (1 << 0),
            Storage = (1 << 1),
            RenderTarget = (1 << 2),
            GenerateMipmaps = (1 << 3)
        };

        /// Get the texture pixel format.
        PixelFormat GetFormat() const { return format; }

        /// Get a mip-level width.
        uint32 GetWidth(uint32 mipLevel = 0) const { return (mipLevel == 0) || (mipLevel < mipLevels) ? Max(1u, width >> mipLevel) : 0; }

        /// Get a mip-level height.
        uint32 GetHeight(uint32 mipLevel = 0) const { return (mipLevel == 0) || (mipLevel < mipLevels) ? Max(1u, height >> mipLevel) : 0; }

        /// Get a mip-level depth.
        uint32 GetDepth(uint32 mipLevel = 0) const { return (mipLevel == 0) || (mipLevel < mipLevels) ? Max(1u, height >> depthOrArraySize) : 0; }

        /// Gets number of mipmap levels of the texture.
        uint32 GetMipLevels() const { return mipLevels; }

        /// Get the array layers of the texture.
        uint32 GetArrayLayers() const { return depthOrArraySize; }

        /// Get the texture usage.
        Usage GetUsage() const { return usage; }

        /// Get the current texture layout.
        TextureLayout GetLayout() const { return layout; }

        /// Get the array index of a subresource.
        uint32 GetSubresourceArraySlice(uint32_t subresource) const { return subresource / mipLevels; }

        /// Get the mip-level of a subresource.
        uint32 GetSubresourceMipLevel(uint32_t subresource) const { return subresource % mipLevels; }

        /// Get the subresource index.
        uint32 GetSubresourceIndex(uint32 mipLevel, uint32 arraySlice) const { return mipLevel + arraySlice * mipLevels; }

        /// Calculates the resulting size at a single level for an original size.
        static uint32 CalculateMipSize(uint32 mipLevel, uint32 baseSize)
        {
            baseSize = baseSize >> mipLevel;
            return baseSize > 0u ? baseSize : 1u;
        }

    protected:
        /// Constructor
        RHITexture()
            : RHIResource(Type::Texture)
        {

        }

        TextureType type = TextureType::Type2D;
        PixelFormat format = PixelFormat::RGBA8Unorm;
        Usage usage = Usage::Sampled;
        uint32 width = 1u;
        uint32 height = 1u;
        uint32 depthOrArraySize = 1u;
        uint32 mipLevels = 1u;
        uint32 sampleCount = 1u;
        TextureLayout layout = TextureLayout::Undefined;
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

        virtual RHITexture* GetCurrentTexture() const = 0;
        virtual RHICommandBuffer* CurrentFrameCommandBuffer() = 0;

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

    /// 
    class ALIMER_API RHICommandBuffer
    {
    public:
        virtual ~RHICommandBuffer() = default;

        virtual void PushDebugGroup(const std::string& name) = 0;
        virtual void PopDebugGroup() = 0;
        virtual void InsertDebugMarker(const std::string& name) = 0;

        virtual void SetViewport(const RHIViewport& viewport) = 0;
        virtual void SetScissorRect(const RectI& scissorRect) = 0;
        virtual void SetBlendColor(const Color& color) = 0;

        virtual void BeginRenderPass(const RenderPassDesc& renderPass) = 0;
        virtual void EndRenderPass() = 0;

        //virtual void BindBuffer(uint32_t slot, GPUBuffer* buffer) = 0;
        //virtual void BindBufferData(uint32_t slot, const void* data, uint32_t size) = 0;

    protected:
        RHICommandBuffer()
        {

        }
    };

    /// Defines the RHI device.
    class ALIMER_API RHIDevice
    {
    public:
        enum class FrameOpResult : uint32_t {
            Success = 0,
            Error,
            SwapChainOutOfDate,
            DeviceLost
        };

        /// Destructor
        virtual ~RHIDevice() = default;

        static RHIDevice* Create(const std::string& applicationName, GraphicsBackendType preferredBackendType, GraphicsDeviceFlags flags = GraphicsDeviceFlags::None);

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
        RHIDevice() = default;

        GraphicsDeviceCaps caps{};

        /// Add a GPU object to keep track of. Called by GraphicsResource.
        //void AddGraphicsResource(GraphicsResource* resource);
        /// Remove a GPU object. Called by GraphicsResource.
        //void RemoveGraphicsResource(GraphicsResource* resource);
        /// Mutex for accessing the GPU objects vector from several threads.
        //std::mutex gpuObjectMutex;
        /// GPU objects.
        //std::vector<GraphicsResource*> gpuObjects;
    };

    /* Enum flags operators */
    ALIMER_DEFINE_ENUM_FLAG_OPERATORS(RHIBuffer::Usage, uint32_t);
    ALIMER_DEFINE_ENUM_FLAG_OPERATORS(RHITexture::Usage, uint32_t);

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

    ALIMER_API const char* ToString(RHIDevice::FrameOpResult value);
}

