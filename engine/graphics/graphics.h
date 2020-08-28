//
// Copyright (c) 2020 Amer Koleci and contributors.
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

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#if !defined(GPU_DEFINE_ENUM_FLAG_OPERATORS)
#define GPU_DEFINE_ENUM_FLAG_OPERATORS(EnumType, UnderlyingEnumType) \
inline constexpr EnumType operator | (EnumType a, EnumType b) { return (EnumType)((UnderlyingEnumType)(a) | (UnderlyingEnumType)(b)); } \
inline constexpr EnumType& operator |= (EnumType &a, EnumType b) { return a = a | b; } \
inline constexpr EnumType operator & (EnumType a, EnumType b) { return EnumType(((UnderlyingEnumType)a) & ((UnderlyingEnumType)b)); } \
inline constexpr EnumType& operator &= (EnumType &a, EnumType b) { return a = a & b; } \
inline constexpr EnumType operator ~ (EnumType a) { return EnumType(~((UnderlyingEnumType)a)); } \
inline constexpr EnumType operator ^ (EnumType a, EnumType b) { return EnumType(((UnderlyingEnumType)a) ^ ((UnderlyingEnumType)b)); } \
inline constexpr EnumType& operator ^= (EnumType &a, EnumType b) { return a = a ^ b; } \
inline constexpr bool any(EnumType a) { return ((UnderlyingEnumType)a) != 0; }
#endif /* !defined(GPU_DEFINE_ENUM_FLAG_OPERATORS) */

namespace Alimer
{
    struct Config;

    namespace graphics
    {
        static constexpr uint32_t kMaxColorAttachments = 8u;
        static constexpr uint32_t kMaxVertexBufferBindings = 8u;
        static constexpr uint32_t kMaxVertexAttributes = 16u;
        static constexpr uint32_t kMaxVertexAttributeOffset = 2047u;
        static constexpr uint32_t kMaxVertexBufferStride = 2048u;
        static constexpr uint32_t kMaxViewportAndScissorRects = 8u;
        static constexpr uint32_t kInvalidHandleId = 0xFFffFFff;

        struct BufferHandle { uint32_t id; bool is_valid() const { return id != kInvalidHandleId; } };
        struct TextureHandle { uint32_t id; bool is_valid() const { return id != kInvalidHandleId; } };

        static constexpr BufferHandle kInvalidBuffer = { kInvalidHandleId };
        static constexpr TextureHandle kInvalidTexture = { kInvalidHandleId };

        enum class LogLevel : uint32_t
        {
            Error,
            Warn,
            Info,
            Debug,
        };

        enum class BackendType : uint32_t
        {
            Default = 0,
            D3D11,
            D3D12,
            OpenGL,
            Vulkan,
            Count
        };

        enum class PowerPreference : uint32_t
        {
            Default,
            LowPower,
            HighPerformance
        };

        /* Structs */
        struct RenderPassColorAttachment
        {
            TextureHandle texture;
            uint32_t mipLevel = 0;
            union {
                //TextureCubemapFace face = TextureCubemapFace::PositiveX;
                uint32_t layer;
                uint32_t slice;
            };
            //LoadAction loadAction = LoadAction::Clear;
            //Color clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        };

        struct RenderPassDepthStencilAttachment
        {
            TextureHandle texture;
            uint32_t mipLevel = 0;
            union {
                //TextureCubemapFace face = TextureCubemapFace::PositiveX;
                uint32_t layer;
                uint32_t slice;
            };
            //LoadAction depthLoadAction = LoadAction::Clear;
           // LoadAction stencilLoadOp = LoadAction::DontCare;
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

        BackendType get_platform_backend(void);
        bool init(const Config* config);
        void shutdown(void);
        void begin_frame(void);
        void end_frame(void);
    }
}

