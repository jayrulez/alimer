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

#include "Core/Ptr.h"
#include "Core/Window.h"
#include "Graphics/Types.h"
#include <EASTL/string.h>
#include <EASTL/intrusive_ptr.h>
#include <EASTL/vector.h>

namespace alimer
{
    static constexpr uint32_t kMaxCommandLists = 16u;
    using CommandList = uint16_t;

    /* Forward declaration */
    class GPUDevice;

    /* Classes */
    class ALIMER_API GPUResource : public RefCounted
    {
    public:
        enum class Type
        {
            Unknown,
            Buffer,
            Texture
        };

        /// Get the type.
        ALIMER_FORCE_INLINE Type GetType() const { return type; }

    protected:
        GPUResource(Type type_)
            : type(type_)
        {
        }

        Type type;
    };

    class ALIMER_API GPUTexture : public GPUResource
    {
    public:
        using Parent = GPUResource;

        struct Desc
        {
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

        /// Get the description of the texture.
        ALIMER_FORCE_INLINE const Desc& GetDesc() const { return desc; }

    protected:
        GPUTexture(const Desc& desc_)
            : Parent(Type::Texture)
            , desc(desc_)
        {
        }

        Desc desc;
    };

    class ALIMER_API GPUBuffer : public GPUResource
    {
    public:
        using Parent = GPUResource;

        struct Desc
        {

        };

        /// Get the description of the buffer.
        ALIMER_FORCE_INLINE const Desc& GetDesc() const { return desc; }

    protected:
        GPUBuffer(const Desc& desc_)
            : Parent(Type::Buffer)
            , desc(desc_)
        {
        }

        Desc desc;
    };

    class ALIMER_API GPUContext : public RefCounted
    {

    };

    class ALIMER_API GPUDevice : public RefCounted
    {
    public:
        
        virtual bool BeginFrame() = 0;
        virtual void EndFrame() = 0;


        /// Gets the main GPU context.
        virtual GPUContext* GetMainContext() const = 0;

    };

    class GPU final
    {
    public:
        static GPUDevice* Instance;

        static void EnableBackendValidation(bool enableBackendValidation);
        static bool IsBackendValidationEnabled();
        static void EnableGPUBasedBackendValidation(bool enableGPUBasedBackendValidation);
        static bool IsGPUBasedBackendValidationEnabled();
    };
}
