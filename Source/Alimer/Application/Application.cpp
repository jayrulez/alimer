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
#include "Application/AppHost.h"
#include "Core/Log.h"
#include "Core/Input.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/gpu/gpu.h"
#include "UI/ImGuiLayer.h"
#include "Math/Color.h"

namespace Alimer
{
    // Make sure this is linked in.
    void ApplicationDummy()
    {
    }

    Application* Application::s_current;

    Application::Application(const Configuration& config)
        : config{ config }
    {
        // Create default AppHost.
        host = AppHost::CreateDefault(this);

        // Construct platform logic first.
        RegisterSubsystem(new Input());

        s_current = this;

        LOGI("Application started");
    }

    Application::~Application()
    {
        //gameSystems.clear();
        ImGuiLayer::Shutdown();
        RemoveSubsystem<Input>();
        gpu::Shutdown();
        s_current = nullptr;
        LOGI("Application destroyed correctly");
    }

    Application* Application::GetCurrent()
    {
        return s_current;
    }

    void Application::InitBeforeRun()
    {
        // Init subsytems.
        GetSubsystem<Input>()->Initialize();

        ImGuiLayer::Initialize();

        // Init GPU.
        auto bounds = GetWindow()->GetBounds();

        GraphicsDeviceSettings settings = {};
        settings.sampleCount = 4;
        if (!gpu::Init(GetWindow()->GetNativeHandle(), gpu::InitFlags::DebugOutput))
        {

        }

        // Create main window SwapChain
        /*struct VertexPositionColor
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
        vertexBuffer.Create(BufferUsage::Vertex, 3, sizeof(VertexPositionColor), vertices);*/

        ImGuiLayer::Initialize();

        Initialize();
        if (exitCode)
        {
            //Stop();
            return;
        }

        running = true;
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
        //if (!GraphicsDevice::Instance->BeginFrame())
        //    return false;

        //ImGuiLayer::BeginFrame(graphics->GetRenderWindow(), 0.0f);

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

        /*CommandBuffer& commandBuffer = graphicsDevice->RequestCommandBuffer("Clear");
        commandBuffer.PushDebugGroup("Clear");
        RenderPassColorAttachment colorAttachment = {};
        colorAttachment.texture = graphicsDevice->GetBackbufferTexture();
        colorAttachment.clearColor = Colors::CornflowerBlue;
        //colorAttachment.loadAction = LoadAction::DontCare;
        //colorAttachment.slice = 1;
        commandBuffer.BeginRenderPass(1, &colorAttachment, nullptr);
        commandBuffer.EndRenderPass();
        commandBuffer.PopDebugGroup();
        commandBuffer.Commit();*/
    }

    void Application::EndDraw()
    {
        /*for (auto& gameSystem : gameSystems)
        {
            gameSystem->EndDraw();
        }*/

        //ImGuiLayer::EndFrame();
        //GetGraphics()->GetMainWindow()->Present();
        //GraphicsDevice::Instance->EndFrame();
    }

    int Application::Run()
    {
        if (running) {
            LOGE("Application is already running");
            return EXIT_FAILURE;
        }

        host->Run();
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
            && !GetWindow()->IsMinimized()
            && BeginDraw())
        {
            Draw(time);
            EndDraw();
        }
    }


    Window* Application::GetWindow() const
    {
        return host->GetWindow();
    }
}
