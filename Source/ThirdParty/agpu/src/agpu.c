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

#include "agpu_driver.h"

static const AGPU_Driver* drivers[] = {
#if AGPU_DRIVER_D3D11
    &D3D11Driver,
#endif
#if AGPU_DRIVER_METAL
    & MetalDriver,
#endif
#if AGPU_DRIVER_VULKAN /* TODO: Bump this to the top when Vulkan is done! */
    &VulkanDriver,
#endif
#if AGPU_DRIVER_OPENGL
    &OpenGLDriver,
#endif

    NULL
};


AGPUDevice* agpuCreateDevice(AGPUBackendType backendType) {
    AGPUDevice* device = NULL;
    if (backendType == AGPUBackendType_Force32) {
        for (uint32_t i = 0; AGPU_COUNT_OF(drivers); i++) {
            if (drivers[i]->IsSupported()) {
                device = drivers[i]->CreateDevice();
                break;
            }
        }
    }
    else {
        for (uint32_t i = 0; AGPU_COUNT_OF(drivers); i++) {
            if (drivers[i]->backendType == backendType && drivers[i]->IsSupported()) {
                device = drivers[i]->CreateDevice();
                break;
            }
        }
    }

    return device;
}

void agpuDestroyDevice(AGPUDevice* device) {
    if (device == NULL)
    {
        return;
    }

    device->DestroyDevice(device);
}
