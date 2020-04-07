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

#include "VulkanGraphicsAdapter.h"
#include "VulkanGraphicsProvider.h"
#include "VulkanGraphicsDevice.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "math/math.h"

using namespace std;

namespace alimer
{
    VulkanGraphicsAdapter::VulkanGraphicsAdapter(VulkanGraphicsProvider* provider_, VkPhysicalDevice handle_)
        : GraphicsAdapter(provider_, BackendType::Vulkan)
        , handle(handle_)
    {
        vkGetPhysicalDeviceProperties(handle, &properties);

        vendorId = properties.vendorID;
        deviceId = properties.deviceID;
        switch (properties.deviceType)
        {
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            adapterType = GraphicsAdapterType::Unknown;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            adapterType = GraphicsAdapterType::IntegratedGPU;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            adapterType = GraphicsAdapterType::DiscreteGPU;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            adapterType = GraphicsAdapterType::CPU;
            break;

        default:
            adapterType = GraphicsAdapterType::Unknown;
            break;
        }
        name = properties.deviceName;
    }

    SharedPtr<GraphicsDevice> VulkanGraphicsAdapter::CreateDevice(GraphicsSurface* surface)
    {
        return SharedPtr<GraphicsDevice>(new VulkanGraphicsDevice(this, surface));
    }
}
