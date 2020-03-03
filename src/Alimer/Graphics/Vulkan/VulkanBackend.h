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

#include "Diagnostics/Assert.h"
#include "Diagnostics/Log.h"
#include "Graphics/Types.h"

#include <vk_mem_alloc.h>
#include <volk.h>

namespace Alimer
{
    static inline eastl::string ResultToString(VkResult result)
    {
        if (result == VK_SUCCESS)
        {
            return "VK_SUCCESS";
        }
        if (result == VK_NOT_READY)
        {
            return "VK_NOT_READY";
        }
        if (result == VK_TIMEOUT)
        {
            return "VK_TIMEOUT";
        }
        if (result == VK_EVENT_SET)
        {
            return "VK_EVENT_SET";
        }
        if (result == VK_EVENT_RESET)
        {
            return "VK_EVENT_RESET";
        }
        if (result == VK_INCOMPLETE)
        {
            return "VK_INCOMPLETE";
        }
        if (result == VK_ERROR_OUT_OF_HOST_MEMORY)
        {
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        }
        if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY)
        {
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        }
        if (result == VK_ERROR_INITIALIZATION_FAILED)
        {
            return "VK_ERROR_INITIALIZATION_FAILED";
        }
        if (result == VK_ERROR_DEVICE_LOST)
        {
            return "VK_ERROR_DEVICE_LOST";
        }
        if (result == VK_ERROR_MEMORY_MAP_FAILED)
        {
            return "VK_ERROR_MEMORY_MAP_FAILED";
        }
        if (result == VK_ERROR_LAYER_NOT_PRESENT)
        {
            return "VK_ERROR_LAYER_NOT_PRESENT";
        }
        if (result == VK_ERROR_EXTENSION_NOT_PRESENT)
        {
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        }
        if (result == VK_ERROR_FEATURE_NOT_PRESENT)
        {
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        }
        if (result == VK_ERROR_INCOMPATIBLE_DRIVER)
        {
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        }
        if (result == VK_ERROR_TOO_MANY_OBJECTS)
        {
            return "VK_ERROR_TOO_MANY_OBJECTS";
        }
        if (result == VK_ERROR_FORMAT_NOT_SUPPORTED)
        {
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        }
        if (result == VK_ERROR_FRAGMENTED_POOL)
        {
            return "VK_ERROR_FRAGMENTED_POOL";
        }
        if (result == VK_ERROR_OUT_OF_POOL_MEMORY)
        {
            return "VK_ERROR_OUT_OF_POOL_MEMORY";
        }
        if (result == VK_ERROR_INVALID_EXTERNAL_HANDLE)
        {
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        }
        if (result == VK_ERROR_SURFACE_LOST_KHR)
        {
            return "VK_ERROR_SURFACE_LOST_KHR";
        }
        if (result == VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)
        {
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        }
        if (result == VK_SUBOPTIMAL_KHR)
        {
            return "VK_SUBOPTIMAL_KHR";
        }
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            return "VK_ERROR_OUT_OF_DATE_KHR";
        }
        if (result == VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)
        {
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        }
        if (result == VK_ERROR_VALIDATION_FAILED_EXT)
        {
            return "VK_ERROR_VALIDATION_FAILED_EXT";
        }
        if (result == VK_ERROR_INVALID_SHADER_NV)
        {
            return "VK_ERROR_INVALID_SHADER_NV";
        }
        if (result == VK_ERROR_NOT_PERMITTED_EXT)
        {
            return "VK_ERROR_NOT_PERMITTED_EXT";
        }
        return "UNKNOWN";
    }
}

/// Helper macro to test the result of Vulkan calls which can return an error.
#define VK_CHECK(x)                                                             \
	do                                                                          \
	{                                                                           \
		VkResult err = x;                                                       \
		if (err)                                                                \
		{                                                                       \
			ALIMER_LOGERRORF("Detected Vulkan error: %s", Alimer::ResultToString(err));     \
			abort();                                                            \
		}                                                                       \
	} while (0)
