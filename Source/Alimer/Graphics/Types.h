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
#include "graphics/PixelFormat.h"

namespace alimer
{
    static constexpr uint32_t kInflightFrameCount = 2u;
    static constexpr uint32_t kMaxColorAttachments = 8u;
    static constexpr uint32_t kMaxVertexBufferBindings = 8u;
    static constexpr uint32_t kMaxVertexAttributes = 16u;
    static constexpr uint32_t kMaxVertexAttributeOffset = 2047u;
    static constexpr uint32_t kMaxVertexBufferStride = 2048u;
    static constexpr uint32_t kMaxViewportAndScissorRects = 8u;

    enum class GPUKnownVendorId : uint32_t {
        None = 0,
        AMD = 0x1002,
        Intel = 0x8086,
        Nvidia = 0x10DE,
        ARM = 0x13B5,
        ImgTec = 0x1010,
        Qualcomm = 0x5143
    };

    enum class GPUAdapterType : uint32_t {
        DiscreteGPU,
        IntegratedGPU,
        CPU,
        Unknown
    };

    enum class GPUFlags : uint32_t
    {
        None = 0,
        DebugRuntime = (1 << 0),
        GPUBaseValidation = (1 << 1),
        RenderDoc = (1 << 2),
    };
    ALIMER_DEFINE_ENUM_FLAG_OPERATORS(GPUFlags, uint32_t);

    enum class MemoryUsage : uint32_t
    {
        GpuOnly,
        CpuOnly,
        GpuToCpu
    };

    enum class CommandQueueType
    {
        Graphics,
        Compute,
        Copy
    };

    /// Describes texture type.
    enum class TextureType : uint32_t
    {
        /// Two dimensional texture.
        Texture2D,
        /// Three dimensional texture.
        Texture3D,
        /// A cubemap texture.
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

    enum class LoadAction : uint32 {
        Clear,
        Load,
        Discard
    };


    enum class PresentMode : uint32 {
        Immediate,
        Mailbox,
        Fifo
    };

    struct TextureDescription
    {
        String name;
        TextureType type = TextureType::Texture2D;
        PixelFormat format = PixelFormat::RGBA8Unorm;
        TextureUsage usage = TextureUsage::Sampled;
        uint32_t width = 1u;
        uint32_t height = 1u;
        uint32_t depth = 1u;
        uint32_t arraySize = 1u;
        uint32_t mipLevels = 1u;
        uint32_t sampleCount = 1u;
    };

    /// Describes a buffer.
    struct BufferDescription
    {
        String name;
        BufferUsage usage;
        uint32_t size;
        uint32_t stride = 0;
        MemoryUsage memoryUsage = MemoryUsage::GpuOnly;
    };

    /// Describes a Swapchain
    struct SwapchainDescription
    {
        void* windowHandle;
        uint32 width;
        uint32 height;
        PixelFormat preferredColorFormat = PixelFormat::BGRA8Unorm;
        PixelFormat preferredDepthStencilFormat = PixelFormat::Depth32Float;
        PresentMode presentMode = PresentMode::Fifo;
        bool isFullscreen = false;
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
}
