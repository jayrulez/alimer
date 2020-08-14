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
#include "Core/Log.h"
#include "Core/Window.h"
#include "Core/Input.h"
#include "Graphics/Graphics.h"
#include "UI/ImGuiLayer.h"
#include "Math/Color.h"
#include <imgui.h>

namespace alimer
{
    Application::Application()
    {
        // Construct platform logic first.
        PlatformConstruct();
        RegisterSubsystem(new Input());

        // Init ImGui
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        /*ImGuiIO& io = ImGui::GetIO();
        io.UserData = this;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
        //io.ConfigViewportsNoAutoMerge = true;
        //io.ConfigViewportsNoTaskBarIcon = true;
        */

        LOGI("Application started");
    }

    Application::~Application()
    {
        //gameSystems.clear();
        RemoveSubsystem<Input>();
        ImGuiLayer::Shutdown();
        PlatformDestroy();
        LOGI("Application destroyed correctly");
    }

    void Application::InitBeforeRun()
    {
        // Init subsytems.
        GetSubsystem<Input>()->Initialize();

        // Init GPU.
        if (!headless)
        {
            auto graphics = Graphics::Create(RendererType::Vulkan);

            graphics->GetRenderWindow()->SetTitle(config.windowTitle);
            if (!graphics->SetMode(config.windowSize, WindowFlags::Resizable))
            {
                headless = true;
            }
            else
            {
                // Create main window SwapChain
                /*windowSwapChain = new SwapChain(window.GetHandle(), window.GetWidth(), window.GetHeight(), window.IsFullscreen());

                struct VertexPositionColor
                {
                    Vector3 position;
                    Color color;
                };

                const VertexPositionColor vertices[] = {
                    // positions            colors
                    {{0.0f, 0.5f, 0.5f},    {1.0f, 0.0f, 0.0f, 1.0f}},
                    {{ 0.5f, -0.5f, 0.5f},  {0.0f, 1.0f, 0.0f, 1.0f}},
                    {{-0.5f, -0.5f, 0.5f},  {0.0f, 0.0f, 1.0f, 1.0f}}
                };

                Buffer vertexBuffer;
                vertexBuffer.Create(BufferUsage::Vertex, 3, sizeof(VertexPositionColor), vertices);
                */
                ImGuiLayer::Initialize();
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
        /*for (auto& gameSystem : gameSystems)
        {
            gameSystem->Initialize();
        }*/
    }

    void Application::BeginRun()
    {

    }

    void Application::EndRun()
    {

    }

    bool Application::BeginDraw()
    {
        if (!graphics->BeginFrame()) {
            return false;
        }
        ImGuiLayer::BeginFrame(graphics->GetRenderWindow(), 0.0f);

        /*for (auto& gameSystem : gameSystems)
        {
            gameSystem->BeginDraw();
        }*/

        return true;
    }

    void Application::Draw(const GameTime& gameTime)
    {
        /*for (auto& gameSystem : gameSystems)
        {
            gameSystem->Draw(time);
        }*/

        if (GetInput()->IsMouseButtonDown(MouseButton::Right)) {
            LOGI("Right pressed");
        }

        if (GetInput()->IsMouseButtonHeld(MouseButton::Right)) {
            LOGI("Right held");
        }

        graphics->PushDebugGroup("Clear");
        RenderPassColorAttachment colorAttachment = {};
        colorAttachment.clearColor = Colors::CornflowerBlue;
        //colorAttachment.loadAction = LoadAction::DontCare;
        graphics->BeginRenderPass(1, &colorAttachment, nullptr);
        graphics->EndRenderPass();
        graphics->PopDebugGroup();
    }

    void Application::EndDraw()
    {
        /*for (auto& gameSystem : gameSystems)
        {
            gameSystem->EndDraw();
        }*/
        
        ImGuiLayer::EndFrame();
        graphics->EndFrame();
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
        /*for (auto& gameSystem : gameSystems)
        {
            gameSystem->Update(gameTime);
        }*/
    }

    void Application::Render()
    {
        // Don't try to render anything before the first Update.
        if (running
            && time.GetFrameCount() > 0
            && !graphics->GetRenderWindow()->IsMinimized()
            && BeginDraw())
        {
            Draw(time);
            EndDraw();
        }
    }
}
