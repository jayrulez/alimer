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
#include "Graphics/CommandContext.h"
#include "Core/Log.h"
#include <agpu.h>
#include <imgui.h>
#include <imgui_internal.h>

#if defined(ALIMER_GLFW)
#   define GLFW_INCLUDE_NONE
#   include <GLFW/glfw3.h>
#endif

namespace alimer
{
    namespace
    {
        void* agpu_gl_get_proc_address(const char* name) {
#if defined(ALIMER_GLFW)
            return (void*)glfwGetProcAddress(name);
#else
            return nullptr;
#endif
        }

        void agpu_callback(void* context, const char* message, agpu_log_level level) {
            switch (level)
            {
            case AGPU_LOG_LEVEL_INFO:
                Log(LogLevel::Info, message);
                break;
            case AGPU_LOG_LEVEL_WARN:
                Log(LogLevel::Warn, message);
                break;
            case AGPU_LOG_LEVEL_ERROR:
                Log(LogLevel::Error, message);
                break;
            default:
                break;
            }
        }
    }

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
        ImGui::DestroyContext();
        agpu_shutdown();
        PlatformDestroy();
        LOGI("Application destroyed correctly");
    }

    void Application::InitBeforeRun()
    {
        // Init subsytems.
        GetSubsystem<Input>()->Initialize();

        // Init ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.UserData = this;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
        //io.ConfigViewportsNoAutoMerge = true;
        //io.ConfigViewportsNoTaskBarIcon = true;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        io.Fonts->AddFontDefault();
        //io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 18.0f);

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        // Color scheme
        /*style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
        style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);*/

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        auto rendererType = AGPU_BACKEND_OPENGL;

        // Create main window.
        WindowFlags windowFlags = WindowFlags::Resizable;
        if (rendererType == AGPU_BACKEND_OPENGL) {
            windowFlags |= WindowFlags::OpenGL;
        }

        window.reset(new Window());
        window->Create(config.windowTitle, config.windowSize, windowFlags);

        // Init graphics.
        agpu_config config = {};
#ifdef _DEBUG
        config.debug = true;
#endif
        config.callback = agpu_callback;
        config.context = this;

#if defined(ALIMER_GLFW)
        config.gl_get_proc_address = agpu_gl_get_proc_address;
#endif

        if (agpu_init(&config) == false)
        {
            headless = true;
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
        agpu_frame_begin();

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
        agpu_frame_end();

#if defined(ALIMER_GLFW)
        glfwSwapBuffers((GLFWwindow*)window->GetWindow());
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
