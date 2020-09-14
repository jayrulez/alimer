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

namespace Alimer
{
    class GraphicsDevice;

    /// Defines a GPU Resource created by device.
    class ALIMER_API GraphicsResource : public Object
    {
        ALIMER_OBJECT(GraphicsResource, Object);

    public:
        enum class Type
        {
            Buffer,
            Texture,
            Sampler,
            SwapChain,
        };

        virtual ~GraphicsResource();

        /// Release the GPU resource.
        virtual void Destroy() = 0;

        inline uint64 GetID() { return id; }

        /// Set the resource name.
        void SetName(const std::string& newName);

        /// Get the resource name
        const std::string& GetName() const { return name; }
        /// Return name id of the resource.
        const StringId32& GetNameId() const { return nameId; }

    protected:
        GraphicsResource(Type type);

        virtual void BackendSetName() {}

    protected:
        Type type;

        std::string name;
        StringId32 nameId;
        uint64 id;

    private:
        static uint64 s_objectID;
    };
}
