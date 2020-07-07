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
        ImGui::DestroyContext();
        PlatformDestroy();
        vgpu_shutdown();
        LOG_INFO("Application destroyed correctly");
    }

    void Application::InitBeforeRun()
    {
        // Create main window.
        if (!headless)
        {
            window.Create(config.windowTitle, config.windowSize, WindowFlags::Resizable);

            vgpu_init_info gpu_init_info = {};
            gpu_init_info.swapchain.native_handle = window.GetHandle();
            gpu_init_info.swapchain.width = window.GetSize().width;
            gpu_init_info.swapchain.height = window.GetSize().height;
            if (!vgpu_init(&gpu_init_info)) {
                headless = true;
            }

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

            /*ImGuiIO& io = ImGui::GetIO();
            io.BackendRendererName = "Alimer Direct3D12";
            io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
            io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

            ImGuiViewport* main_viewport = ImGui::GetMainViewport();
            main_viewport->RendererUserData = IM_NEW(ImGuiViewportDataD3D12)(maxInflightFrames);

            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
                platform_io.Renderer_CreateWindow = ImGui_D3D12_CreateWindow;
                platform_io.Renderer_DestroyWindow = ImGui_D3D12_DestroyWindow;
                platform_io.Renderer_SetWindowSize = ImGui_D3D12_SetWindowSize;
                platform_io.Renderer_RenderWindow = ImGui_D3D12_RenderWindow;
                platform_io.Renderer_SwapBuffers = ImGui_D3D12_SwapBuffers;
            }*/

            
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
        vgpu_begin_frame();

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

        //window.GetSwapChain()->Present();
        vgpu_end_frame();
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
