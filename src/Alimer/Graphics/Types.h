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
#include "Graphics/PixelFormat.h"

namespace alimer
{
    /// Enum describing the GraphicsDevice backend.
    enum class GraphicsBackend : uint32_t {
        /// Default best platform supported backend.
        Default,
        /// Null backend.
        Null,
        /// Vulkan backend.
        Vulkan,
        /// Direct3D 12 backend.
        Direct3D12,
        /// Metal backend.
        Metal
    };

    enum class GPUPowerPreference : uint32_t
    {
        DontCare,
        LowPower,
        HighPerformance
    };

    enum class GraphicsDeviceFlags : uint32_t
    {
        None = 0x0,
        /// Enable debug runtime.
        DebugRuntime = 0x1,
        /// Enable GPU based validation.
        GPUBasedValidation = 0x2,
        /// Enable RenderDoc integration.
        RenderDoc = 0x4
    };
    ALIMER_DEFINE_ENUM_BITWISE_OPERATORS(GraphicsDeviceFlags);

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

    /// Defines the usage of Texture.
    enum class TextureUsage : uint32_t
    {
        None = 0,
        ShaderRead = 0x01,
        ShaderWrite = 0x02,
        RenderTarget = 0x04,
    };
    ALIMER_DEFINE_ENUM_BITWISE_OPERATORS(TextureUsage);

    /// Describes a Graphics buffer.
    struct BufferDescriptor
    {
        uint64_t size;
        //BufferUsage usage;
        eastl::string name;
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
        eastl::string name;
    };

    /// Describes a SwapChain.
    struct SwapChainDescriptor
    {
        uint32_t width;
        uint32_t height;
        bool vsync = true;
        PixelFormat colorFormat = PixelFormat::BGRA8UNorm;
        PixelFormat depthStencilFormat = PixelFormat::Undefined;
    };
} 
