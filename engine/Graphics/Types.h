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

#include "Core/Utils.h"
#include "Math/Color.h"
#include "graphics/PixelFormat.h"

namespace alimer
{
    static constexpr uint32_t kMaxColorAttachments = 8u;
    static constexpr uint32_t kMaxVertexBufferBindings = 8u;
    static constexpr uint32_t kMaxVertexAttributes = 16u;
    static constexpr uint32_t kMaxVertexAttributeOffset = 2047u;
    static constexpr uint32_t kMaxVertexBufferStride = 2048u;

    /// Enum describing the Device backend.
    enum class BackendType : uint32_t
    {
        /// Vulkan backend.
        Vulkan,
        /// Direct3D 12 backend.
        Direct3D12,
        /// Direct3D 11.1+ backend.
        Direct3D11,
        /// Metal backend.
        Metal,
        /// OpenGL backend.
        OpenGL,
        /// Null renderer.
        Null,
        /// Default best platform supported backend.
        Count
    };

    enum class GPUVendorId : uint32_t
    {
        None = 0,
        AMD = 0x1002,
        Intel = 0x8086,
        Nvidia = 0x10DE,
        ARM = 0x13B5,
        ImgTec = 0x1010,
        Qualcomm = 0x5143
    };

    enum class GPUAdapterType : uint32_t
    {
        DiscreteGPU,
        IntegratedGPU,
        CPU,
        Unknown
    };

    enum class GPUPowerPreference : uint32_t
    {
        Default,
        LowPower,
        HighPerformance,
    };

    enum class CommandQueueType : uint8_t
    {
        Graphics,
        Compute,
        Copy
    };

    enum class GraphicsResourceUsage : uint32_t
    {
        Default,
        Immutable,
        Dynamic,
        Staging
    };

    enum class TextureSampleCount : uint32_t
    {
        Count1 = 1,
        Count2 = 2,
        Count4 = 4,
        Count8 = 8,
        Count16 = 16,
        Count32 = 32,
    };

    enum class TextureType : uint32_t
    {
        /// Two dimensional texture
        Type2D,
        /// Three dimensional texture
        Type3D,
        /// Cube texture
        TypeCube
    };

    /// Defines the usage of Texture.
    enum class TextureUsage : uint32_t
    {
        None = 0,
        Sampled = (1 << 0),
        Storage = (1 << 1),
        RenderTarget = (1 << 2)
    };
    ALIMER_DEFINE_ENUM_BITWISE_OPERATORS(TextureUsage);

    enum class TextureCubemapFace : uint8_t {
        PositiveX = 0, //!< +x face
        NegativeX = 1, //!< -x face
        PositiveY = 2, //!< +y face
        NegativeY = 3, //!< -y face
        PositiveZ = 4, //!< +z face
        NegativeZ = 5, //!< -z face
    };

    enum class LoadAction : uint32_t {
        DontCare,
        Load,
        Clear
    };

    enum class StoreAction : uint32_t {
        Store,
        Clear,
    };

    /* Structures */
    struct GraphicsDeviceDescriptor {
        GPUPowerPreference powerPreference = GPUPowerPreference::Default;
    };

    /// Describes GraphicsDevice capabilities.
    struct GraphicsDeviceCaps
    {
        BackendType backendType;
        uint32_t vendorId;
        uint32_t deviceId;
        GPUAdapterType adapterType = GPUAdapterType::Unknown;
        std::string adapterName;

        struct Features
        {
            bool    independentBlend = false;
            bool    computeShader = false;
            bool    geometryShader = false;
            bool    tessellationShader = false;
            bool    logicOp = false;
            bool    multiViewport = false;
            bool    fullDrawIndexUint32 = false;
            bool    multiDrawIndirect = false;
            bool    fillModeNonSolid = false;
            bool    samplerAnisotropy = false;
            bool    textureCompressionETC2 = false;
            bool    textureCompressionASTC_LDR = false;
            bool    textureCompressionBC = false;
            /// Specifies whether cube array textures are supported.
            bool    textureCubeArray = false;
            /// Specifies whether raytracing is supported.
            bool    raytracing = false;
        };

        struct Limits
        {
            uint32_t        maxVertexAttributes;
            uint32_t        maxVertexBindings;
            uint32_t        maxVertexAttributeOffset;
            uint32_t        maxVertexBindingStride;
            uint32_t        maxTextureDimension2D;
            uint32_t        maxTextureDimension3D;
            uint32_t        maxTextureDimensionCube;
            uint32_t        maxTextureArrayLayers;
            uint32_t        maxColorAttachments;
            uint32_t        maxUniformBufferSize;
            uint64_t        minUniformBufferOffsetAlignment;
            uint32_t        maxStorageBufferSize;
            uint64_t        minStorageBufferOffsetAlignment;
            uint32_t        maxSamplerAnisotropy;
            uint32_t        maxViewports;
            uint32_t        maxViewportWidth;
            uint32_t        maxViewportHeight;
            uint32_t        maxTessellationPatchSize;
            float           pointSizeRangeMin;
            float           pointSizeRangeMax;
            float           lineWidthRangeMin;
            float           lineWidthRangeMax;
            uint32_t        maxComputeSharedMemorySize;
            uint32_t        maxComputeWorkGroupCountX;
            uint32_t        maxComputeWorkGroupCountY;
            uint32_t        maxComputeWorkGroupCountZ;
            uint32_t        maxComputeWorkGroupInvocations;
            uint32_t        maxComputeWorkGroupSizeX;
            uint32_t        maxComputeWorkGroupSizeY;
            uint32_t        maxComputeWorkGroupSizeZ;
        };

        Features features;
        Limits limits;
    };

    struct PresentationParameters
    {
        uint32_t backBufferWidth;
        uint32_t backBufferHeight;
        PixelFormat colorFormat = PixelFormat::BGRA8UNormSrgb;
        PixelFormat depthStencilFormat = PixelFormat::Depth32Float;
        bool isFullscreen;
        uintptr_t deviceWindowHandle; /* Null for headless mode. */
    };

    struct TextureDescription
    {
        TextureType type = TextureType::Type2D;
        PixelFormat format = PixelFormat::RGBA8UNorm;
        TextureUsage usage = TextureUsage::Sampled;
        u32 width = 1;
        u32 height = 1;
        u32 depth = 1;
        u32 mipLevelCount = 1;
        TextureSampleCount sampleCount = TextureSampleCount::Count1;

        const char* label = nullptr;
    };

    class Texture;
    struct RenderPassColorAttachmentDescriptor
    {
        Texture* texture;
        uint32_t mipLevel = 0;
        union {
            TextureCubemapFace face = TextureCubemapFace::PositiveX;
            uint32_t layer;
            uint32_t slice;
        };
        LoadAction loadAction = LoadAction::Clear;
        StoreAction storeOp = StoreAction::Store;
        Color clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    };

    struct RenderPassDepthStencilAttachmentDescriptor {
        Texture* texture;
        uint32_t mipLevel = 0;
        union {
            TextureCubemapFace face = TextureCubemapFace::PositiveX;
            uint32_t layer;
            uint32_t slice;
        };
        LoadAction depthLoadAction = LoadAction::Clear;
        StoreAction depthStoreOp = StoreAction::Store;
        LoadAction stencilLoadOp = LoadAction::DontCare;
        StoreAction stencilStoreOp = StoreAction::Clear;
        float clearDepth = 1.0f;
        u8 clearStencil;
    };

    struct RenderPassDescriptor
    {
        RenderPassColorAttachmentDescriptor colorAttachments[kMaxColorAttachments];
        RenderPassDepthStencilAttachmentDescriptor depthStencilAttachment;
    };
}
