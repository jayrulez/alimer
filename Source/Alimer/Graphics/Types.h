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
#include "Graphics/gpu/gpu.h"

#if !defined(GPU_DEBUG)
#   if !defined(NDEBUG) || defined(DEBUG) || defined(_DEBUG)
#	    define GPU_DEBUG
#   endif
#endif

namespace Alimer
{
    static constexpr uint32 kInflightFrameCount = 2u;
    

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

    /* Structs */
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

    ALIMER_API const char* ToString(GPUBackendType value);
}
