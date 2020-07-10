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
#include "Input/InputManager.h"
#include "Graphics/GraphicsDevice.h"
#include "Core/Log.h"
#include <vgpu.h>

namespace alimer
{
    Application::Application()
        : input(new InputManager())
    {
        gameSystems.Push(input);
        PlatformConstuct();
        LOG_INFO("Application started");
    }

    Application::~Application()
    {
        for (auto gameSystem : gameSystems)
        {
            SafeDelete(gameSystem);
        }

        gameSystems.Clear();
        window.Close();
        SafeDelete(_gui);
        PlatformDestroy();
        vgpu::shutdown();
        LOG_INFO("Application destroyed correctly");
    }

    void Application::InitBeforeRun()
    {
        // Create main window.
        if (!headless)
        {
            window.Create(config.windowTitle, config.windowSize, WindowFlags::Resizable);

            vgpu::InitFlags initFlags = vgpu::InitFlags::None;
#ifdef _DEBUG
            initFlags |= vgpu::InitFlags::DebugRutime;
#endif

            vgpu::PresentationParameters presentationParameters = {};
            presentationParameters.backbufferWidth = window.GetSize().width;
            presentationParameters.backbufferHeight = window.GetSize().height;
            presentationParameters.windowHandle = window.GetNativeHandle();
            presentationParameters.display = window.GetNativeDisplay();

            if (!vgpu::init(initFlags, presentationParameters))
            {
                headless = true;
            }
            else
            {
                _gui = new Gui(&window);
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
        vgpu::beginFrame();
        _gui->BeginFrame();

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

        //graphicsDevice->PushDebugGroup("Clear");
        /*vgpu_pass_begin_info begin_info = {};
        begin_info.color_attachments[0].texture = vgpu_get_backbuffer_texture();
        begin_info.color_attachments[0].clear_color = { 0.392156899f, 0.584313750f, 0.929411829f, 1.0f };
        vgpu_begin_pass(&begin_info);
        vgpu_end_pass();*/
        //graphicsDevice->BeginDefaultRenderPass(Colors::CornflowerBlue, 1.0f, 0);
        //graphicsDevice->PopDebugGroup();
    }

    void Application::EndDraw()
    {
        for (auto gameSystem : gameSystems)
        {
            gameSystem->EndDraw();
        }

        _gui->Render();
        vgpu::endFrame();
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
        time.Tick([&]() {
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
            && !window.IsMinimized()
            && BeginDraw())
        {
            Draw(time);
            EndDraw();
        }
    }
}
