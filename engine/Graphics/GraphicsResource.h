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

#include "Graphics/Types.h"
#include <string>

namespace alimer
{
    class Graphics;

    /// Defines a base graphics resource class.
    class ALIMER_API GraphicsResource 
    {
    public:
        enum class Type
        {
            Buffer,
            Texture,
            Sampler,
            RenderPipeline,
            AccelerationStructure,
        };

        virtual ~GraphicsResource() = default;

        virtual void Destroy() = 0;

        inline Type GetType() const
        {
            return type;
        }
        inline bool IsTexture() const
        {
            return type == Type::Texture;
        }
        inline bool IsBuffer() const
        {
            return type == Type::Buffer;
        }
        inline bool IsSampler() const
        {
            return type == Type::Sampler;
        }
        inline bool IsAccelerationStructure() const
        {
            return type == Type::AccelerationStructure;
        }

        /// Set the resource name.
        virtual void SetName(const String& newName)
        {
            name = newName;
        }

        /// Get the resource name
        const String& GetName() const
        {
            return name;
        }

    protected:
        GraphicsResource(Type type_);

        Type type;
        String name;
    };
}
