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

#include "graphics/GraphicsResource.h"
#include "Core/Vector.h"
#include <mutex>
#include <memory>

namespace alimer
{
#ifdef _DEBUG
#define DEFAULT_ENABLE_DEBUG_LAYER true
#else
#define DEFAULT_ENABLE_DEBUG_LAYER false
#endif

    class Texture;
    using CommandList = uint8_t;

    class ALIMER_API GraphicsDeviceEvents
    {
    public:
        virtual void OnDeviceLost() = 0;
        virtual void OnDeviceRestored() = 0;

    protected:
        ~GraphicsDeviceEvents() = default;
    };

    /// Defines the logical graphics device class.
    class ALIMER_API GraphicsDevice
    {
        friend class GraphicsResource;

    public:
        /**
        * Device description
        */
        struct Desc 
        {
            BackendType preferredBackendType = BackendType::Count;
            bool enableDebugLayer = DEFAULT_ENABLE_DEBUG_LAYER;
        };

        virtual ~GraphicsDevice() = default;

        GraphicsDevice(const GraphicsDevice&) = delete;
        GraphicsDevice(GraphicsDevice&&) = delete;
        GraphicsDevice& operator=(const GraphicsDevice&) = delete;
        GraphicsDevice& operator=(GraphicsDevice&&) = delete;

        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;

        static std::unique_ptr<GraphicsDevice> Create(const Desc& desc, const PresentationParameters& presentationParameters);

        //virtual Texture* CreateTexture(const TextureDescription& desc, const void* initialData) = 0;

        const GraphicsDeviceCaps& GetCaps() const {
            return caps;
        }

        virtual TextureHandle CreateTexture(const TextureDesc& desc, const void* pData, bool autoGenerateMipmaps) = 0;
        virtual void DestroyTexture(TextureHandle handle) = 0;

    private:
        virtual bool Initialize(const PresentationParameters& presentationParameters) = 0;
        virtual void Shutdown() = 0;

        void TrackResource(GraphicsResource* resource);
        void UntrackResource(GraphicsResource* resource);

    protected:
        GraphicsDevice(const Desc& desc);
        void ReleaseTrackedResources();

        Desc desc;
        GraphicsDeviceCaps caps{};
        std::mutex trackedResourcesMutex;
        Vector<GraphicsResource*> trackedResources;
        GraphicsDeviceEvents* events = nullptr;

        template <typename T, uint32_t MAX_COUNT>
        class Pool
        {
        public:
            Pool()
            {
                values = (T*)mem;
                for (int i = 0; i < MAX_COUNT + 1; ++i) {
                    new (&values[i]) int(i + 1);
                }
                new (&values[MAX_COUNT]) int(-1);
                first_free = 1;
            }

            int alloc()
            {
                if (first_free == -1) return -1;

                const int id = first_free;
                first_free = *((int*)&values[id]);
                new (&values[id]) T;
                return id;
            }

            void dealloc(uint32_t index)
            {
                values[index].~T();
                new (&values[index]) int(first_free);
                first_free = index;
            }

            alignas(T) uint8_t mem[sizeof(T) * (MAX_COUNT + 1)] = {};
            T* values;
            int first_free;

            T& operator[](int index) { return values[index]; }
            bool isFull() const { return first_free == -1; }
        };
    };
}
