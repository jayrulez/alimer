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

#include "core/Utils.h"
#include "math/size.h"
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
        /// Null backend.
        Null,
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

    enum class GPUPowerPreference : uint32_t
    {
        HighPerformance,
        LowPower
    };

    enum class GraphicsAdapterType : uint32_t
    {
        DiscreteGPU,
        IntegratedGPU,
        CPU,
        Unknown
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

    enum class QueueType : uint32_t
    {
        Graphics,
        Compute,
        Copy
    };

    enum class BufferUsage : uint32_t
    {
        None = 0,
        MapRead = 1 << 0,
        MapWrite = 1 << 1,
        CopySrc = 1 << 2,
        CopyDst = 1 << 3,
        Index = 1 << 4,
        Vertex = 1 << 5,
        Uniform = 1 << 6,
        Storage = 1 << 7,
        Indirect = 1 << 8,
    };
    ALIMER_DEFINE_ENUM_BITWISE_OPERATORS(BufferUsage);


    /// Defines the type of Texture.
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
        Sampled = 0x01,
        Storage = 0x02,
        RenderTarget = 0x04,
    };
    ALIMER_DEFINE_ENUM_BITWISE_OPERATORS(TextureUsage);

    enum class PresentInterval : uint32_t
    {
        /// Equivalent to setting One.
        Default,
        One,
        Two,
        Immediate
    };

    struct PlatformData
    {
        void* display;
        /*
        * Native window handle (HWND, IUnknown, ANativeWindow, NSWindow)..
        * If null headless device will be created if supported by backend.
        */
        void* windowHandle;
    };

    struct SwapChainDescriptor
    {
        /// Platform data.
        PlatformData platformData;

        usize extent;
        PixelFormat colorFormat = PixelFormat::BGRA8UNorm;
        PixelFormat depthStencilFormat = PixelFormat::Undefined;
        bool isFullScreen = false;
        TextureSampleCount sampleCount = TextureSampleCount::Count1;
        PresentInterval presentationInterval = PresentInterval::Default;
    };

    /// Describes a Graphics buffer.
    struct BufferDescriptor
    {
        const char* label = nullptr;
        BufferUsage usage;
        uint64_t size;
        /// Initial content to initialize with.
        const void* content = nullptr;
    };

    /// Describes a texture.
    struct TextureDescriptor
    {
        TextureType type = TextureType::Type2D;
        TextureUsage usage = TextureUsage::Sampled;

        usize3 extent = { 1u, 1u, 1u };
        PixelFormat format = PixelFormat::RGBA8UNorm;
        uint32_t mipLevels = 1;
        TextureSampleCount sampleCount = TextureSampleCount::Count1;
        /// Initial content to initialize with.
        const void* content = nullptr;
        /// Pointer to external texture handle
        const void* externalHandle = nullptr;
        const char* label = nullptr;
    };

    /// Describes GPUDevice features.
    struct GPUFeatures
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

    /// Describes GPUDevice limits.
    struct GPULimits
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

    ALIMER_API std::string ToString(BackendType type);
}
