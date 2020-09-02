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


#include "core/Assert.h"
#include "Graphics/Texture.h"
#include "gpu_driver.h"
#include "Platform/Application.h"

namespace gpu
{
    static const agpu_driver* drivers[] = {
       #if defined(ALIMER_ENABLE_D3D12)
           &d3d12_driver,
       #endif
       #if defined(ALIMER_ENABLE_D3D11)
           &d3d11_driver,
       #endif
       #if AGPU_DRIVER_METAL
           & metal_driver,
       #endif
       #if AGPU_DRIVER_VULKAN && defined(TODO_VK)
           &vulkan_driver,
       #endif
       #if defined(ALIMER_ENABLE_OPENGL)
           &gl_driver,
       #endif

           NULL
    };


    agpu_api agpu_get_platform_api(void)
    {
        for (uint32_t i = 0; AGPU_COUNT_OF(drivers); i++)
        {
            if (!drivers[i])
                break;

            if (drivers[i]->supported()) {
                return drivers[i]->api;
            }
        }

        return AGPU_API_DEFAULT;
    }

    bool agpu_init(agpu_api preferred_api, const agpu_config* config) {

    }

    void agpu_shutdown(void) {

    }

    void agpu_begin_frame(void) {

    }

    void agpu_end_frame(void) {

    }
}
