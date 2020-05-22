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

#include <foundation/platform.h>
#include "core/Utils.h"
#include "graphics/PixelFormat.h"
#include "math/Color.h"

namespace alimer
{
    namespace graphics
    {
        /* Handles */
        static constexpr uint32_t kInvalidHandle = 0xFFffFFff;

        struct ContextHandle { uint32_t value; bool isValid() const { return value != kInvalidHandle; } };
        struct TextureHandle { uint32_t value; bool isValid() const { return value != kInvalidHandle; } };
        struct BufferHandle { uint32_t value; bool isValid() const { return value != kInvalidHandle; } };
        struct RenderPassHandle { uint32_t value; bool isValid() const { return value != kInvalidHandle; } };

        static constexpr ContextHandle kInvalidContext = { kInvalidHandle };
        static constexpr TextureHandle kInvalidTexture = { kInvalidHandle };
        static constexpr BufferHandle kInvalidBuffer = { kInvalidHandle };
        static constexpr RenderPassHandle kInvalidRenderPass = { kInvalidHandle };

        /* Enums */
        enum class LogLevel : uint32_t {
            Error,
            Warn,
            Info,
            Debug
        };

        enum BackendType : uint32_t {
            Null,
            Direct3D11,
            Direct3D12,
            Metal,
            Vulkan,
            OpenGL,
            OpenGLES,
            Count
        };

        enum class TextureState : uint32_t
        {
            Undefined,
            General,
            RenderTarget,
            DepthStencil,
            DepthStencilReadOnly,
            ShaderRead,
            ShaderWrite,
            CopyDest,
            CopySource,
            Present
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
            OutputAttachment = (1 << 2)
        };
        ALIMER_DEFINE_ENUM_BITWISE_OPERATORS(TextureUsage);

        enum class TextureSampleCount : uint32_t
        {
            Count1 = 1,
            Count2 = 2,
            Count4 = 4,
            Count8 = 8,
            Count16 = 16,
            Count32 = 32,
        };

        typedef void (*LogCallback)(void* userData, const char* message, LogLevel level);
        typedef void* (*GetProcAddressCallback)(const char* functionName);

        /* Structs */
        struct Configuration {
            BackendType backendType = BackendType::Count;
            bool debug;
            LogCallback logCallback;
            GetProcAddressCallback getProcAddress;
            void* userData;
        };

        struct ContextInfo {
            uintptr_t handle;
            uint32_t width;
            uint32_t height;
        };

        struct TextureInfo {
            TextureType type = TextureType::Type2D;
            PixelFormat format = PixelFormat::RGBA8UNorm;
            TextureUsage usage = TextureUsage::Sampled;
            uint32_t width = 1;
            uint32_t height = 1;
            uint32_t depth = 1;
            uint32_t mipLevels = 1;
            TextureSampleCount sampleCount = TextureSampleCount::Count1;

            const char* label = nullptr;
            const void* externalHandle;
        };

        struct RenderPassInfo {
            const char* label = nullptr;
        };

        bool Initialize(const Configuration& config);
        void Shutdown();

        /* Context */
        ContextHandle CreateContext(const ContextInfo& info);
        void DestroyContext(ContextHandle handle);
        bool ResizeContext(ContextHandle handle, uint32_t width, uint32_t height);
        bool BeginFrame(ContextHandle handle);
        void BeginRenderPass(ContextHandle handle, const Color& clearColor, float clearDepth, uint8_t clearStencil);
        void EndRenderPass(ContextHandle handle);
        void EndFrame(ContextHandle handle);

        /* Texture */
        TextureHandle CreateTexture(const TextureInfo& info);
        void DestroyTexture(TextureHandle handle);

        /* RenderPass */
        RenderPassHandle CreateRenderPass(const RenderPassInfo& info);
        void DestroyRenderPass(RenderPassHandle handle);
    }
}
