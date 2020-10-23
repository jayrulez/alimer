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

#include "Core/Log.h"
#include "Platform/Platform.h"
#include "Platform/Event.h"
#include "Platform/Application.h"
#include "IO/FileSystem.h"
#include "RHI/RHI.h"
#include "UI/ImGuiLayer.h"

namespace Alimer
{
    static Application* s_appCurrent = nullptr;

    Application::Application(const Config& config)
        : name("Alimer")
        , config{ config }
        , state(State::Uninitialized)
        , assets(config.rootDirectory)
    {
        ALIMER_ASSERT_MSG(s_appCurrent == nullptr, "Cannot create more than one Application");

        s_appCurrent = this;
    }

    Application::~Application()
    {
        //ImGuiLayer::Shutdown();
        graphicsDevice.reset();
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
        // Create main window.
        WindowFlags windowFlags = WindowFlags::None;
        if (config.resizable)
            windowFlags |= WindowFlags::Resizable;

        if (config.fullscreen)
            windowFlags |= WindowFlags::Fullscreen;

        window = MakeUnique<Window>(config.title, Window::Centered, Window::Centered, config.width, config.height, windowFlags);

        // Init graphics
        bool enableDebugLayer = false;
#ifdef _DEBUG
        enableDebugLayer = true;
#endif

        graphicsDevice = GraphicsDevice::Create(window->GetHandle(), config.preferredBackendType, false, enableDebugLayer);
        if (!graphicsDevice)
        {
            headless = true;
        }

        ImGuiLayer::Initialize();

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
        CommandList commandList = graphicsDevice->BeginCommandList();
        graphicsDevice->PresentBegin(commandList);
        graphicsDevice->PushDebugGroup(commandList, "Frame");

        OnDraw(commandList);

        /*VGPUColor color = { 1.0f, 0.78f, 0.05f, 1.0f };
        vgpuPushDebugGroup("Frame", &color);

        VGPURenderPassDescriptor renderPass = {};
        renderPass.colorAttachmentCount = 1u;
        vgpuBeginRenderPass(&renderPass);
        vgpuEndRenderPass();
        context->BeginRenderPass(renderPass);
        context->EndRenderPass();
        agpu_bind_pipeline(render_pipeline);
        agpu_draw(3, 1, 0);*/
        graphicsDevice->PopDebugGroup(commandList);
        graphicsDevice->PresentEnd(commandList);
    }

    const Config* Application::GetConfig()
    {
        return &config;
    }
}
