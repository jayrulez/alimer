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


#include "gpu_driver.h"
#include <stdio.h>
#include <stdarg.h>

#define VGPU_MAX_LOG_MESSAGE (4096)
static agpu_log_callback s_log_function = NULL;
static void* s_log_user_data = NULL;

void agpu_set_log_callback(agpu_log_callback callback, void* user_data) {
    s_log_function = callback;
    s_log_user_data = user_data;
}

void agpu_log_error(const char* format, ...) {
    if (s_log_function) {
        va_list args;
        va_start(args, format);
        char message[VGPU_MAX_LOG_MESSAGE];
        vsnprintf(message, VGPU_MAX_LOG_MESSAGE, format, args);
        s_log_function(s_log_user_data, AGPU_LOG_LEVEL_ERROR, message);
        va_end(args);
    }
}

void agpu_log_warn(const char* format, ...) {
    if (s_log_function) {
        va_list args;
        va_start(args, format);
        char message[VGPU_MAX_LOG_MESSAGE];
        vsnprintf(message, VGPU_MAX_LOG_MESSAGE, format, args);
        s_log_function(s_log_user_data, AGPU_LOG_LEVEL_ERROR, message);
        va_end(args);
    }
}

void agpu_log_info(const char* format, ...) {
    if (s_log_function) {
        va_list args;
        va_start(args, format);
        char message[VGPU_MAX_LOG_MESSAGE];
        vsnprintf(message, VGPU_MAX_LOG_MESSAGE, format, args);
        s_log_function(s_log_user_data, AGPU_LOG_LEVEL_INFO, message);
        va_end(args);
    }
}

namespace gpu
{
    static const Driver* drivers[] = {
    #if ALIMER_ENABLE_D3D11
        &D3D11_Driver,
    #endif
    #if AGPU_DRIVER_METAL
        & metal_driver,
    #endif
    #if AGPU_DRIVER_VULKAN && defined(TODO_VK)
        &vulkan_driver,
    #endif
    #if AGPU_DRIVER_OPENGL
        & gl_driver,
    #endif

        NULL
    };

    static BackendType s_backend = BackendType::D3D11;
    static Renderer* gpu_renderer = nullptr;

    void SetPreferredBackend(BackendType backend)
    {
        if (gpu_renderer != nullptr)
            return;

        s_backend = backend;
    }

    bool Init(void* windowHandle, InitFlags initFlags)
    {
        if (gpu_renderer != nullptr)
            return true;


        if (s_backend == BackendType::Count) {
            for (uint32_t i = 0; AGPU_COUNT_OF(drivers); i++) {
                if (drivers[i]->IsSupported()) {
                    gpu_renderer = drivers[i]->CreateRenderer();
                    break;
                }
            }
        }
        else {
            for (uint32_t i = 0; AGPU_COUNT_OF(drivers); i++) {
                if (drivers[i]->type == s_backend && drivers[i]->IsSupported()) {
                    gpu_renderer = drivers[i]->CreateRenderer();
                    break;
                }
            }
        }

        if (!gpu_renderer || !gpu_renderer->Init(windowHandle, initFlags)) {
            return false;
        }

        return true;
    }

    void Shutdown()
    {
        if (gpu_renderer == nullptr)
        {
            return;
        }

        gpu_renderer->Shutdown();
        gpu_renderer = nullptr;
    }
}
