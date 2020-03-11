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

#include "VulkanGraphicsDevice.h"

namespace Alimer
{
    bool VulkanGraphicsDevice::IsAvailable()
    {
        static bool availableInitialized = false;
        static bool available = false;
        if (availableInitialized) {
            return available;
        }

        availableInitialized = true;

        VkResult result = volkInitialize();
        if (result != VK_SUCCESS)
        {
            ALIMER_LOGERROR("Failed to initialize volk.");
            return false;
        }

        available = true;
        return available;
    }

    VulkanGraphicsDevice::VulkanGraphicsDevice(const GraphicsDeviceDescriptor* descriptor)
        : GraphicsDevice(descriptor)
    {
    }

    VulkanGraphicsDevice::~VulkanGraphicsDevice()
    {
        WaitIdle();
        Destroy();
    }

    void VulkanGraphicsDevice::Destroy()
    {
        mainContext.reset();

        /*SafeDelete(graphicsQueue);
        SafeDelete(computeQueue);
        SafeDelete(copyQueue);

        // Allocator
        D3D12MA::Stats stats;
        allocator->CalculateStats(&stats);

        if (stats.Total.UsedBytes > 0) {
            ALIMER_LOGERRORF("Total device memory leaked: %llu bytes.", stats.Total.UsedBytes);
        }

        SafeRelease(allocator);*/
    }

    void VulkanGraphicsDevice::InitCapabilities()
    {
    }

    void VulkanGraphicsDevice::WaitIdle()
    {
    }

    bool VulkanGraphicsDevice::BeginFrame()
    {
        return true;
    }

    void VulkanGraphicsDevice::EndFrame()
    {

    }

    SwapChain* VulkanGraphicsDevice::CreateSwapChainCore(void* nativeHandle, const SwapChainDescriptor* descriptor)
    {
        return nullptr;
    }
}
