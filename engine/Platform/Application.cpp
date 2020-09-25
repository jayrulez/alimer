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
#include "Platform/Application.h"
#include "Platform/Platform.h"
#include "Graphics/GraphicsDevice.h"
#include "UI/ImGuiLayer.h"
#include <agpu.h>

namespace Alimer
{
    static Application* s_appCurrent = nullptr;

    Application::Application(const Config& config)
        : config{ config }
        , state(State::Uninitialized)
        , assets(config.rootDirectory)
    {
        ALIMER_ASSERT_MSG(s_appCurrent == nullptr, "Cannot create more than one Application");

        platform = Platform::Create(this);
        s_appCurrent = this;
    }

    Application::~Application()
    {
        swapChain.Reset();
        graphics.reset();
        ImGuiLayer::Shutdown();
        agpu::Shutdown();
        s_appCurrent = nullptr;
        platform.reset();
    }

    Application* Application::Current()
    {
        return s_appCurrent;
    }

    void Application::InitBeforeRun()
    {
        agpu::InitFlags initFlags = {};
#ifdef _DEBUG
        initFlags |= agpu::InitFlags::DebugRuntime;
#endif

        agpu::Init(initFlags, GetMainWindow().GetNativeHandle());

        ImGuiLayer::Initialize();

        assets.Load<Texture>("texture.png");

        // Define the geometry for a triangle.
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
        platform->Run();
    }

    void Application::Tick()
    {
        auto swapchain = agpu::GetPrimarySwapchain();

        if (agpu::BeginFrame(swapchain) != agpu::FrameOpResult::Success)
            return;

        agpu::PushDebugGroup("Frame");

        agpu::RenderPassColorAttachment colorAttachment{};
        colorAttachment.texture = agpu::GetCurrentTexture(swapchain);
        //renderPass.colorAttachments[0].clearColor = Colors::CornflowerBlue;
        colorAttachment.clearColor = { 0.392156899f, 0.584313750f, 0.929411829f, 1.0f };

        agpu::RenderPassDescription renderPass{};
        renderPass.colorAttachmentsCount = 1u;
        renderPass.colorAttachments = &colorAttachment;

        agpu::BeginRenderPass(&renderPass);
        agpu::EndRenderPass();

        agpu::PopDebugGroup();
        agpu::EndFrame(swapchain);
    }

    const Config* Application::GetConfig()
    {
        return &config;
    }

    Window& Application::GetMainWindow() const
    {
        return platform->GetMainWindow();
    }
}
