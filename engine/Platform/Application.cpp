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

        // Figure out the graphics backend API.
        if (config.rendererType  == GPUBackendType::Count)
        {
            //config.rendererType = gpu::getPlatformBackend();
        }

        platform = Platform::Create(this);

        s_appCurrent = this;
    }

    Application::~Application()
    {
        agpu::shutdown();
        s_appCurrent = nullptr;
        platform.reset();
    }

    Application* Application::Current()
    {
        return s_appCurrent;
    }

    void Application::InitBeforeRun()
    {
        agpu::InitFlags initFlags = agpu::InitFlags::None;

#ifdef _DEBUG
        initFlags |= agpu::InitFlags::DebugRuntime;
#endif

        agpu::PresentationParameters presentationParameters = {};
        presentationParameters.windowHandle = GetMainWindow().GetNativeHandle();

        agpu::init(initFlags, &presentationParameters);

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
        if (!agpu::beginFrame())
            return;

        agpu::PushDebugGroup("Frame");

        /*RenderPassDescription renderPass{};
        renderPass.colorAttachments[0].texture = GetGraphics()->GetPrimarySwapChain()->GetColorTexture();
        renderPass.colorAttachments[0].clearColor = Colors::CornflowerBlue;

        commandBuffer->BeginRenderPass(&renderPass);
        commandBuffer->EndRenderPass();*/

        agpu::PopDebugGroup();

        agpu::endFrame();
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
