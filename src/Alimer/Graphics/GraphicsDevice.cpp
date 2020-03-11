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

#include "config.h"
#include "Graphics/GraphicsDevice.h"

#if defined(ALIMER_GRAPHICS_VULKAN)
#include "Graphics/Vulkan/VulkanGraphicsDevice.h"
#endif

#if defined(ALIMER_GRAPHICS_D3D12)
#include "Graphics/D3D12/D3D12GraphicsDevice.h"
#endif

namespace Alimer
{
    GraphicsDevice::GraphicsDevice(const GraphicsDeviceDescriptor* descriptor)
        : flags(descriptor->flags)
        , powerPreference(descriptor->powerPreference)
    {

    }

    eastl::set<GraphicsBackend> GraphicsDevice::GetAvailableBackends()
    {
        static eastl::set<GraphicsBackend> availableBackends;

        if (availableBackends.empty())
        {
            availableBackends.insert(GraphicsBackend::Null);

#if defined(ALIMER_GRAPHICS_D3D12)
            if (D3D12GraphicsDevice::IsAvailable())
                availableBackends.insert(GraphicsBackend::Direct3D12);
#endif
        }

        return availableBackends;
    }

    GraphicsDevice* GraphicsDevice::Create(const GraphicsDeviceDescriptor* descriptor)
    {
        ALIMER_ASSERT(descriptor);

        GraphicsBackend backend = descriptor->preferredBackend;
        if (backend == GraphicsBackend::Default)
        {
            auto availableDrivers = GetAvailableBackends();

            if (availableDrivers.find(GraphicsBackend::Metal) != availableDrivers.end())
                backend = GraphicsBackend::Metal;
            else if (availableDrivers.find(GraphicsBackend::Vulkan) != availableDrivers.end())
                backend = GraphicsBackend::Vulkan;
            else if (availableDrivers.find(GraphicsBackend::Direct3D12) != availableDrivers.end())
                backend = GraphicsBackend::Direct3D12;
            else
                backend = GraphicsBackend::Null;
        }

        GraphicsDevice* device = nullptr;
        switch (backend)
        {

#if defined(ALIMER_GRAPHICS_D3D12)
        case GraphicsBackend::Direct3D12:
            if (D3D12GraphicsDevice::IsAvailable()) {
                device = new D3D12GraphicsDevice(descriptor);
                ALIMER_LOGINFO("Created Direct3D12 GraphicsDevice");
            }
            break;
#endif /* defined(ALIMER_GRAPHICS_D3D12) */

#if defined(ALIMER_GRAPHICS_METAL)
        case GraphicsBackend::Metal:
            if (MetalGraphicsDevice::IsAvailable()) {
                device = new MetalGraphicsDevice();
            }
            break;
#endif /* defined(ALIMER_GRAPHICS_METAL) */

        default:
            break;
        }

        return device;
    }

    SwapChain* GraphicsDevice::CreateSwapChain(void* nativeHandle, const SwapChainDescriptor* descriptor)
    {
        ALIMER_ASSERT(nativeHandle);
        ALIMER_ASSERT(descriptor);

        return CreateSwapChainCore(nativeHandle, descriptor);
    }
}
