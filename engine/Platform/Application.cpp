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
#include "IO/FileSystem.h"
#include "UI/ImGuiLayer.h"
#include <agpu.h>

namespace Alimer
{
    static Application* s_appCurrent = nullptr;
    static agpu_buffer vertex_buffer;
    static agpu_pipeline render_pipeline;

    static void agpu_callback(void* context, agpu_log_level level, const char* message)
    {
        ALIMER_UNUSED(context);

        switch (level)
        {
        case AGPU_LOG_LEVEL_INFO:
            LOGI(message);
            break;
        case AGPU_LOG_LEVEL_WARN:
            LOGW(message);
            break;
        case AGPU_LOG_LEVEL_ERROR:
            LOGE(message);
            break;
        default:
            break;
        }
    }

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
        ImGuiLayer::Shutdown();
        agpu_shutdown();
        s_appCurrent = nullptr;
        platform.reset();
    }

    Application* Application::Current()
    {
        return s_appCurrent;
    }

    void Application::InitBeforeRun()
    {
        agpu_set_preferred_backend(AGPU_BACKEND_TYPE_OPENGL);
        agpu_set_log_callback(agpu_callback, this);
        agpu_config config = {};

#ifdef _DEBUG
        config.debug = true;
#endif

        // Setup swapchain info.
        config.swapchain_info.window_handle = GetMainWindow().GetHandle();
        config.swapchain_info.width = GetMainWindow().GetWidth();
        config.swapchain_info.height = GetMainWindow().GetHeight();

        if (!agpu_init("Alimer", &config))
        {
            headless = true;
        }

        // Create SwapChain
        ImGuiLayer::Initialize();

        assets.Load<Texture>("texture.png");

        // Define the geometry for a triangle.
        float vertices[] = {
            0.0f, 0.5f, 0.0f,
            0.5f, -0.5f, 0.0f,
            -0.5f,  -0.5f, 0.0f
        };

        agpu_buffer_info buffer_info = {};
        buffer_info.size = sizeof(vertices);
        buffer_info.type = AGPU_BUFFER_TYPE_VERTEX;
        buffer_info.data = vertices;

        vertex_buffer = agpu_create_buffer(&buffer_info);

        auto vs_bytecode = File::ReadAllBytes("assets/shaders/triangle.vert.spv");
        auto fs_bytecode = File::ReadAllBytes("assets/shaders/triangle.frag.spv");

        agpu_shader_info shader_info = {};
        shader_info.vertex.code = vs_bytecode.data();
        shader_info.vertex.size = vs_bytecode.size();
        shader_info.fragment.code = fs_bytecode.data();
        shader_info.fragment.size = fs_bytecode.size();
        agpu_shader shader = agpu_create_shader(&shader_info);

        // Create pipeline
        agpu_pipeline_info pipeline_info{};
        pipeline_info.shader = shader;
        render_pipeline = agpu_create_pipeline(&pipeline_info);

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
        if (!agpu_begin_frame())
            return;

        agpu_push_debug_group("Frame");

        /*RenderPassDesc renderPass{};
        renderPass.colorAttachments[0].texture = swapChain->GetCurrentTexture();
        renderPass.colorAttachments[0].clearColor = Colors::CornflowerBlue;
        context->BeginRenderPass(renderPass);
        context->EndRenderPass();*/
        agpu_bind_pipeline(render_pipeline);
        agpu_draw(3, 1, 0);
        agpu_pop_debug_group();
        agpu_end_frame();
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
