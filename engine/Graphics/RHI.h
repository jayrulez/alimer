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

#include "Graphics/GraphicsBuffer.h"
#include "Graphics/Texture.h"
#include "Math/Size.h"
#include <memory>
#include <mutex>

namespace Alimer
{
    class Window;
    class SwapChain;

    enum class BeginFrameFlags : uint32_t {
        None = 0
    };
    ALIMER_DEFINE_ENUM_FLAG_OPERATORS(BeginFrameFlags, uint32_t);

    enum class EndFrameFlags : uint32_t {
        None = 0,
        SkipPresent = 1 << 0
    };
    ALIMER_DEFINE_ENUM_FLAG_OPERATORS(EndFrameFlags, uint32_t);

    class CommandContext;

    class ALIMER_API RHIDevice
    {
    public:
        enum class FrameOpResult : uint32_t {
            Success = 0,
            Error,
            SwapChainOutOfDate,
            DeviceLost
        };

        /// Destructor
        virtual ~RHIDevice() = default;

        static RHIDevice* Create(const std::string& applicationName, GraphicsBackendType preferredBackendType, GraphicsDeviceFlags flags = GraphicsDeviceFlags::None);

        /// Get whether device is lost.
        virtual bool IsDeviceLost() const = 0;

        /// Wait for GPU to finish pending operation and become idle.
        virtual void WaitForGPU() = 0;

        virtual FrameOpResult BeginFrame(SwapChain* swapChain, BeginFrameFlags flags = BeginFrameFlags::None) = 0;
        virtual FrameOpResult EndFrame(SwapChain* swapChain, EndFrameFlags flags = EndFrameFlags::None) = 0;

        virtual CommandContext* GetImmediateContext() const = 0;

        virtual SwapChain* CreateSwapChain() = 0;

        virtual GraphicsBuffer* CreateBuffer(const BufferDescription& description, const void* initialData, const char* label = nullptr) = 0;

        /// Gets the device backend type.
        GraphicsBackendType GetBackendType() const { return caps.backendType; }

        /// Get the device caps.
        const GraphicsDeviceCaps& GetCaps() const { return caps; }

    protected:
        RHIDevice() = default;

        GraphicsDeviceCaps caps{};

        /// Add a GPU object to keep track of. Called by GraphicsResource.
        //void AddGraphicsResource(GraphicsResource* resource);
        /// Remove a GPU object. Called by GraphicsResource.
        //void RemoveGraphicsResource(GraphicsResource* resource);
        /// Mutex for accessing the GPU objects vector from several threads.
        //std::mutex gpuObjectMutex;
        /// GPU objects.
        //std::vector<GraphicsResource*> gpuObjects;
    };


    /* Helper methods */
    inline uint32_t RHICalculateMipLevels(uint32_t width, uint32_t height, uint32_t depth = 1u)
    {
        uint32_t mipLevels = 0;
        uint32_t size = Max(Max(width, height), depth);
        while (1u << mipLevels <= size) {
            ++mipLevels;
        }

        if (1u << mipLevels < size) {
            ++mipLevels;
        }

        return mipLevels;
    }

    ALIMER_API const char* ToString(RHIDevice::FrameOpResult value);
}

