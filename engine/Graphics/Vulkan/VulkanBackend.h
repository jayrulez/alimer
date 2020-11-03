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

#include "Core/Log.h"
#include "Graphics/Graphics.h"
#include "vk_mem_alloc.h"
#include "volk.h"

namespace alimer
{
    /**
     * @brief Helper function to convert a VkResult enum to a string
     * @param format Vulkan result to convert.
     * @return The string to return.
     */
    const std::string ToString(VkResult result);

    class VulkanGraphics final : public Graphics
    {
    public:
        VulkanGraphics(Window& window, const GraphicsSettings& settings);
        ~VulkanGraphics() override;

    private:
        struct InstanceFeatures
        {
            bool debugUtils                   = false;
            bool headless                     = false;
            bool getPhysicalDeviceProperties2 = false;
            bool getSurfaceCapabilities2      = false;
        } instanceFeatues;
        VkInstance instance{VK_NULL_HANDLE};
#if defined(_DEBUG)
        VkDebugUtilsMessengerEXT debug_utils_messenger{VK_NULL_HANDLE};
#endif
        VkSurfaceKHR     surface{VK_NULL_HANDLE};
        VkPhysicalDevice physical_device{VK_NULL_HANDLE};
        VkDevice         device{VK_NULL_HANDLE};
        VmaAllocator     memory_allocator{VK_NULL_HANDLE};
    };
}

/// @brief Helper macro to test the result of Vulkan calls which can return an error.
#define VK_CHECK(x)                                                   \
    do                                                                \
    {                                                                 \
        VkResult err = x;                                             \
        if (err)                                                      \
        {                                                             \
            LOGE("Detected Vulkan error: {}", alimer::ToString(err)); \
            abort();                                                  \
        }                                                             \
    } while (0)

#define VK_THROW(result, msg)                                        \
    do                                                               \
    {                                                                \
        LOGE("Vulkan: {}, error {}", msg, alimer::ToString(result)); \
        abort();                                                     \
    } while (0)