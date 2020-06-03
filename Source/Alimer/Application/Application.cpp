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
#include "Core/Engine.h"
#include "graphics/GraphicsDevice.h"
#include "graphics/GraphicsContext.h"
#include "UI/Gui.h"
#include "Input/InputManager.h"
#include "Core/Log.h"

namespace alimer
{
    Application::Application()
        : input(new InputManager())
    {
        Engine::Configuration config;
        config.plugins.Push("Alimer.Direct3D11");
        config.plugins.Push("Alimer.Direct3D12");
        config.plugins.Push("Alimer.Vulkan");
        engine = Engine::Create(config, allocator);

        gameSystems.Push(input);
        PlatformConstuct();
    }

    Application::~Application()
    {
        for (auto gameSystem : gameSystems)
        {
            SafeDelete(gameSystem);
        }

        gameSystems.Clear();
        gui.reset();
        mainWindow.reset();
        mainGraphicsContext.Reset();
        Engine::Destroy(engine);
        PlatformDestroy();
    }

    void Application::InitBeforeRun()
    {
        if (!engine->Initialize()) {
            return;
        }

        // Create main window.
        if (!headless)
        {
            mainWindow.reset(new Window(
                config.windowTitle,
                config.windowSize.width, config.windowSize.height,
                WindowFlags::Resizable)
            );

            //window_set_centered(main_window);
            GraphicsContextDescription contextDesc = {};
            contextDesc.handle = mainWindow->GetHandle();
            contextDesc.width = mainWindow->GetSize().width;
            contextDesc.height = mainWindow->GetSize().height;

            mainGraphicsContext = engine->GetGraphicsDevice().CreateContext(contextDesc);

            /*GraphicsViewDescriptor viewDescriptor = {};
            viewDescriptor.width = mainWindow->GetSize().width;
            viewDescriptor.height = mainWindow->GetSize().height;
            //contextDescriptor.colorFormat = PixelFormat::BGRA8UNormSrgb;
            viewDescriptor.colorFormat = PixelFormat::RGBA8UNorm;
            mainView = graphicsDevice->CreateView(mainWindow->GetHandle(), &viewDescriptor);

            gui.reset(new Gui(graphicsDevice.get(), mainWindow.get()));*/
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
        //mainGraphicsContext->BeginFrame();

        for (auto gameSystem : gameSystems)
        {
            gameSystem->BeginDraw();
        }

        //gui->BeginFrame();

        return true;
    }

    static bool show_demo_window = true;
    static bool show_another_window = false;
    static Color clear_color = Color(0.45f, 0.55f, 0.60f, 1.00f);

    void Application::Draw(const GameTime& gameTime)
    {
        for (auto gameSystem : gameSystems)
        {
            gameSystem->Draw(time);
        }

#if TODO_IMGUI
        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }


        auto& context = graphicsDevice->BeginContext("Frame");
        RenderPassDescriptor renderPass = {};
        renderPass.colorAttachments[0].texture = mainView->GetCurrentColorTexture();
        renderPass.colorAttachments[0].clearColor = clear_color;
        context.BeginRenderPass(&renderPass);
        context.EndRenderPass();
        gui->Render(context);
        context.Flush(true);
#endif // TODO_IMGUI

    }

    void Application::EndDraw()
    {
        for (auto gameSystem : gameSystems)
        {
            gameSystem->EndDraw();
        }

        //mainGraphicsContext->Present();
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
        time.Tick([&]()
            {
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
            && !mainWindow->IsMinimized()
            && BeginDraw())
        {
            Draw(time);
            EndDraw();
        }
    }
} // namespace alimer
