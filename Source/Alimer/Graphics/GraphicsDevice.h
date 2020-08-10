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

#include "Core/Window.h"
#include "Graphics/CommandContext.h"
#include "Graphics/Buffer.h"
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
        virtual ~GraphicsDevice() = default;

        struct Desc
        {
            BackendType preferredBackendType = BackendType::Count;
            GPUDeviceFlags flags = GPUDeviceFlags::None;
            PixelFormat colorFormat = PixelFormat::BGRA8UnormSrgb;
            PixelFormat depthStencilFormat = PixelFormat::Depth32Float;
            bool enableVSync = false;
        };

        static GraphicsDevice* Create(Window* window, const Desc& desc);

        virtual bool BeginFrame() = 0;
        virtual void EndFrame() = 0;

        /// Get the device capabilities.
        ALIMER_FORCE_INLINE const GraphicsCapabilities& GetCaps() const { return caps; }

        /**
        * Get the default main command context.
        * The default render-context is managed completely by the device.
        * The user should just queue commands into it, the device will take care of allocation, submission and synchronization.
        */
        virtual CommandContext* GetMainContext() const = 0;

        /// Total number of CPU frames completed (completed means all command buffers submitted to the GPU)
        ALIMER_FORCE_INLINE uint64_t GetFrameCount() const { return frameCount; }

        /* Resource creation methods */
        virtual BufferHandle CreateBuffer(BufferUsage usage, uint32_t size, uint32_t stride, const void* data = nullptr) = 0;
        virtual void Destroy(BufferHandle handle) = 0;
        virtual void SetName(BufferHandle handle, const char* name) = 0;

        virtual SwapChainHandle CreateSwapChain(void* windowHandle, uint32_t width, uint32_t height, bool isFullscreen, bool enableVSync, PixelFormat preferredColorFormat) = 0;
        virtual void Destroy(SwapChainHandle handle) = 0;
        virtual void Present(SwapChainHandle handle) = 0;
        virtual uint32_t GetImageCount(SwapChainHandle handle) const = 0;

    protected:
        GraphicsCapabilities caps{};
        uint64_t frameCount{ 0 };

    protected:
        GraphicsDevice() = default;

    protected:
        template <typename T, uint32_t MAX_COUNT>
        class GPUResourcePool
        {
        public:
            GPUResourcePool()
            {
                values = (T*)mem;
                for (int i = 0; i < MAX_COUNT; ++i) {
                    new (&values[i]) int(i + 1);
                }
                new (&values[MAX_COUNT - 1]) int(-1);
                first_free = 0;
            }

            int Alloc()
            {
                if (first_free == -1) return -1;

                const int id = first_free;
                first_free = *((int*)&values[id]);
                new ( &values[id]) T;
                return id;
            }

            void Dealloc(uint32_t index)
            {
                values[index].~T();
                new ( &values[index]) int(first_free);
                first_free = index;
            }

            alignas(T) uint8_t mem[sizeof(T) * MAX_COUNT];
            T* values;
            int first_free;

            T& operator[](int index) { return values[index]; }
            bool isFull() const { return first_free == -1; }
        };

    private:
        ALIMER_DISABLE_COPY_MOVE(GraphicsDevice);
    };
}

