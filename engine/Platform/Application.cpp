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
#include "UI/ImGuiLayer.h"

namespace Alimer
{
    static Application* s_appCurrent = nullptr;

    Application::Application(const Config& config)
        : config{ config }
        , state(State::Uninitialized)
        , assets(config.rootDirectory)
    {
        ALIMER_ASSERT_MSG(s_appCurrent == nullptr, "Cannot create more than one Application");

        // Init graphics device
        GraphicsDeviceFlags flags = GraphicsDeviceFlags::None;
#ifdef _DEBUG
        flags |= GraphicsDeviceFlags::DebugRuntime;
#endif
        if (!GraphicsDevice::Initialize("Alimer", config.preferredBackendType, flags))
        {
            headless = true;
        }

        s_appCurrent = this;
    }

    Application::~Application()
    {
        GraphicsDevice::Instance->WaitForGPU();
        //ImGuiLayer::Shutdown();
        windowSwapChain.reset();
        GraphicsDevice::Shutdown();
        s_appCurrent = nullptr;
    }

    Application* Application::Current()
    {
        return s_appCurrent;
    }

    void Application::InitBeforeRun()
    {
        // Create main window.
        WindowFlags windowFlags = WindowFlags::None;
        if (config.resizable)
            windowFlags |= WindowFlags::Resizable;

        if (config.fullscreen)
            windowFlags |= WindowFlags::Fullscreen;

        window = std::make_unique<Window>(config.title, Window::Centered, Window::Centered, config.width, config.height, windowFlags);

        // Create SwapChain
        windowSwapChain.reset(GraphicsDevice::Instance->CreateSwapChain());
        windowSwapChain->SetWindow(window.get());
        windowSwapChain->CreateOrResize();

        ImGuiLayer::Initialize();

        assets.Load<Texture>("texture.png");

        /*struct Vertex
        {
            Float3 position;
            Color color;
        };

        Vertex triangleVertices[] =
        {
            { { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -0.5f, -0.5, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        };

        BufferDescription bufferDesc{};
        bufferDesc.usage = BufferUsage::Vertex;
        bufferDesc.size = sizeof(triangleVertices);
        bufferDesc.stride = sizeof(Vertex);

        auto buffer = GetGraphics()->CreateBuffer(bufferDesc, triangleVertices);*/
    }

    void Application::Run()
    {
        InitBeforeRun();

        running = true;
        while (running)
        {
            Event evt {};
            while (PollEvent(evt))
            {
                if (evt.type == EventType::Quit)
                {
                    running = false;
                    break;
                }
            }

            Tick();
        }
    }

    void Application::Tick()
    {
        FrameOpResult result = GraphicsDevice::Instance->BeginFrame(windowSwapChain.get());
        if (result == FrameOpResult::SwapChainOutOfDate)
        {
            // TODO: resize;
        }

        if (result != FrameOpResult::Success)
        {
            LOGD("BeginFrame failed with result {}", ToString(result));
            return;
        }

        /*agpu_push_debug_group("Frame");
        RenderPassDesc renderPass{};
        renderPass.colorAttachments[0].texture = swapChain->GetCurrentTexture();
        renderPass.colorAttachments[0].clearColor = Colors::CornflowerBlue;
        context->BeginRenderPass(renderPass);
        context->EndRenderPass();
        agpu_bind_pipeline(render_pipeline);
        agpu_draw(3, 1, 0);
        agpu_pop_debug_group();*/

        GraphicsDevice::Instance->EndFrame(windowSwapChain.get());
    }

    const Config* Application::GetConfig()
    {
        return &config;
    }
}
