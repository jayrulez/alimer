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

namespace alimer
{
    static constexpr uint32_t kMaxColorAttachments = 8u;
    static constexpr uint32_t kMaxVertexBufferBindings = 8u;
    static constexpr uint32_t kMaxVertexAttributes = 16u;
    static constexpr uint32_t kMaxVertexAttributeOffset = 2047u;
    static constexpr uint32_t kMaxVertexBufferStride = 2048u;

    static constexpr uint32_t kMaxSwapchains = 16u;
    static constexpr uint32_t kMaxTextures = 4096u;
    static constexpr uint32_t kMaxBuffers = 4096u;

    /* Known vendor ids */
    static constexpr uint32_t kVendorId_AMD = 0x1002;
    static constexpr uint32_t kVendorId_ARM = 0x13B5;
    static constexpr uint32_t kVendorId_ImgTec = 0x1010;
    static constexpr uint32_t kVendorId_Intel = 0x8086;
    static constexpr uint32_t kVendorId_Nvidia = 0x10DE;
    static constexpr uint32_t kVendorId_Qualcomm = 0x5143;

    static constexpr uint32_t kInvalidHandle = 0xffFFffFF;
    struct GpuSwapchain { uint32_t id; bool isValid() const { return id != kInvalidHandle; } };
    struct GPUTexture { uint32_t id; bool isValid() const { return id != kInvalidHandle; } };
    struct GPUBuffer { uint32_t id; bool isValid() const { return id != kInvalidHandle; } };

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

    enum class GraphicsProviderFlags : uint32_t
    {
        None = 0,
        Headless = 0x1,
        Validation = 0x2,
        GPUBasedValidation = 0x4
    };
    ALIMER_DEFINE_ENUM_BITWISE_OPERATORS(GraphicsProviderFlags);

    enum class GraphicsAdapterType : uint32_t
    {
        DiscreteGPU,
        IntegratedGPU,
        CPU,
        Unknown
    };

    enum class QueueType : uint32_t
    {
        Graphics,
        Compute,
        Copy
    };

    enum class GPUPowerPreference : uint32_t
    {
        DontCare,
        LowPower,
        HighPerformance,
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
        Sampled = 0x01,
        Storage = 0x02,
        OutputAttachment = 0x04,
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

    struct GraphicsDeviceCaps
    {
        /// The backend type.
        BackendType backendType;

        /// Selected GPU vendor PCI id.
        uint32_t vendorId = 0;
        uint32_t deviceId = 0;
        GraphicsAdapterType adapterType = GraphicsAdapterType::Unknown;
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

        Features features;
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

    inline bool IsAMD(uint32_t vendorId) { return vendorId == kVendorId_AMD; }
    inline bool IsARM(uint32_t vendorId) { return vendorId == kVendorId_ARM; }
    inline bool IsImgTec(uint32_t vendorId) { return vendorId == kVendorId_ImgTec; }
    inline bool IsIntel(uint32_t vendorId) { return vendorId == kVendorId_Intel; }
    inline bool IsNvidia(uint32_t vendorId) { return vendorId == kVendorId_Nvidia; }
    inline bool IsQualcomm(uint32_t vendorId) { return vendorId == kVendorId_Qualcomm; }

    ALIMER_API std::string ToString(BackendType type);
} 
