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

#if !defined(GPU_DEBUG)
#   if !defined(NDEBUG) || defined(DEBUG) || defined(_DEBUG)
#	    define GPU_DEBUG
#   endif
#endif

namespace alimer
{
    static constexpr uint32 kInflightFrameCount = 2u;
    static constexpr uint32 kMaxColorAttachments = 8u;
    static constexpr uint32 kMaxVertexBufferBindings = 8u;
    static constexpr uint32 kMaxVertexAttributes = 16u;
    static constexpr uint32 kMaxVertexAttributeOffset = 2047u;
    static constexpr uint32 kMaxVertexBufferStride = 2048u;
    static constexpr uint32 kMaxViewportAndScissorRects = 8u;
    static constexpr uint32 kInvalidHandleId = 0xFFffFFff;

    struct BufferHandle { uint32_t id; bool isValid() const { return id != kInvalidHandleId; } };

    static constexpr BufferHandle kInvalidBuffer = { kInvalidHandleId };

    using CommandList = uint8_t;
    static constexpr CommandList kMaxCommandLists = 16;

    enum class GPUAdapterType : uint32
    {
        DiscreteGPU,
        IntegratedGPU,
        CPU,
        Unknown
    };

    /// Enum describing the rendering backend.
    enum class GPUBackendType : uint32
    {
        /// Null renderer.
        Null,
        /// Direct3D 11 backend.
        D3D11,
        /// Direct3D 12 backend.
        D3D12,
        /// Metal backend.
        Metal,
        /// Vulkan backend.
        Vulkan,
        /// OpenGL backend.
        OpenGL,
        /// OpenGLES backend.
        OpenGLES,
        /// Default best platform supported backend.
        Count
    };

    enum class GPUPowerPreference : uint32_t
    {
        Default,
        LowPower,
        HighPerformance
    };

    /// Describes the texture dimension.
    enum class TextureDimension
    {
        /// A two-dimensional texture image.
        Texture2D,
        /// A three-dimensional texture image.
        Texture3D,
        /// A cube texture with six two-dimensional images.
        TextureCube
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
        Vertex = 1 << 0,
        Index = 1 << 1,
        Uniform = 1 << 2,
        Storage = 1 << 3,
        Indirect = 1 << 4,
        Dynamic = 1 << 5,
        Staging = 1 << 6,
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
        DontCare,
        Load,
        Clear
    };

    struct BufferDescription
    {
        BufferUsage usage;
        uint32_t size;
        uint32_t stride;
    };

    struct GPUTextureDescriptor
    {
        TextureDimension dimension = TextureDimension::Texture2D;
        PixelFormat format = PixelFormat::RGBA8Unorm;
        TextureUsage usage = TextureUsage::Sampled;
        uint32_t width = 1u;
        uint32_t height = 1u;
        uint32_t depth = 1u;
        uint32_t mipLevels = 1u;
        uint32_t arrayLayers = 1u;
        uint32_t sampleCount = 1u;

        static GPUTextureDescriptor New2D(uint32 width, uint32 height, PixelFormat format, bool mipmapped = false, TextureUsage usage = TextureUsage::Sampled)
        {
            GPUTextureDescriptor descriptor = {};
            descriptor.dimension = TextureDimension::Texture2D;
            descriptor.format = format;
            descriptor.usage = usage;
            descriptor.width = width;
            descriptor.height = height;
            descriptor.depth = 1u;
            descriptor.mipLevels = mipmapped ? 0u : 1u;
            descriptor.arrayLayers = 1u;
            descriptor.sampleCount = 1u;
            return descriptor;
        }
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

    struct RenderPassDepthStencilAttachment {
        Texture* texture = nullptr;
        uint32_t mipLevel = 0;
        union {
            TextureCubemapFace face = TextureCubemapFace::PositiveX;
            uint32_t layer;
            uint32_t slice;
        };
        LoadAction depthLoadAction = LoadAction::Clear;
        LoadAction stencilLoadOp = LoadAction::DontCare;
        float clearDepth = 1.0f;
        uint8_t clearStencil = 0;
    };

    struct RenderPassDescriptor
    {
        // Render area will be clipped to the actual framebuffer.
        //RectU renderArea = { UINT32_MAX, UINT32_MAX };

        RenderPassColorAttachment colorAttachments[kMaxColorAttachments];
        RenderPassDepthStencilAttachment depthStencilAttachment;
    };

    struct GPUFeatures
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

    struct GPULimits
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

    /// Describes GPUDevice capabilities.
    struct GraphicsCapabilities
    {
        GPUFeatures features;
        GPULimits limits;
    };

    struct GPUPlatformHandle
    {
#if ALIMER_PLATFORM_WINDOWS
        HINSTANCE   hinstance;
        HWND        hwnd;
#endif
    };

    struct GPUSwapChainDescriptor
    {
        GPUPlatformHandle handle;
        uint32_t width = 1u;
        uint32_t height = 1u;
        PixelFormat colorFormat = PixelFormat::BGRA8UnormSrgb;
        bool isFullscreen = false;
        uint32_t sampleCount = 1u;
    };

    struct GPUDeviceDescriptor
    {
        GPUPowerPreference powerPreference = GPUPowerPreference::Default;
        GPUSwapChainDescriptor swapChain;
    };

    ALIMER_API const char* ToString(GPUBackendType value);
}
