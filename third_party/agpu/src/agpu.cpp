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

    static const Driver* drivers[] = {
    #if AGPU_DRIVER_D3D12
        &D3D12_Driver,
    #endif
    #if AGPU_DRIVER_D3D11
        &D3D11_Driver,
    #endif
    #if AGPU_DRIVER_METAL
        & metal_driver,
    #endif
    #if AGPU_DRIVER_VULKAN && defined(TODO_VK)
        &vulkan_driver,
    #endif
    #if AGPU_DRIVER_OPENGL
        &GL_Driver,
    #endif

        nullptr
    };

    static BackendType s_backend = BackendType::Count;
    static Renderer* renderer = nullptr;

    bool SetPreferredBackend(BackendType backend)
    {
        if (renderer != nullptr)
            return false;

        s_backend = backend;
        return true;
    }

    bool Init(InitFlags flags, void* windowHandle)
    {
        if (renderer) {
            return true;
        }

        if (s_backend == BackendType::Count)
        {
            for (uint32_t i = 0; AGPU_COUNT_OF(drivers); i++)
            {
                if (!drivers[i])
                    break;

                if (drivers[i]->isSupported()) {
                    renderer = drivers[i]->createRenderer();
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
                    renderer = drivers[i]->createRenderer();
                    break;
                }
            }
        }

        if (!renderer || !renderer->init(flags, windowHandle)) {
            return false;
        }

        return true;
    }

    void Shutdown(void) {
        if (renderer == nullptr)
            return;

        renderer->Shutdown();
        renderer = nullptr;
    }

    Swapchain GetPrimarySwapchain(void)
    {
        return renderer->GetPrimarySwapchain();
    }

    FrameOpResult BeginFrame(Swapchain swapchain)
    {
        AGPU_ASSERT(swapchain.isValid());

        return renderer->BeginFrame(swapchain);
    }

    FrameOpResult EndFrame(Swapchain swapchain, EndFrameFlags flags)
    {
        AGPU_ASSERT(swapchain.isValid());

        return renderer->EndFrame(swapchain, flags);
    }

    const Caps* QueryCaps(void)
    {
        AGPU_ASSERT(renderer);
        return renderer->QueryCaps();
    }

    Swapchain CreateSwapchain(void* windowHandle)
    {
        AGPU_ASSERT(windowHandle);

        return renderer->CreateSwapchain(windowHandle);
    }

    void DestroySwapchain(Swapchain handle)
    {
        if (handle.isValid())
        {
            renderer->DestroySwapchain(handle);
        }
    }

    Texture GetCurrentTexture(Swapchain handle)
    {
        AGPU_ASSERT(handle.isValid());

        return renderer->GetCurrentTexture(handle);
    }

    BufferHandle CreateBuffer(uint32_t count, uint32_t stride, const void* initialData)
    {
        if (!count || !stride)
        {
            return kInvalidBuffer;
        }

        return renderer->CreateBuffer(count, stride, initialData);
    }

    void DestroyBuffer(BufferHandle handle)
    {
        if (handle.isValid())
        {
            renderer->DestroyBuffer(handle);
        }
    }


    Texture CreateExternalTexture2D(intptr_t handle, uint32_t width, uint32_t height, PixelFormat format, bool mipmaps)
    {
        uint32_t mipLevels = mipmaps ? CalculateMipLevels(width, height, 1u) : 1u;
        return renderer->CreateTexture(width, height, format, mipLevels, handle);
    }

    void DestroyTexture(Texture handle)
    {
        if (handle.isValid())
        {
            renderer->DestroyTexture(handle);
        }
    }

    /* Commands */
    void PushDebugGroup(const char* name)
    {
        renderer->PushDebugGroup(name);
    }

    void PopDebugGroup(void)
    {
        renderer->PopDebugGroup();
    }

    void InsertDebugMarker(const char* name)
    {
        renderer->InsertDebugMarker(name);
    }

    void BeginRenderPass(const RenderPassDescription* renderPass)
    {
        AGPU_ASSERT(renderPass);

        renderer->BeginRenderPass(renderPass);
    }

    void EndRenderPass(void)
    {
        renderer->EndRenderPass();
    }

    /* Util methods */
    uint32_t CalculateMipLevels(uint32_t width, uint32_t height, uint32_t depth)
    {
        uint32_t mipLevels = 0u;
        uint32_t size = AGPU_MAX(AGPU_MAX(width, height), depth);
        while (1u << mipLevels <= size) {
            ++mipLevels;
        }

        if (1u << mipLevels < size) {
            ++mipLevels;
        }

        return mipLevels;
    }
}

