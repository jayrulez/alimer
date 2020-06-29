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

#include "graphics/Types.h"
#include "Core/Ptr.h"

namespace alimer
{
    class GraphicsDevice;

    /// Defines a Graphics Resource created by device.
    class GraphicsResource : public RefCounted
    {
    public:
        /// Describes GPU resource type.
        enum class Type : uint32_t
        {
            /// Resource is of unknown type.
            Unknown,
            /// Resource is a buffer.
            Buffer,
            /// One dimensional texture.
            Texture1D,
            /// Two dimensional texture.
            Texture2D,
            /// Three dimensional texture.
            Texture3D,
            /// A cubemap texture.
            TextureCube
        };

        GraphicsResource(GraphicsDevice& device, Type type, const std::string& name, MemoryUsage memoryUsage);
        virtual ~GraphicsResource();

        /// Release the GPU resource.
        virtual void Destroy() {}

        /*
        * Returns the device that created this resource.
        *
        * return - A pointer to the device that created this resource.
        */
        const GraphicsDevice& GetDevice() const;

        /**
        * Get the resource type.
        */
        Type GetType() const { return type; }

        /**
        * Set the resource name
        */
        void SetName(const std::string& newName) { name = newName; BackendSetName(); }

        /**
        * Get the resource name
        */
        const std::string& GetName() const { return name; }

        /**
        * Get the memory type.
        */
        MemoryUsage GetMemoryUsage() const { return memoryUsage; }

        /**
        * Get the size of the resource
        */
        uint64_t GetSize() const { return size; }

        bool IsAllocated() const { return handle.isValid(); }

        /**
        * Gets the GPU handle.
        */
        GpuHandle GetHandle() const { return handle; }

    private:
        virtual void BackendSetName() {}

    protected:
        GraphicsDevice& device;
        GpuHandle handle{ kInvalidId };
        Type type;
        MemoryUsage memoryUsage;
        std::string name;
        uint64_t size{ 0 };
    };
}
