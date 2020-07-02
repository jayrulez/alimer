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

#include "Core/Object.h"
#include "Core/Vector.h"
#include "Graphics/Texture.h"
#include "Math/Rect.h"
#include "Math/Viewport.h"
#include <set>
#include <mutex>
#include <atomic>

namespace alimer
{
    class GraphicsBuffer;

    class ALIMER_API GraphicsDeviceEvents
    {
    public:
        virtual void OnDeviceLost() = 0;
        virtual void OnDeviceRestored() = 0;

    protected:
        ~GraphicsDeviceEvents() = default;
    };

    /// Defines the logical graphics device class.
    class ALIMER_API GraphicsDevice : public Object
    {
        ALIMER_OBJECT(GraphicsDevice, Object);

        friend class GraphicsResource;

    public:
        /// Destructor
        virtual ~GraphicsDevice();

        static std::set<BackendType> GetAvailableBackends();
        static BackendType GetPreferredBackend();
        static void SetPreferredBackend(BackendType backendType);

        static RefPtr<GraphicsDevice> Create(bool enableValidationLayer, const PresentationParameters& presentationParameters);

        virtual void WaitForGPU() = 0;
        void BeginFrame();
        void EndFrame();

        void Resize(uint32_t width, uint32_t height);

        // Resource creation methods.
        virtual RefPtr<Texture> CreateTexture(const TextureDescription& desc, const void* initialData = nullptr) = 0;

        // CommandList
        virtual CommandList BeginCommandList(const char* name) = 0;
        virtual void InsertDebugMarker(CommandList commandList, const char* name) = 0;
        virtual void PushDebugGroup(CommandList commandList, const char* name) = 0;
        virtual void PopDebugGroup(CommandList commandList) = 0;

        virtual void SetScissorRect(CommandList commandList, const Rect& scissorRect) = 0;
        virtual void SetScissorRects(CommandList commandList, const Rect* scissorRects, uint32_t count) = 0;
        virtual void SetViewport(CommandList commandList, const Viewport& viewport) = 0;
        virtual void SetViewports(CommandList commandList, const Viewport* viewports, uint32_t count) = 0;
        virtual void SetBlendColor(CommandList commandList, const Color& color) = 0;

        virtual void BindBuffer(CommandList commandList, uint32_t slot, GraphicsBuffer* buffer) = 0;
        virtual void BindBufferData(CommandList commandList, uint32_t slot, const void* data, uint32_t size) = 0;

        /// Get the device capabilities.
        const GraphicsDeviceCaps& GetCaps() const { return caps; }

        /// Get the current backbuffer texture.
        Texture* GetBackbufferTexture() const;

        /// Get the depth stencil texture.
        Texture* GetDepthStencilTexture() const;

    private:
        virtual bool BackendInitialize(const PresentationParameters& presentationParameters) = 0;
        virtual void Shutdown() = 0;
        virtual bool BeginFrameImpl() { return true; }
        virtual void EndFrameImpl() = 0;

        void TrackResource(GraphicsResource* resource);
        void UntrackResource(GraphicsResource* resource);

    protected:
        GraphicsDevice(bool enableValidationLayer);
        void ReleaseTrackedResources();

        bool enableValidationLayer;
        bool headless;
        GraphicsDeviceCaps caps{};
        uint32_t backbufferWidth = 0;
        uint32_t backbufferHeight = 0;
        float dpiScale = 1.0f;

        std::mutex trackedResourcesMutex;
        Vector<GraphicsResource*> trackedResources;
        GraphicsDeviceEvents* events = nullptr;

        /// Current active frame index
        uint32_t frameIndex{ 0 };

        /// Whether a frame is active or not
        bool frameActive{ false };

        uint32_t backbufferIndex{ 0 };
        Vector<RefPtr<Texture>> backbufferTextures;
        RefPtr<Texture> depthStencilTexture;

        // Fixed size very simple thread safe ring buffer
        template <typename T, size_t capacity>
        class ThreadSafeRingBuffer
        {
        public:
            // Push an item to the end if there is free space
            //	Returns true if succesful
            //	Returns false if there is not enough space
            inline bool push_back(const T& item)
            {
                bool result = false;
                lock();
                size_t next = (head + 1) % capacity;
                if (next != tail)
                {
                    data[head] = item;
                    head = next;
                    result = true;
                }
                unlock();
                return result;
            }

            // Get an item if there are any
            //	Returns true if succesful
            //	Returns false if there are no items
            inline bool pop_front(T& item)
            {
                bool result = false;
                lock();
                if (tail != head)
                {
                    item = data[tail];
                    tail = (tail + 1) % capacity;
                    result = true;
                }
                unlock();
                return result;
            }

        private:
            void lock()
            {
                while (!try_lock()) {}
            }

            bool try_lock()
            {
                return !lck.test_and_set(std::memory_order_acquire);
            }

            void unlock()
            {
                lck.clear(std::memory_order_release);
            }

            T data[capacity];
            size_t head = 0;
            size_t tail = 0;
            std::atomic_flag lck = ATOMIC_FLAG_INIT;
        };

    private:
        ALIMER_DISABLE_COPY_MOVE(GraphicsDevice);
    };
}
