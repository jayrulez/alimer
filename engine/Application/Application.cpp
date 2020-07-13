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
#include "Core/Window.h"
#include "Core/Input.h"
#include "Graphics/Graphics.h"
#include "Core/Log.h"

namespace alimer
{
    Application::Application()
    {
        RegisterSubsystem(new Input());

        PlatformConstuct();
        LOG_INFO("Application started");
    }

    Application::~Application()
    {
        gameSystems.Clear();
        window.Close();
        gui.Reset();
        RHIShutdown();
        RemoveSubsystem<Input>();
        PlatformDestroy();
        LOG_INFO("Application destroyed correctly");
    }

    void Application::InitBeforeRun()
    {
        // Init subsytems
        GetSubsystem<Input>()->Initialize();

        // Create main window.
        window.Create(config.windowTitle, config.windowSize, WindowFlags::Resizable);

        // Init graphics
#ifdef _DEBUG
        bool enableDebugLayer = true;
#else
        bool enableDebugLayer = false;
#endif

        if (!Graphics::Initialize(&window, enableDebugLayer, config.preferredGraphicsBackend))
        {
            headless = true;
        }
        else
        {
            gui = new Gui(&window);
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
        for (auto& gameSystem : gameSystems)
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
        RHIBeginFrame();
        gui->BeginFrame();

        for (auto& gameSystem : gameSystems)
        {
            gameSystem->BeginDraw();
        }

        return true;
    }

    void Application::Draw(const GameTime& gameTime)
    {
        for (auto& gameSystem : gameSystems)
        {
            gameSystem->Draw(time);
        }

        if (GetInput()->IsMouseButtonDown(MouseButton::Right)) {
            LOG_INFO("Right pressed");
        }

        if (GetInput()->IsMouseButtonHeld(MouseButton::Right)) {
            LOG_INFO("Right held");
        }

        /*auto commandContext = Graphics::GetDefaultContext();
        commandContext->PushDebugGroup("Clear");
        RenderPassDescription renderPass = window.GetSwapChain()->GetCurrentRenderPassDescription();
        renderPass.colorAttachments[0].clearColor = Colors::CornflowerBlue;
        commandContext->BeginRenderPass(renderPass);
        commandContext->EndRenderPass();
        commandContext->PopDebugGroup();*/
    }

    void Application::EndDraw()
    {
        for (auto& gameSystem : gameSystems)
        {
            gameSystem->EndDraw();
        }

        gui->Render();
        RHIEndFrame();
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

        // Update input at end of frame.
        GetInput()->Update();
    }

    void Application::Update(const GameTime& gameTime)
    {
        for (auto& gameSystem : gameSystems)
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
