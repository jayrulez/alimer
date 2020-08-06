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

#include "Graphics/CommandContext.h"
#include "Graphics/Buffer.h"
#include <EASTL/intrusive_ptr.h>
#include <EASTL/vector.h>

namespace alimer
{
    class Window;

    /// Defines the logical graphics subsystem.
    class ALIMER_API GraphicsDevice : public Object
    {
        friend class GraphicsResource;

        ALIMER_OBJECT(GraphicsDevice, Object);

    public:
        struct Desc
        {
            BackendType preferredBackendType = BackendType::Count;
            GPUDeviceFlags flags = GPUDeviceFlags::None;
            PixelFormat colorFormat = PixelFormat::BGRA8UnormSrgb;
            PixelFormat depthStencilFormat = PixelFormat::Depth32Float;
            bool enableVSync = false;
        };

        static GraphicsDevice* Instance;
        static GraphicsDevice* Create(Window* window, const Desc& desc);

        virtual bool BeginFrame() = 0;
        virtual uint64_t EndFrame() = 0;

        /// Get the device capabilities.
        ALIMER_FORCE_INLINE const GraphicsCapabilities& GetCaps() const { return caps; }

        /// Gets the main GPU context.
        virtual CommandContext* GetMainContext() const = 0;

        /* Resource creation methods */
        virtual Buffer* CreateBuffer(const BufferDescription& desc, const void* initialData) = 0;

        /// Total number of CPU frames completed (completed means all command buffers submitted to the GPU)
        ALIMER_FORCE_INLINE uint64_t CurrentCPUFrame() const { return currentCPUFrame; }

    protected:
        GraphicsDevice() = default;

        GraphicsCapabilities caps{};
        uint64_t currentCPUFrame{ 0 };

    private:
        ALIMER_DISABLE_COPY_MOVE(GraphicsDevice);
    };
}

