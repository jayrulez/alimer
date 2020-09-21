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
#include <stdio.h>
#include <stdarg.h>

namespace agpu
{
    /* Logging */
    static logCallback s_log_function = nullptr;
    static void* s_log_user_data = nullptr;

    void setLogCallback(logCallback callback, void* user_data)
    {
        s_log_function = callback;
        s_log_user_data = user_data;
    }

    void logError(const char* format, ...)
    {
        if (s_log_function) {
            va_list args;
            va_start(args, format);
            char message[kMaxLogMessageLength];
            vsnprintf(message, kMaxLogMessageLength, format, args);
            s_log_function(s_log_user_data, LogLevel::Error, message);
            va_end(args);
        }
    }

    void logWarn(const char* format, ...)
    {
        if (s_log_function) {
            va_list args;
            va_start(args, format);
            char message[kMaxLogMessageLength];
            vsnprintf(message, kMaxLogMessageLength, format, args);
            s_log_function(s_log_user_data, LogLevel::Warn, message);
            va_end(args);
        }
    }

    void logInfo(const char* format, ...)
    {
        if (s_log_function) {
            va_list args;
            va_start(args, format);
            char message[kMaxLogMessageLength];
            vsnprintf(message, kMaxLogMessageLength, format, args);
            s_log_function(s_log_user_data, LogLevel::Info, message);
            va_end(args);
        }
    }

    static const agpu_driver* drivers[] = {
    #if AGPU_DRIVER_D3D11
        & d3d11_driver,
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

        nullptr
    };

    static BackendType s_backend = BackendType::Count;
    static agpu_renderer* gpu_renderer = nullptr;

    bool setPreferredBackend(BackendType backend)
    {
        if (gpu_renderer != nullptr)
            return false;

        s_backend = backend;
        return true;
    }

    bool init(InitFlags flags, const PresentationParameters* presentationParameters)
    {
        AGPU_ASSERT(presentationParameters);

        if (gpu_renderer) {
            return true;
        }

        if (s_backend == BackendType::Count)
        {
            for (uint32_t i = 0; AGPU_COUNT_OF(drivers); i++)
            {
                if (!drivers[i])
                    break;

                if (drivers[i]->isSupported()) {
                    gpu_renderer = drivers[i]->createRenderer();
                    break;
                }
            }
        }
        else
        {
            for (uint32_t i = 0; AGPU_COUNT_OF(drivers); i++)
            {
                if (!drivers[i])
                    break;

                if (drivers[i]->backend == s_backend && drivers[i]->isSupported())
                {
                    gpu_renderer = drivers[i]->createRenderer();
                    break;
                }
            }
        }

        if (!gpu_renderer || !gpu_renderer->init(flags, presentationParameters)) {
            return false;
        }

        return true;
    }

    void shutdown(void) {
        if (gpu_renderer == nullptr)
            return;

        gpu_renderer->shutdown();
        gpu_renderer = nullptr;
    }

    void resize(uint32_t width, uint32_t height)
    {
        gpu_renderer->resize(width, height);
    }

    bool beginFrame(void)
    {
        return gpu_renderer->beginFrame();
    }

    void endFrame(void)
    {
        gpu_renderer->endFrame();
    }

    const Caps* getCaps(void)
    {
        AGPU_ASSERT(gpu_renderer);
        return gpu_renderer->queryCaps();
    }

    BufferHandle createBuffer(uint32_t count, uint32_t stride, const void* initialData)
    {
        if (!gpu_renderer || !count || !stride)
        {
            return kInvalidBuffer;
        }

        return gpu_renderer->createBuffer(count, stride, initialData);
    }

    void destroyBuffer(BufferHandle handle)
    {
        if (handle.isValid())
        {
            gpu_renderer->destroyBuffer(handle);
        }
    }
}

