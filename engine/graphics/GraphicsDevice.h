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

#include "core/Ptr.h"
#include "graphics/GraphicsResource.h"
#include "os/os.h"
#include <memory>
#include <set>
#include <mutex>

namespace alimer
{
    struct TextureDesc;
    struct SwapChainDesc;
    class ICommandQueue;
    class ISwapChain;
    class ITexture;

    /// Desribes a GraphicsDevice
    struct GraphicsDeviceDesc
    {
        const char* applicationName = "";
        GraphicsDeviceFlags flags = GraphicsDeviceFlags::None;
    };

    /// Defines the logical graphics device class.
    class ALIMER_API IGraphicsDevice
    {
    public:
        ALIMER_DECL_DEVICE_INTERFACE(IGraphicsDevice);

        /// Waits for the device to become idle.
        virtual void WaitForIdle() = 0;

        virtual bool BeginFrame() = 0;
        virtual void EndFrame() = 0;

        virtual ICommandQueue* GetGraphicsQueue() const = 0;
        virtual ICommandQueue* GetComputeQueue() const = 0;
        virtual ICommandQueue* GetCopyQueue() const = 0;

        virtual RefPtr<ISwapChain> CreateSwapChain(window_t* window, ICommandQueue* commandQueue, const SwapChainDesc* pDesc) = 0;
        virtual RefPtr<ITexture> CreateTexture(const TextureDesc* pDesc, const void* initialData) = 0;
    };


    /// Get set of available graphics backends.
    ALIMER_API std::set<GraphicsAPI> GetAvailableGraphicsAPI();

    /// Create new graphics device
    ALIMER_API std::unique_ptr<IGraphicsDevice> CreateGraphicsDevice(GraphicsAPI api, const GraphicsDeviceDesc& desc);
}
