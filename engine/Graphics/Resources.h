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

#include "Core/Object.h"
#include "Graphics/PixelFormat.h"
#include "Math/Extent.h"
#include <string>

namespace alimer
{
    class Graphics;

    /// Defines a base graphics resource class.
    struct ALIMER_API GraphicsResource
    {
    public:
        enum class Type
        {
            Buffer,
            Texture,
            Sampler,
            SwapChain
        };

        explicit GraphicsResource(Graphics& graphics, Type type);
        virtual ~GraphicsResource() = default;

        GraphicsResource(GraphicsResource&) = delete;
        GraphicsResource& operator=(GraphicsResource&) = delete;

        GraphicsResource(GraphicsResource&&) noexcept;
        GraphicsResource& operator=(GraphicsResource&&) noexcept;

        /// Unconditionally destroy the GPU resource.
        virtual void Destroy() = 0;

        /// Get the resource type.
        Type GetType() const
        {
            return type;
        }

        /// Get the Graphics that created this resource.
        Graphics& GetGraphics() const
        {
            return *graphics;
        }

        /// Set the name.
        virtual void set_name(const std::string& newName)
        {
            name = newName;
        }

        /// Get the resource name
        const std::string& get_name() const
        {
            return name;
        }

    protected:
        Graphics*   graphics;
        Type        type;
        std::string name;
    };

    class ALIMER_API Texture final : public GraphicsResource, public Object
    {
        ALIMER_OBJECT(Texture, Object);

    public:
        Texture(Graphics& graphics);
        ~Texture() = default;

        void Destroy() override;
    };
}