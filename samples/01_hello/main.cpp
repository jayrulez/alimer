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
#include "Graphics/GraphicsBuffer.h"
#include "Graphics/SwapChain.h"
#include "Graphics/GraphicsDevice.h"

namespace Alimer
{
    class HelloWorldApp final : public Application
    {
    public:
        HelloWorldApp(const Config& config)
            : Application(config)
        {

        }
        ~HelloWorldApp() override;

        void Initialize() override;
        void OnDraw(CommandContext* context) override;

    private:
        std::unique_ptr<GraphicsBuffer> vertexBuffer;
    };

    HelloWorldApp::~HelloWorldApp()
    {

    }

    void HelloWorldApp::Initialize()
    {
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
        bufferDesc.size = sizeof(Vertex);
        vertexBuffer.reset(graphicsDevice->CreateBuffer(bufferDesc, triangleVertices));*/
    }

    void HelloWorldApp::OnDraw(CommandContext* context)
    {
        context->PushDebugGroup("Frame");
        RenderPassDesc renderPass{};
        renderPass.colorAttachments[0].texture = windowSwapChain->GetCurrentTexture();
        renderPass.colorAttachments[0].clearColor = Colors::CornflowerBlue;
        context->BeginRenderPass(renderPass);
        context->EndRenderPass();
        context->PopDebugGroup();
        //windowSwapChain->p
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

