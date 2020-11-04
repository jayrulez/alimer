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

#include "Platform/Application.h"
#include "AlimerConfig.h"
#include "Core/Log.h"
#include "Graphics/Graphics.h"
#include "IO/FileSystem.h"
#include "Platform/Event.h"
#include "Platform/Input.h"
#include "Platform/Platform.h"
#include "UI/ImGuiLayer.h"

#if defined(__ANDROID__)
#    include <android/log.h>
#    include <spdlog/sinks/android_sink.h>
#endif

#if ALIMER_PLATFORM_WINDOWS
#    include <spdlog/sinks/basic_file_sink.h>
#    include <spdlog/sinks/stdout_color_sinks.h>
#else
#    include <spdlog/sinks/stdout_color_sinks.h>
#endif

namespace alimer
{
    static Application* s_appCurrent = nullptr;

    Application::Application(const Config& config)
        : name("Alimer")
        , config{config}
        , state(State::Uninitialized)
        , assets(config.rootDirectory)
    {
        ALIMER_ASSERT_MSG(s_appCurrent == nullptr, "Cannot create more than one Application");

        std::vector<spdlog::sink_ptr> sinks;
#if defined(__ANDROID__)
        sinks.push_back(std::make_shared<spdlog::sinks::android_sink_mt>("Alimer"));
#endif

#ifdef _WIN32
        sinks.push_back(std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>());
        sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("Log.txt", true));
#else
        sinks.push_back(std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>());
#endif

        auto logger = std::make_shared<spdlog::logger>("logger", sinks.begin(), sinks.end());

#if defined(_DEBUG)
        logger->set_level(spdlog::level::debug);
#else
        logger->set_level(spdlog::level::info);
#endif

        logger->set_pattern(LOGGER_FORMAT);
        spdlog::set_default_logger(logger);

        LOGI("Logger initialized");

        s_appCurrent = this;
    }

    Application::~Application()
    {
#if defined(ALIMER_IMGUI)
        imguiLayer.reset();
#endif
        RemoveSubsystem<Input>();
        RemoveSubsystem<Graphics>();
        graphics.Reset();
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
            settings.backendType = config.backendType;
            settings.flags = config.deviceFlags;
            graphics = Graphics::Create(window->GetHandle(), settings);
            RegisterSubsystem(graphics);
#if defined(ALIMER_IMGUI)
            imguiLayer = std::make_unique<ImGuiLayer>(graphics);
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
        auto& commandList = graphics->BeginCommandList();
        commandList.PresentBegin();
        OnDraw(commandList);
        commandList.PresentEnd();
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
