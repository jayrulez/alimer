//
// Copyright (c) 2019-2020 Amer Koleci and contributors.
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

#include "Math/Color.h"
#include "Math/Rect.h"
#include "Graphics/PixelFormat.h"

namespace Alimer
{
    static constexpr uint32_t kMaxColorAttachments = 8u;
    static constexpr uint32_t kMaxVertexBufferBindings = 8u;
    static constexpr uint32_t kMaxVertexAttributes = 16u;
    static constexpr uint32_t kMaxVertexAttributeOffset = 2047u;
    static constexpr uint32_t kMaxVertexBufferStride = 2048u;
    static constexpr uint32_t kMaxViewportAndScissorRects = 8u;

    static constexpr uint32_t KnownVendorId_AMD = 0x1002;
    static constexpr uint32_t KnownVendorId_Intel = 0x8086;
    static constexpr uint32_t KnownVendorId_Nvidia = 0x10DE;
    static constexpr uint32_t KnownVendorId_Microsoft = 0x1414;
    static constexpr uint32_t KnownVendorId_ARM = 0x13B5;
    static constexpr uint32_t KnownVendorId_ImgTec = 0x1010;
    static constexpr uint32_t KnownVendorId_Qualcomm = 0x5143;

    enum class GraphicsDeviceFlags : uint32
    {
        None = 0,
        DebugRuntime = 1 << 0,
        GPUBasedValidation = 1 << 2,
        RenderDoc = 1 << 3,
        LowPowerPreference = 1 << 4,
    };
    ALIMER_DEFINE_ENUM_FLAG_OPERATORS(GraphicsDeviceFlags, uint32);

    enum class GPUAdapterType
    {
        DiscreteGPU,
        IntegratedGPU,
        CPU,
        Unknown
    };

    /// Enum describing the rendering backend.
    enum class GraphicsBackendType : uint32
    {
        /// Null renderer.
        Null,
        /// Direct3D 11 backend.
        Direct3D11,
        /// Direct3D 12 backend.
        Direct3D12,
        /// Metal backend.
        Metal,
        /// Vulkan backend.
        Vulkan,
        /// Default best platform supported backend.
        Count
    };

    enum class BufferUsage : uint32_t
    {
        None = 0,
        Vertex = 1 << 0,
        Index = 1 << 1,
        Uniform = 1 << 2,
        Storage = 1 << 3,
        Indirect = 1 << 4,
        Dynamic = 1 << 5,
        Staging = 1 << 6,
    };
    ALIMER_DEFINE_ENUM_FLAG_OPERATORS(BufferUsage, uint32_t);

    /// Describes the texture type.
    enum class TextureType
    {
        /// A two-dimensional texture image.
        Type2D,
        /// A three-dimensional texture image.
        Type3D,
        /// A cube texture with six two-dimensional images.
        TypeCube
    };

    enum class TextureLayout : uint32
    {
        Undefined,
        General,
        RenderTarget,
        DepthStencil,
        DepthStencilReadOnly,
        Present
    };

    enum class LoadAction : uint32_t
    {
        DontCare,
        Load,
        Clear
    };

    enum class StoreAction : uint32_t
    {
        DontCare,
        Store,
    };


    /* Structs */
    struct BufferDescription
    {
        BufferUsage usage;
        uint32_t size;
        uint32_t stride;
    };

    struct TextureDescription
    {
        TextureType type = TextureType::Type2D;
        PixelFormat format = PixelFormat::RGBA8Unorm;
        uint32 width = 1u;
        uint32 height = 1u;
        uint32 depthOrArraySize = 1u;
        uint32 mipLevels = 1u;
        uint32 sampleCount = 1u;
        TextureLayout initialLayout = TextureLayout::Undefined;

        static TextureDescription New2D(PixelFormat format, uint32 width, uint32 height, bool mipmapped = false)
        {
            TextureDescription desc = {};
            desc.type = TextureType::Type2D;
            desc.format = format;
            desc.width = width;
            desc.height = height;
            desc.depthOrArraySize = 1u;
            desc.mipLevels = mipmapped ? 0u : 1u;
            desc.sampleCount = 1u;
            return desc;
        }
    };

    class RHITexture;
    struct RenderPassColorAttachment
    {
        RHITexture* texture = nullptr;
        uint32_t mipLevel = 0;
        uint32_t slice;
        LoadAction loadAction = LoadAction::Clear;
        StoreAction storeAction = StoreAction::Store;
        Color clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    };

    struct RenderPassDepthStencilAttachment
    {
        RHITexture* texture = nullptr;
        uint32_t mipLevel = 0;
        uint32_t slice;
        LoadAction depthLoadAction = LoadAction::Clear;
        StoreAction depthStoreAction = StoreAction::Store;
        LoadAction stencilLoadAction = LoadAction::DontCare;
        StoreAction stencilStoreAction = StoreAction::DontCare;
        float clearDepth = 1.0f;
        uint8_t clearStencil = 0;
    };

    struct RenderPassDesc
    {
        // Render area will be clipped to the actual framebuffer.
        //RectU renderArea = { UINT32_MAX, UINT32_MAX };

        RenderPassColorAttachment colorAttachments[kMaxColorAttachments];
        RenderPassDepthStencilAttachment depthStencilAttachment;
    };

    struct PresentationParameters
    {
        void* windowHandle;
        uint32_t backBufferWidth = 0;
        uint32_t backBufferHeight = 0;
        PixelFormat backBufferFormat = PixelFormat::BGRA8UnormSrgb;
        PixelFormat depthStencilFormat = PixelFormat::Depth32Float;
        bool isFullscreen = false;
        bool verticalSync = true;
    };

    struct GraphicsDeviceFeatures
    {
        bool baseVertex = false;
        bool independentBlend = false;
        bool computeShader = false;
        bool geometryShader = false;
        bool tessellationShader = false;
        bool logicOp = false;
        bool multiViewport = false;
        bool fullDrawIndexUint32 = false;
        bool multiDrawIndirect = false;
        bool fillModeNonSolid = false;
        bool samplerAnisotropy = false;
        bool textureCompressionETC2 = false;
        bool textureCompressionASTC_LDR = false;
        bool textureCompressionBC = false;
        /// Specifies whether cube array textures are supported.
        bool textureCubeArray = false;
        /// Specifies whether raytracing is supported.
        bool raytracing = false;
    };

    struct GraphicsDeviceLimits
    {
        uint32_t maxVertexAttributes;
        uint32_t maxVertexBindings;
        uint32_t maxVertexAttributeOffset;
        uint32_t maxVertexBindingStride;
        uint32_t maxTextureDimension2D;
        uint32_t maxTextureDimension3D;
        uint32_t maxTextureDimensionCube;
        uint32_t maxTextureArrayLayers;
        uint32_t maxColorAttachments;
        uint32_t maxUniformBufferSize;
        uint32_t minUniformBufferOffsetAlignment;
        uint32_t maxStorageBufferSize;
        uint32_t minStorageBufferOffsetAlignment;
        uint32_t maxSamplerAnisotropy;
        uint32_t maxViewports;
        uint32_t maxViewportWidth;
        uint32_t maxViewportHeight;
        uint32_t maxTessellationPatchSize;
        float pointSizeRangeMin;
        float pointSizeRangeMax;
        float lineWidthRangeMin;
        float lineWidthRangeMax;
        uint32_t maxComputeSharedMemorySize;
        uint32_t maxComputeWorkGroupCountX;
        uint32_t maxComputeWorkGroupCountY;
        uint32_t maxComputeWorkGroupCountZ;
        uint32_t maxComputeWorkGroupInvocations;
        uint32_t maxComputeWorkGroupSizeX;
        uint32_t maxComputeWorkGroupSizeY;
        uint32_t maxComputeWorkGroupSizeZ;
    };

    struct GraphicsDeviceCaps
    {
        GraphicsBackendType backendType;

        /// Gets the GPU device identifier.
        uint32_t deviceId;

        /// Gets the GPU vendor identifier.
        uint32_t vendorId;

        /// Gets the adapter name.
        std::string adapterName;

        /// Gets the adapter type.
        GPUAdapterType adapterType;

        GraphicsDeviceFeatures features;
        GraphicsDeviceLimits limits;
    };

    ALIMER_API const char* ToString(GraphicsBackendType value);

    // Returns true if adapter's vendor is AMD.
    ALIMER_FORCE_INLINE bool IsAMD(uint32 vendorId)
    {
        return vendorId == KnownVendorId_AMD;
    }

    // Returns true if adapter's vendor is Intel.
    ALIMER_FORCE_INLINE bool IsIntel(uint32 vendorId)
    {
        return vendorId == KnownVendorId_Intel;
    }

    // Returns true if adapter's vendor is Nvidia.
    ALIMER_FORCE_INLINE bool IsNvidia(uint32 vendorId)
    {
        return vendorId == KnownVendorId_Nvidia;
    }

    // Returns true if adapter's vendor is Microsoft.
    ALIMER_FORCE_INLINE bool IsMicrosoft(uint32 vendorId)
    {
        return vendorId == KnownVendorId_Microsoft;
    }
}
