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

#include "Platform/Application.h"

namespace Alimer
{
    class HelloWorldApp final : public Application
    {
    public:
        HelloWorldApp(const Config& config)
            : Application(config)
        {

        }

        void Initialize() override;
        void OnDraw() override;
    };

    void HelloWorldApp::Initialize()
    {
        struct Vertex
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

        RHIResourceUploadBatch* batch = nullptr;
        auto buffer = rhiDevice->CreateStaticBuffer(batch, triangleVertices, RHIBuffer::Usage::Vertex, sizeof(Vertex));
    }

    void HelloWorldApp::OnDraw()
    {
        RHICommandBuffer* cb = windowSwapChain->CurrentFrameCommandBuffer();
        cb->PushDebugGroup("Frame");
        RenderPassDesc renderPass{};
        renderPass.colorAttachments[0].texture = windowSwapChain->GetCurrentTexture();
        renderPass.colorAttachments[0].clearColor = Colors::CornflowerBlue;
        cb->BeginRenderPass(renderPass);
        cb->EndRenderPass();
        cb->PopDebugGroup();
    }

    Application* CreateApplication()
    {
        Config config{};
        //config.rendererType = GPUBackendType::Vulkan;
        config.title = "TestApp";
        //config.fullscreen = true;
        //config.width = 1280;
        //config.height = 720;

        return new HelloWorldApp(config);
    }
}

