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
#include "Graphics/Types.h"

namespace alimer
{
    /// Defines a Graphics Resource created by device.
    class ALIMER_API GraphicsResource : public Object
    {
        ALIMER_OBJECT(GraphicsResource, Object);

    public:
        enum class ResourceDimension
        {
            Unknown,
            Buffer,
            Texture1D = 2,
            Texture2D = 3,
            Texture3D = 4
        };

        enum class Usage
        {
            Default,
            Immutable,
            Dynamic,
            Staging
        };

        virtual ~GraphicsResource() = default;

        /// Release the GPU resource.
        virtual void Destroy() {}

        /// Set the resource name.
        void SetName(const String& newName) { name = newName; BackendSetName(); }

        /**
        * Get the resource name
        */
        const String& GetName() const { return name; }

    protected:
        GraphicsResource(ResourceDimension dimension_)
            : dimension(dimension_)
        {
        }

        virtual void BackendSetName() {}

    protected:
        String name;
        ResourceDimension dimension;
    };
}
