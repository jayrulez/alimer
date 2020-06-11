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

#include "Application/Application.h"
#include "Application/Window.h"
#include "Core/Engine.h"
#include "graphics/GraphicsProvider.h"
#include "graphics/GraphicsDevice.h"
#include "graphics/CommandQueue.h"
#include "graphics/CommandBuffer.h"
#include "graphics/SwapChain.h"
#include "Input/InputManager.h"
#include "Core/Log.h"

namespace alimer
{
    static void vgpu_log_callback(void* user_data, vgpu_log_level level, const char* message) {
        switch (level)
        {
        case VGPU_LOG_LEVEL_ERROR:
            LOG_ERROR(message);
            break;

        case VGPU_LOG_LEVEL_WARN:
            LOG_WARN(message);
            break;

        case VGPU_LOG_LEVEL_INFO:
            LOG_INFO(message);
            break;
        case VGPU_LOG_LEVEL_DEBUG:
            LOG_DEBUG(message);
            break;
        default:
            break;
        }
    }

    Application::Application()
        : input(new InputManager())
    {
        gameSystems.Push(input);

        vgpu_log_set_log_callback(vgpu_log_callback, this);

        PlatformConstuct();
    }

    Application::~Application()
    {
        for (auto gameSystem : gameSystems)
        {
            SafeDelete(gameSystem);
        }

        gameSystems.Clear();
        vgpu_shutdown();
        SafeDelete(mainWindow);
        PlatformDestroy();
    }

    void Application::InitBeforeRun()
    {
        // Create main window.
        if (!headless)
        {
            mainWindow = new Window(
                config.windowTitle,
                config.windowSize.width, config.windowSize.height,
                WindowFlags::Resizable);

            vgpu_config gpu_config = {};
            gpu_config.backend_type = VGPU_BACKEND_TYPE_VULKAN;
#ifdef _DEBUG
            gpu_config.debug = true;
#endif
            gpu_config.window_handle = mainWindow->GetHandle();
            gpu_config.color_format = VGPU_TEXTURE_FORMAT_BGRA8;
            gpu_config.depth_stencil_format = VGPU_TEXTURE_FORMAT_D32F;
            if (!vgpu_init(&gpu_config)) {
                headless = true;
            }
        }

        Initialize();
        if (exitCode)
        {
            //Stop();
            return;
        }

        time.ResetElapsedTime();
        BeginRun();
    }

    void Application::Initialize()
    {
        for (auto gameSystem : gameSystems)
        {
            gameSystem->Initialize();
        }
    }

    void Application::BeginRun()
    {

    }

    void Application::EndRun()
    {

    }

    bool Application::BeginDraw()
    {
        if (!vgpu_frame_begin()) {
            return false;
        }

        for (auto gameSystem : gameSystems)
        {
            gameSystem->BeginDraw();
        }

        return true;
    }

    void Application::Draw(const GameTime& gameTime)
    {
        for (auto gameSystem : gameSystems)
        {
            gameSystem->Draw(time);
        }
    }

    void Application::EndDraw()
    {
        for (auto gameSystem : gameSystems)
        {
            gameSystem->EndDraw();
        }

        vgpu_frame_finish();
    }

    int Application::Run()
    {
        if (running) {
            LOG_ERROR("Application is already running");
            return EXIT_FAILURE;
        }

        PlatformRun();
        return exitCode;
    }

    void Application::Tick()
    {
        time.Tick([&]()
            {
                Update(time);
            });

        Render();
    }

    void Application::Update(const GameTime& gameTime)
    {
        for (auto gameSystem : gameSystems)
        {
            gameSystem->Update(gameTime);
        }
    }

    void Application::Render()
    {
        // Don't try to render anything before the first Update.
        if (running
            && time.GetFrameCount() > 0
            && !mainWindow->IsMinimized()
            && BeginDraw())
        {
            Draw(time);
            EndDraw();
        }
    }
} // namespace alimer