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

#include "Core/Window.h"
#include "Graphics/Types.h"

namespace alimer
{
    using Hash = uint64_t;

    class Hasher
    {
    public:
        explicit Hasher(Hash h_)
            : h(h_)
        {
        }

        Hasher() = default;

        template <typename T>
        inline void data(const T* data_, size_t size)
        {
            size /= sizeof(*data_);
            for (size_t i = 0; i < size; i++)
                h = (h * 0x100000001b3ull) ^ data_[i];
        }

        inline void u32(uint32_t value)
        {
            h = (h * 0x100000001b3ull) ^ value;
        }

        inline void s32(int32_t value)
        {
            u32(uint32_t(value));
        }

        inline void f32(float value)
        {
            union
            {
                float f32;
                uint32_t u32;
            } u;
            u.f32 = value;
            u32(u.u32);
        }

        inline void u64(uint64_t value)
        {
            u32(value & 0xffffffffu);
            u32(value >> 32);
        }

        template <typename T>
        inline void pointer(T* ptr)
        {
            u64(reinterpret_cast<uintptr_t>(ptr));
        }

        inline void string(const char* str)
        {
            char c;
            u32(0xff);
            while ((c = *str++) != '\0')
                u32(uint8_t(c));
        }

        inline void string(const std::string& str)
        {
            u32(0xff);
            for (auto& c : str)
                u32(uint8_t(c));
        }

        inline Hash get() const
        {
            return h;
        }

    private:
        Hash h = 0xcbf29ce484222325ull;
    };

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
            new (&values[id]) T;
            return id;
        }

        void Dealloc(uint32_t index)
        {
            values[index].~T();
            new (&values[index]) int(first_free);
            first_free = index;
        }

        alignas(T) uint8_t mem[sizeof(T) * MAX_COUNT];
        T* values;
        int first_free;

        T& operator[](int index) { return values[index]; }
        bool isFull() const { return first_free == -1; }
    };

    class GraphicsImpl 
    {
    public:
        virtual ~GraphicsImpl() = default;

        bool IsInitialized() const { return initialized; }
        const GraphicsCapabilities& GetCaps() const { return caps; }

        virtual bool Initialize(WindowHandle windowHandle, uint32_t width, uint32_t height, bool isFullscreen) = 0;
        virtual bool BeginFrame() = 0;
        virtual void EndFrame(uint64_t frameIndex) = 0;

        virtual void SetVerticalSync(bool value) { verticalSync = value; }
        bool GetVerticalSync() const { return verticalSync; }

        /* Resource creation methods */
        virtual TextureHandle CreateTexture(TextureDimension dimension, uint32_t width, uint32_t height, const void* data, void* externalHandle) = 0;
        virtual void Destroy(TextureHandle handle) = 0;
        virtual void SetName(TextureHandle handle, const char* name) = 0;

        virtual BufferHandle CreateBuffer(BufferUsage usage, uint32_t size, uint32_t stride, const void* data = nullptr) = 0;
        virtual void Destroy(BufferHandle handle) = 0;
        virtual void SetName(BufferHandle handle, const char* name) = 0;

        /* Commands */
        virtual void PushDebugGroup(const String& name, CommandList commandList) = 0;
        virtual void PopDebugGroup(CommandList commandList) = 0;
        virtual void InsertDebugMarker(const String& name, CommandList commandList) = 0;

        virtual void BeginRenderPass(CommandList commandList, uint32_t numColorAttachments, const RenderPassColorAttachment* colorAttachments, const RenderPassDepthStencilAttachment* depthStencil) = 0;
        virtual void EndRenderPass(CommandList commandList) = 0;

    protected:
        GraphicsImpl() = default;

        bool initialized = false;
        GraphicsCapabilities caps{};
        bool verticalSync = false;

        UInt2 backbufferSize = UInt2::Zero;
    };
}
