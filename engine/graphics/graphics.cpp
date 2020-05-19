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

#include "config.h"
#include "graphics/graphics_backend.h"

#if defined(ALIMER_GRAPHICS_VULKAN) 
#include "graphics/vulkan/graphics_vulkan.h"
#endif

#include <cstdarg>
#include <cstdio>

namespace alimer
{
    namespace graphics
    {
        static Renderer* s_renderer = nullptr;
        static LogCallback s_logCallback = nullptr;
        static void* s_logCallbackUserData = nullptr;

        void LogError(const char* format, ...)
        {
            if (s_logCallback) {
                char message[1024];
                va_list args;
                va_start(args, format);
                vsnprintf(message, sizeof(message), format, args);
                s_logCallback(s_logCallbackUserData, message, LogLevel::Error);
                va_end(args);
            }
        }

        void LogWarn(const char* format, ...)
        {
            if (s_logCallback) {
                char message[1024];
                va_list args;
                va_start(args, format);
                vsnprintf(message, sizeof(message), format, args);
                s_logCallback(s_logCallbackUserData, message, LogLevel::Warn);
                va_end(args);
            }
        }

        void LogInfo(const char* format, ...)
        {
            if (s_logCallback) {
                char message[1024];
                va_list args;
                va_start(args, format);
                vsnprintf(message, sizeof(message), format, args);
                s_logCallback(s_logCallbackUserData, message, LogLevel::Info);
                va_end(args);
            }
        }

        void LogDebug(const char* format, ...)
        {
            if (s_logCallback) {
                char message[1024];
                va_list args;
                va_start(args, format);
                vsnprintf(message, sizeof(message), format, args);
                s_logCallback(s_logCallbackUserData, message, LogLevel::Debug);
                va_end(args);
            }
        }

        bool Initialize(const Configuration& config)
        {
            if (s_renderer != nullptr) {
                return true;
            }

            s_logCallback = config.logCallback;
            s_logCallbackUserData = config.userData;

            s_renderer = vulkan::CreateRenderer();
            if (!s_renderer->IsSupported() || !s_renderer->Init(config)) {
                s_renderer = nullptr;
                return false;
            }

            return true;
        }

        void Shutdown()
        {
            if (s_renderer == NULL) {
                return;
            }

            s_renderer->Shutdown();
            s_renderer = NULL;
        }

        ContextHandle CreateContext(const ContextInfo& info)
        {
            return s_renderer->CreateContext(info);
        }

        void DestroyContext(ContextHandle handle)
        {
            if (handle.isValid())
            {
                s_renderer->DestroyContext(handle);
            }
        }

        bool ResizeContext(ContextHandle handle, uint32_t width, uint32_t height)
        {
            return s_renderer->ResizeContext(handle, width, height);
        }

        bool BeginFrame(ContextHandle handle)
        {
            return s_renderer->BeginFrame(handle);
        }

        void EndFrame(ContextHandle handle)
        {
            s_renderer->EndFrame(handle);
        }
    }
}
