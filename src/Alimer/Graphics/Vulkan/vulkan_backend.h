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

#include "Core/Utils.h"
#include "Diagnostics/Assert.h"
#include "Diagnostics/Log.h"
#include "Graphics/Types.h"

#include <vk_mem_alloc.h>
#include <volk.h>

namespace alimer
{
    extern eastl::string to_string(VkResult result);
}

/// Helper macro to test the result of Vulkan calls which can return an error.
#define VK_CHECK(x)                                                                 \
	do                                                                              \
	{                                                                               \
		VkResult result = x;                                                        \
		if (result != VK_SUCCESS)                                                   \
		{                                                                           \
			ALIMER_LOGE("Detected Vulkan error: %s", alimer::to_string(result));    \
			abort();                                                                \
		}                                                                           \
	} while (0)

#define VK_THROW(result, str) ALIMER_LOGE("%s : %s", str, alimer::to_string(result)); 
