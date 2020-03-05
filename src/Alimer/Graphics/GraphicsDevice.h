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
#include <EASTL/unique_ptr.h>
#include <EASTL/set.h>

namespace Alimer
{
    class Texture;
    class SwapChain;
    class GraphicsBuffer;

    class ALIMER_API GraphicsDevice
    {
    protected:
        GraphicsDevice(const GraphicsDeviceDescriptor* descriptor);

    public:
        /// Destructor.
        virtual ~GraphicsDevice() = default;

        static eastl::set<GraphicsBackend> GetAvailableBackends();

        static GraphicsDevice* Create(const GraphicsDeviceDescriptor* descriptor);

        virtual void WaitIdle() = 0;
        virtual bool BeginFrame() = 0;
        virtual void EndFrame() = 0;

        /**
        * Get the main GraphicsContext.
        * The main context is managed completely by the device. The user should just queue commands into it, the device will take care of allocation, submission and synchronization
        */
        GraphicsContext* GetMainContext() const { return mainContext.get(); }

        SwapChain* CreateSwapChain(void* nativeHandle, const SwapChainDescriptor* descriptor);

    private:
        virtual SwapChain* CreateSwapChainCore(void* nativeHandle, const SwapChainDescriptor* descriptor) = 0;

    protected:
        GraphicsDeviceFlags flags;

        /// GPU device power preference.
        GPUPowerPreference powerPreference;

        eastl::unique_ptr<GraphicsContext> mainContext;

    private:
        ALIMER_DISABLE_COPY_MOVE(GraphicsDevice);
    };
}
