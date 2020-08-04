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
#include "GPU/GPU.h"
#include "Core/Log.h"

namespace alimer
{
    Application::Application()
    {
        // Construct platform logic first.
        PlatformConstruct();
        RegisterSubsystem(new Input());
        LOGI("Application started");
    }

    Application::~Application()
    {
        gameSystems.clear();
        window.reset();
        RemoveSubsystem<Input>();
        PlatformDestroy();
        LOGI("Application destroyed correctly");
    }

    void Application::InitBeforeRun()
    {
        // Init subsytems.
        GetSubsystem<Input>()->Initialize();

        // Init graphics device
        if (!headless)
        {
            auto adapter = GPU::RequestAdapter(PowerPreference::Default);
            if (adapter == nullptr) {
                headless = true;
            }
        }

        // Create main window.
        WindowFlags windowFlags = WindowFlags::Resizable;
        /*if (gpu_backend_type == AGPU_BACKEND_OPENGL) {
            windowFlags |= WindowFlags::OpenGL;
        }*/

        window.reset(new Window());
        window->Create(config.windowTitle, config.windowSize, windowFlags);

        // Init graphics.
        /*agpu_swapchain_info swapchain_info = {};
        swapchain_info.native_handle = window->GetNativeHandle();
        swapchain_info.width = window->GetSize().width;
        swapchain_info.height = window->GetSize().height;

        agpu_config gpu_config = {};
#ifdef _DEBUG
        gpu_config.debug = true;
#endif
        gpu_config.swapchain = &swapchain_info;

        if (agpu_init(config.applicationName.c_str(), &gpu_config) == false)
        {
            headless = true;
        }*/

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
        //agpu_frame_begin();

        //window->BeginFrame();
        //ImGui::NewFrame();

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
            LOGI("Right pressed");
        }

        if (GetInput()->IsMouseButtonHeld(MouseButton::Right)) {
            LOGI("Right held");
        }
        /*CommandBuffer& commandBuffer = GPU->BeginCommandBuffer("Clear");
        commandContext->PushDebugGroup("Clear");
        RenderPassDescription renderPass = window.GetSwapChain()->GetCurrentRenderPassDescription();
        renderPass.colorAttachments[0].clearColor = Colors::CornflowerBlue;
        commandContext->BeginRenderPass(renderPass);
        commandContext->EndRenderPass();
        commandContext->PopDebugGroup();
        commandBuffer.Commit();*/
    }

    void Application::EndDraw()
    {
        for (auto& gameSystem : gameSystems)
        {
            gameSystem->EndDraw();
        }

        //ImGuiIO& io = ImGui::GetIO();
        //ImGui::Render();
        //agpu_frame_end();

#if defined(ALIMER_GLFW)
        //glfwSwapBuffers((GLFWwindow*)window->GetWindow());
#endif
    }

    int Application::Run()
    {
        if (running) {
            LOGE("Application is already running");
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
            && !window->IsMinimized()
            && BeginDraw())
        {
            Draw(time);
            EndDraw();
        }
    }
}
