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

#include "graphics/SwapChain.h"
#include "Math/Color.h"
#include <mutex>

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

        static GraphicsDevice* Create(const Desc& desc, const SwapChainDesc& swapchainDesc);

        const GraphicsDeviceCaps& GetCaps() const { return caps; }
        SwapChain* GetMainSwapChain() const { return mainSwapChain.Get(); }

        virtual SwapChainHandle CreateSwapChain(const SwapChainDesc& desc) = 0;
        virtual void DestroySwapChain(SwapChainHandle handle) = 0;
        virtual uint32_t GetBackbufferCount(SwapChainHandle handle) = 0;
        virtual TextureHandle GetBackbufferTexture(SwapChainHandle handle, uint32_t index) = 0;
        virtual uint32_t Present(SwapChainHandle handle) = 0;

        virtual TextureHandle CreateTexture(const TextureDesc& desc, const void* pData, bool autoGenerateMipmaps) = 0;
        virtual TextureHandle CreateTexture(const TextureDesc& desc, uint64_t nativeHandle) = 0;
        virtual void DestroyTexture(TextureHandle handle) = 0;

        virtual void InsertDebugMarker(const char* name, CommandList commandList = 0) = 0;
        virtual void PushDebugGroup(const char* name, CommandList commandList = 0) = 0;
        virtual void PopDebugGroup(CommandList commandList = 0) = 0;
        //virtual void BeginRenderPass(const RenderPassDesc* desc, CommandList commandList = 0);
        virtual void BeginDefaultRenderPass(const Color& clearColor, float clearDepth = 1.0f, uint8_t clearStencil = 0, CommandList commandList = 0);
        virtual void BeginRenderPass(const RenderPassDesc& desc, CommandList commandList = 0) = 0;
        virtual void EndRenderPass(CommandList commandList = 0) = 0;
        virtual void SetBlendColor(const Color& color, CommandList commandList = 0) = 0;

    private:
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
        RefPtr<SwapChain> mainSwapChain;

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
