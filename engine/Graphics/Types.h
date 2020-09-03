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
    static constexpr uint32 kInflightFrameCount = 2u;
    static constexpr uint32 kMaxColorAttachments = 8u;
    static constexpr uint32 kMaxVertexBufferBindings = 8u;
    static constexpr uint32 kMaxVertexAttributes = 16u;
    static constexpr uint32 kMaxVertexAttributeOffset = 2047u;
    static constexpr uint32 kMaxVertexBufferStride = 2048u;
    static constexpr uint32 kMaxViewportAndScissorRects = 8u;

    static constexpr uint32 KnownVendorId_AMD = 0x1002;
    static constexpr uint32 KnownVendorId_Intel = 0x8086;
    static constexpr uint32 KnownVendorId_Nvidia = 0x10DE;
    static constexpr uint32 KnownVendorId_Microsoft = 0x1414;
    static constexpr uint32 KnownVendorId_ARM = 0x13B5;
    static constexpr uint32 KnownVendorId_ImgTec = 0x1010;
    static constexpr uint32 KnownVendorId_Qualcomm = 0x5143;

    enum class GPUAdapterType
    {
        DiscreteGPU,
        IntegratedGPU,
        CPU,
        Unknown
    };

    /// Enum describing the rendering backend.
    enum class RendererType
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
        /// OpenGL backend.
        OpenGL,
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

    enum class GPUBufferUsage : uint32_t
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
    ALIMER_DEFINE_ENUM_FLAG_OPERATORS(GPUBufferUsage, uint32_t);

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
        DontCare,
        Load,
        Clear
    };

    /// GraphicsDevice flags.
    enum class GraphicsDeviceFlags : uint32_t
    {
        None = 0x0,
        /// Enable debug runtime.
        DebugRuntime = 0x1,
        /// Enable GPU based validation.
        GPUBasedValidation = 0x2,
        /// Enable RenderDoc integration.
        RenderDoc = 0x4,
    };
    ALIMER_DEFINE_ENUM_FLAG_OPERATORS(GraphicsDeviceFlags, uint32_t);

    /* Structs */
    struct GPUBufferDescription
    {
        GPUBufferUsage usage;
        uint32 size;
        uint32 stride;
    };

    struct GPUTextureDescription
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

        static GPUTextureDescription New2D(PixelFormat format, uint32 width, uint32 height, bool mipmapped = false, TextureUsage usage = TextureUsage::Sampled)
        {
            GPUTextureDescription desc = {};
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
        TextureUsage usage = TextureUsage::RenderTarget;
        PixelFormat colorFormat = PixelFormat::BGRA8UnormSrgb;
        /// The depth format.
        PixelFormat depthStencilFormat = PixelFormat::Depth32Float;
        uint32_t width;
        uint32_t height;
        bool verticalSync = false;
        uint32 sampleCount = 1u;
        const char* label;
    };

    struct GraphicsDeviceDescription
    {
        GraphicsDeviceFlags flags;
        GraphicsAdapterPreference adapterPreference;
        SwapChainDescription primarySwapChain;
    };


    struct GraphicsDeviceCaps
    {
        RendererType rendererType;

        /// Gets the GPU device identifier.
        uint32 deviceId;

        /// Gets the GPU vendor identifier.
        uint32 vendorId;

        /// Gets the adapter name.
        std::string adapterName;

        /// Gets the adapter type.
        GPUAdapterType adapterType;

        struct Features
        {
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

    ALIMER_API const char* ToString(RendererType value);

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
