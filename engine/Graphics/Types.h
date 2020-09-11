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
    static constexpr uint32_t kRenderLatency = 2u;
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

    using CommandList = uint8_t;
    static constexpr CommandList kMaxCommandListCount = 16;

    enum class GPUAdapterType
    {
        DiscreteGPU,
        IntegratedGPU,
        CPU,
        Unknown
    };

    /// Enum describing the rendering backend.
    enum class GPUBackendType
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

    enum class GraphicsAdapterPreference : uint32_t
    {
        Default,
        LowPower,
        HighPerformance
    };

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

    /// Defines the usage of texture resource.
    enum class TextureUsage : uint32_t
    {
        None = 0,
        Sampled = (1 << 0),
        Storage = (1 << 1),
        RenderTarget = (1 << 2),
        GenerateMipmaps = (1 << 3)
    };
    ALIMER_DEFINE_ENUM_FLAG_OPERATORS(TextureUsage, uint32_t);

    enum class BufferUsage : uint32_t
    {
        None = 0,
        MapRead = 0x00000001,
        MapWrite = 0x00000002,
        CopySrc = 0x00000004,
        CopyDst = 0x00000008,
        Index = 0x00000010,
        Vertex = 0x00000020,
        Uniform = 0x00000040,
        Storage = 0x00000080,
        Indirect = 0x00000100,
        QueryResolve = 0x00000200,
    };
    ALIMER_DEFINE_ENUM_FLAG_OPERATORS(BufferUsage, uint32_t);

    enum class TextureCubemapFace : uint8_t {
        PositiveX = 0, //!< +x face
        NegativeX = 1, //!< -x face
        PositiveY = 2, //!< +y face
        NegativeY = 3, //!< -y face
        PositiveZ = 4, //!< +z face
        NegativeZ = 5, //!< -z face
    };

    enum class LoadAction : uint32_t
    {
        Discard,
        Load,
        Clear
    };

    /* Structs */
    struct BufferDescription
    {
        BufferUsage usage;
        uint32 size;
        uint32 stride;
    };

    struct TextureDescription
    {
        TextureType type = TextureType::Type2D;
        PixelFormat format = PixelFormat::RGBA8Unorm;
        TextureUsage usage = TextureUsage::Sampled;
        uint32 width = 1u;
        uint32 height = 1u;
        uint32 depth = 1u;
        uint32 mipLevels = 1u;
        uint32 arrayLayers = 1u;
        uint32 sampleCount = 1u;
        const char* label;

        static TextureDescription New2D(PixelFormat format, uint32 width, uint32 height, bool mipmapped = false, TextureUsage usage = TextureUsage::Sampled)
        {
            TextureDescription desc = {};
            desc.type = TextureType::Type2D;
            desc.format = format;
            desc.usage = usage;
            desc.width = width;
            desc.height = height;
            desc.depth = 1u;
            desc.mipLevels = mipmapped ? 0u : 1u;
            desc.arrayLayers = 1u;
            desc.sampleCount = 1u;
            return desc;
        }
    };

    struct SwapChainDescription
    {
        void* windowHandle;
        /// The color format.
        PixelFormat colorFormat = PixelFormat::BGRA8UnormSrgb;
        /// The depth format.
        PixelFormat depthStencilFormat = PixelFormat::Depth32Float;
        uint32_t width;
        uint32_t height;
        bool vsync = false;
        bool fullscreen = false;
    };

    class Texture;
    struct RenderPassColorAttachment
    {
        Texture* texture = nullptr;
        uint32_t mipLevel = 0;
        union {
            TextureCubemapFace face = TextureCubemapFace::PositiveX;
            uint32_t layer;
            uint32_t slice;
        };
        LoadAction loadAction = LoadAction::Clear;
        Color clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    };

    struct RenderPassDepthStencilAttachment
    {
        Texture* texture = nullptr;
        uint32_t mipLevel = 0;
        union {
            TextureCubemapFace face = TextureCubemapFace::PositiveX;
            uint32_t layer;
            uint32_t slice;
        };
        LoadAction depthLoadAction = LoadAction::Clear;
        LoadAction stencilLoadOp = LoadAction::Discard;
        float clearDepth = 1.0f;
        uint8_t clearStencil = 0;
    };

    struct RenderPassDescription
    {
        // Render area will be clipped to the actual framebuffer.
        //RectU renderArea = { UINT32_MAX, UINT32_MAX };

        RenderPassColorAttachment colorAttachments[kMaxColorAttachments];
        RenderPassDepthStencilAttachment depthStencilAttachment;
    };

    struct GraphicsDeviceDescription
    {
        bool enableDebugLayer = false;
        std::string applicationName;
        GraphicsAdapterPreference adapterPreference;
        SwapChainDescription primarySwapChain;
    };

    struct GraphicsDeviceCaps
    {
        GPUBackendType backendType;

        /// Gets the GPU device identifier.
        uint32_t deviceId;

        /// Gets the GPU vendor identifier.
        uint32_t vendorId;

        /// Gets the adapter name.
        std::string adapterName;

        /// Gets the adapter type.
        GPUAdapterType adapterType;

        struct Features
        {
            bool commandLists = false;
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

        struct Limits
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

        Features features;
        Limits limits;
    };

    ALIMER_API const char* ToString(GPUBackendType value);

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
