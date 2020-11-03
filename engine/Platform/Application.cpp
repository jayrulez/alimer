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

#include "AlimerConfig.h"
#include "Platform/Application.h"
#include "Core/Log.h"
#include "Graphics/Graphics.h"
#include "IO/FileSystem.h"
#include "Platform/Event.h"
#include "Platform/Input.h"
#include "Platform/Platform.h"
#include "UI/ImGuiLayer.h"

namespace alimer
{
    static Application* s_appCurrent = nullptr;

    Application::Application(const Config& config)
        : name("Alimer"), config{config}, state(State::Uninitialized), assets(config.rootDirectory)
    {
        ALIMER_ASSERT_MSG(s_appCurrent == nullptr, "Cannot create more than one Application");

        s_appCurrent = this;
    }

    Application::~Application()
    {
#if defined(ALIMER_IMGUI)
        imguiLayer.reset();
#endif
        RemoveSubsystem<Input>();
        RemoveSubsystem<Graphics>();
        s_appCurrent = nullptr;
    }

    Application* Application::Current()
    {
        return s_appCurrent;
    }

    const std::string& Application::GetName() const
    {
        return name;
    }

    void Application::SetName(const std::string& name_)
    {
        name = name_;
    }

    void Application::InitBeforeRun()
    {
        if (!headless)
        {
            // Create main window.
            WindowFlags windowFlags = WindowFlags::None;
            if (config.resizable)
                windowFlags |= WindowFlags::Resizable;

            if (config.fullscreen)
                windowFlags |= WindowFlags::Fullscreen;

            window = std::make_unique<Window>(config.title, Window::Centered, Window::Centered, config.width, config.height, windowFlags);

            // Input module
            RegisterSubsystem(new Input());

            // Init graphics device.
            GraphicsSettings settings{};
            RegisterSubsystem(Graphics::Create(window->GetHandle(), settings));
#if defined(ALIMER_IMGUI)
            imguiLayer = std::make_unique<ImGuiLayer>(GetSubsystem<Graphics>());
#endif
        }

        Initialize();
    }

    void Application::Run()
    {
        InitBeforeRun();

        state = State::Running;
        while (state == State::Running)
        {
            Event evt{};
            while (PollEvent(evt))
            {
                if (evt.type == EventType::Quit)
                {
                    state = State::Uninitialized;
                    break;
                }
            }

            Tick();
        }
    }

    void Application::Tick()
    {
        auto graphics = GetSubsystem<Graphics>();
        if (!graphics->BeginFrame())
            return;

        OnDraw();

        graphics->EndFrame();
    }

    const Config* Application::GetConfig()
    {
        return &config;
    }

    Window& Application::GetWindow() const
    {
        return *window;
    }
}
