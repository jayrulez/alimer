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
#include "graphics/PixelFormat.h"

#ifndef GPU_DEBUG
#   if !defined(NDEBUG) || defined(DEBUG) || defined(_DEBUG)
#       define GPU_DEBUG
#   endif
#endif 

namespace alimer
{
    static constexpr uint32_t kMaxColorAttachments = 8u;
    static constexpr uint32_t kMaxVertexBufferBindings = 8u;
    static constexpr uint32_t kMaxVertexAttributes = 16u;
    static constexpr uint32_t kMaxVertexAttributeOffset = 2047u;
    static constexpr uint32_t kMaxVertexBufferStride = 2048u;

    /// Enum describing the Device backend.
    enum class GPUBackend : uint32_t
    {
        /// Null backend.
        Null,
        /// Vulkan backend.
        Vulkan,
        /// Direct3D 12 backend.
        Direct3D12,
        /// Metal backend.
        Metal,
        /// OpenGL backend.
        OpenGL,
        /// Default best platform supported backend.
        Count
    };

    enum class GPUDeviceFlags : uint32_t
    {
        None = 0,
        Headless = 0x1,
        Validation = 0x2,
        GpuBasedValidation = 0x4
    };
    ALIMER_DEFINE_ENUM_BITWISE_OPERATORS(GPUDeviceFlags);

    enum class CommandQueueType : uint32_t
    {
        Graphics,
        Compute,
        Copy
    };

    /// Defines the type of Texture
    enum class TextureType : uint32_t
    {
        /// One dimensional texture
        Type1D,
        /// Two dimensional texture
        Type2D,
        /// Three dimensional texture
        Type3D,
        /// Cube texture
        TypeCube
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

    enum class BufferUsage : uint32_t
    {
        None = 0,
        MapRead = 1 << 0,
        MapWrite = 1 << 1,
        CopySrc = 1 << 2,
        CopyDst = 1 << 3,
        Index   = 1 << 4,
        Vertex  = 1 << 5,
        Uniform = 1 << 6,
        Storage = 1 << 7,
        Indirect = 1 << 8,
    };
    ALIMER_DEFINE_ENUM_BITWISE_OPERATORS(BufferUsage);

    /// Defines the usage of Texture.
    enum class TextureUsage : uint32_t
    {
        None = 0,
        ShaderRead = 0x01,
        ShaderWrite = 0x02,
        RenderTarget = 0x04,
    };
    ALIMER_DEFINE_ENUM_BITWISE_OPERATORS(TextureUsage);

    enum class PresentMode : uint32_t
    {
        Immediate,
        Mailbox,
        Fifo
    };

    /// Describes a Graphics buffer.
    struct BufferDescriptor
    {
        const char* label = nullptr;
        BufferUsage usage;
        uint64_t size;
    };

    /// Describes a GPU Texture.
    struct TextureDescriptor
    {
        TextureType type = TextureType::Type2D;
        /// Texture format .
        PixelFormat format = PixelFormat::RGBA8UNorm;

        uint32_t width = 1u;
        uint32_t height = 1u;
        uint32_t depth = 1u;
        uint32_t arraySize = 1u;
        uint32_t mipLevels = 1u;
        TextureSampleCount  samples = TextureSampleCount::Count1;
        TextureUsage usage = TextureUsage::ShaderRead;
        //TextureLayout initialLayout = TextureLayout::General;
        std::string name;
    };

    /// Describes a SwapChain.
    struct SwapChainDescriptor
    {
        /// Native window handle.
        void* nativeWindowHandle;

        uint32_t width;
        uint32_t height;

        PixelFormat colorFormat = PixelFormat::BGRA8UNorm;
        PixelFormat depthStencilFormat = PixelFormat::Undefined;
        PresentMode presentMode = PresentMode::Fifo;
    };

    /// GraphicsDevice information.
    struct GPUDeviceInfo
    {
        GPUBackend backend;

        /// Rendering API name.
        std::string backendName;

        /// The hardware gpu device vendor name.
        std::string vendorName;

        /// The hardware gpu device vendor id.
        uint32_t vendorId;

        /// The hardware gpu device name.
        std::string deviceName;
    };

    /// Describes GPUDevice features.
    struct GPUDeviceFeatures
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
        bool    textureCompressionBC = false;
        bool    textureCompressionPVRTC = false;
        bool    textureCompressionETC2 = false;
        bool    textureCompressionASTC = false;
        bool    pipelineStatisticsQuery = false;
        /// Specifies whether 1D textures are supported.
        bool    texture1D = false;
        /// Specifies whether 3D textures are supported.
        bool    texture3D = false;
        /// Specifies whether 2D array textures are supported.
        bool    texture2DArray = false;
        /// Specifies whether cube array textures are supported.
        bool    textureCubeArray = false;
        /// Specifies whether raytracing is supported.
        bool    raytracing = false;
    };

    /// Describes GPUDevice limits.
    struct GPUDeviceLimits
    {
        uint32_t        maxVertexInputAttributes;
        uint32_t        maxVertexInputBindings;
        uint32_t        maxVertexInputAttributeOffset;
        uint32_t        maxVertexInputBindingStride;
        uint32_t        maxTextureDimension1D;
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
        uint32_t        maxViewportDimensions[2];
        uint32_t        maxTessellationPatchSize;
        float           pointSizeRange[2];
        float           lineWidthRange[2];
        uint32_t        maxComputeSharedMemorySize;
        uint32_t        maxComputeWorkGroupCount[3];
        uint32_t        maxComputeWorkGroupInvocations;
        uint32_t        maxComputeWorkGroupSize[3];
    };

    ALIMER_API std::string to_string(GPUBackend type);
} 
