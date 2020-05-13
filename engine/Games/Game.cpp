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

#include "os/os.h"
#include "gpu/gpu.h"
#include "Games/Game.h"
#include "graphics/GraphicsDevice.h"
#include "graphics/ISwapChain.h"
#include "Input/InputManager.h"
#include "core/Log.h"

namespace alimer
{
    Game::Game(const Configuration& config_)
        : config(config_)
        , input(new InputManager())
    {
        /* Init OS first */
        os_init();

        gameSystems.push_back(input);
    }

    Game::~Game()
    {
        for (auto gameSystem : gameSystems)
        {
            SafeDelete(gameSystem);
        }

        gameSystems.clear();
        vgpuShutdown();
        window_destroy(main_window);
        os_shutdown();
    }

    static vgpu_buffer* vertexBuffer;
    static vgpu_buffer* indexBuffer;
    static vgpu_buffer* uboBuffer;
    static vgpu_shader shader;
    static agpu_pipeline render_pipeline;

    void Game::InitBeforeRun()
    {
        // Create main window.
        if (!headless)
        {
            main_window = window_create(
                config.windowTitle.c_str(),
                config.windowSize.width, config.windowSize.height,
                WINDOW_FLAG_RESIZABLE | WINDOW_FLAG_OPENGL);
            window_set_centered(main_window);

            GraphicsDeviceDesc deviceDesc = { };
#ifdef _DEBUG
            deviceDesc.flags |= GraphicsDeviceFlags::Debug;
#endif
            graphicsDevice = CreateGraphicsDevice(GraphicsAPI::Vulkan, &deviceDesc);

            SwapChainDesc swapChainDesc = {};
            swapChainDesc.width = window_width(main_window);
            swapChainDesc.height = window_height(main_window);
            swapChain = graphicsDevice->CreateSwapChain(main_window, &swapChainDesc);

            /*VGPUSwapChainInfo swapchain = {};
            swapchain.nativeHandle = window_handle(main_window);
            swapchain.width = window_width(main_window);
            swapchain.height = window_height(main_window);

            VGpuDeviceDescriptor deviceDesc = {};
            deviceDesc.preferredBackend = VGPUBackendType_Count;
            deviceDesc.flags = AGPU_DEVICE_FLAGS_VSYNC;

#ifdef _DEBUG
            deviceDesc.flags |= AGPU_DEVICE_FLAGS_DEBUG;
#endif
            deviceDesc.gl.GetProcAddress = gl_get_proc_address;
            deviceDesc.swapchain = &swapchain;

            if (!vgpuInit(&deviceDesc)) {
                headless = true;
            }

            const float vertices[] = {
                // pos                  color                       uvs 
                -1.0f, -1.0f, -1.0f,    1.0f, 0.0f, 0.0f, 1.0f,     0.0f, 0.0f,
                1.0f, -1.0f, -1.0f,    1.0f, 0.0f, 0.0f, 1.0f,     1.0f, 0.0f,
                1.0f,  1.0f, -1.0f,    1.0f, 0.0f, 0.0f, 1.0f,     1.0f, 1.0f,
                -1.0f,  1.0f, -1.0f,    1.0f, 0.0f, 0.0f, 1.0f,     0.0f, 1.0f,

                -1.0f, -1.0f,  1.0f,    0.0f, 1.0f, 0.0f, 1.0f,     0.0f, 0.0f,
                1.0f, -1.0f,  1.0f,    0.0f, 1.0f, 0.0f, 1.0f,     1.0f, 0.0f,
                1.0f,  1.0f,  1.0f,    0.0f, 1.0f, 0.0f, 1.0f,     1.0f, 1.0f,
                -1.0f,  1.0f,  1.0f,    0.0f, 1.0f, 0.0f, 1.0f,     0.0f, 1.0f,

                -1.0f, -1.0f, -1.0f,    0.0f, 0.0f, 1.0f, 1.0f,     0.0f, 0.0f,
                -1.0f,  1.0f, -1.0f,    0.0f, 0.0f, 1.0f, 1.0f,     1.0f, 0.0f,
                -1.0f,  1.0f,  1.0f,    0.0f, 0.0f, 1.0f, 1.0f,     1.0f, 1.0f,
                -1.0f, -1.0f,  1.0f,    0.0f, 0.0f, 1.0f, 1.0f,     0.0f, 1.0f,

                1.0f, -1.0f, -1.0f,    1.0f, 0.5f, 0.0f, 1.0f,     0.0f, 0.0f,
                1.0f,  1.0f, -1.0f,    1.0f, 0.5f, 0.0f, 1.0f,     1.0f, 0.0f,
                1.0f,  1.0f,  1.0f,    1.0f, 0.5f, 0.0f, 1.0f,     1.0f, 1.0f,
                1.0f, -1.0f,  1.0f,    1.0f, 0.5f, 0.0f, 1.0f,     0.0f, 1.0f,

                -1.0f, -1.0f, -1.0f,    0.0f, 0.5f, 1.0f, 1.0f,     0.0f, 0.0f,
                -1.0f, -1.0f,  1.0f,    0.0f, 0.5f, 1.0f, 1.0f,     1.0f, 0.0f,
                1.0f, -1.0f,  1.0f,    0.0f, 0.5f, 1.0f, 1.0f,     1.0f, 1.0f,
                1.0f, -1.0f, -1.0f,    0.0f, 0.5f, 1.0f, 1.0f,     0.0f, 1.0f,

                -1.0f,  1.0f, -1.0f,    1.0f, 0.0f, 0.5f, 1.0f,     0.0f, 0.0f,
                -1.0f,  1.0f,  1.0f,    1.0f, 0.0f, 0.5f, 1.0f,     1.0f, 0.0f,
                1.0f,  1.0f,  1.0f,    1.0f, 0.0f, 0.5f, 1.0f,     1.0f, 1.0f,
                1.0f,  1.0f, -1.0f,    1.0f, 0.0f, 0.5f, 1.0f,     0.0f, 1.0f
            };

            vgpu_buffer_info vertexBufferInfo = {};
            vertexBufferInfo.size = sizeof(vertices);
            vertexBufferInfo.usage = VGPUBufferUsage_Vertex;
            vertexBufferInfo.data = vertices;
            vertexBuffer = vgpu_buffer_create(&vertexBufferInfo);

            // create an index buffer 
            uint16_t indices[] = {
                0, 1, 2,  0, 2, 3,
                6, 5, 4,  7, 6, 4,
                8, 9, 10,  8, 10, 11,
                14, 13, 12,  15, 14, 12,
                16, 17, 18,  16, 18, 19,
                22, 21, 20,  23, 22, 20
            };
            vgpu_buffer_info indexBufferInfo = {};
            indexBufferInfo.size = sizeof(indices);
            indexBufferInfo.usage = VGPUBufferUsage_Index;
            indexBufferInfo.data = indices;
            indexBuffer = vgpu_buffer_create(&indexBufferInfo);

            vgpu_buffer_info uboBufferInfo = {};
            uboBufferInfo.size = 64;
            uboBufferInfo.usage = VGPUBufferUsage_Uniform | VGPUBufferUsage_Dynamic;
            uboBuffer = vgpu_buffer_create(&uboBufferInfo);

            uint32_t pixels[4 * 4] = {
                0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
                0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
                0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
                0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
            };
            VGPUTextureInfo textureInfo = {};
            textureInfo.size.width = 4;
            textureInfo.size.height = 4;
            textureInfo.format = AGPUPixelFormat_RGBA8UNorm;
            textureInfo.data = pixels;
            auto texture = vgpuTextureCreate(&textureInfo);

            const char* vertexShaderSource = "#version 330 core\n"
                "layout(std140) uniform;\n"
                "layout (location=0) in vec3 inPosition;\n"
                "layout (location=1) in vec3 inColor;\n"
                "layout (location=2) in vec2 inTexCoord;\n"
                "out vec3 Color;\n"
                "out vec2 TexCoord;\n"
                "uniform mat4 mvp;\n"
                "uniform Transform {\n"
                "   mat4 mvp;\n"
                "} transform;\n"
                "void main()\n"
                "{\n"
                "   gl_Position = transform.mvp * vec4(inPosition, 1.0);\n"
                "   Color = inColor;\n"
                "   TexCoord = inTexCoord * 5.0;\n"
                "}\0";

            const char* fragmentShaderSource = "#version 330 core\n"
                "out vec4 FragColor;\n"
                "in vec3 Color;\n"
                "in vec2 TexCoord;\n"
                "uniform sampler2D texture1;\n"
                "void main()\n"
                "{\n"
                "   FragColor = texture(texture1, TexCoord) * vec4(Color, 1.0);\n"
                "}\n\0";


            vgpu_shader_info shader_info = {};
            shader_info.vertex.source = vertexShaderSource;
            shader_info.fragment.source = fragmentShaderSource;
            shader = vgpuShaderCreate(&shader_info);

            vgpu_pipeline_info pipeline_info = {};
            pipeline_info.shader = shader;
            //pipeline_info.vertexInfo.layouts[0].stride = 28;
            pipeline_info.vertexInfo.attributes[0].format = VGPUVertexFormat_Float3;
            pipeline_info.vertexInfo.attributes[1].format = VGPUVertexFormat_Float4;
            pipeline_info.vertexInfo.attributes[2].format = VGPUVertexFormat_Float2;
            pipeline_info.depth_stencil.depth_compare = VGPU_COMPARE_FUNCTION_LESS_EQUAL;
            pipeline_info.depth_stencil.depth_write_enabled = true;

            render_pipeline = vgpu_create_pipeline(&pipeline_info);*/
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

    void Game::Initialize()
    {
        for (auto gameSystem : gameSystems)
        {
            gameSystem->Initialize();
        }
    }

    void Game::BeginRun()
    {

    }

    void Game::EndRun()
    {

    }

    bool Game::BeginDraw()
    {
        //vgpuFrameBegin();

        for (auto gameSystem : gameSystems)
        {
            gameSystem->BeginDraw();
        }

        return true;
    }

    void Game::Draw(const GameTime& gameTime)
    {
        //auto context = graphicsDevice->GetGraphicsContext();
        //context->BeginRenderPass(mainSwapChain.get(), Colors::CornflowerBlue);
        //context->EndMarker();

        for (auto gameSystem : gameSystems)
        {
            gameSystem->Draw(time);
        }
    }

#include <DirectXMath.h>
    using namespace DirectX;
    using float4x4 = XMFLOAT4X4;

    struct vs_params_t {
        float4x4 mvp;
    };

    void Game::EndDraw()
    {
        for (auto gameSystem : gameSystems)
        {
            gameSystem->EndDraw();
        }

        swapChain->Present();

        return;

        /*auto clear_color = Colors::CornflowerBlue;
        auto defaultRenderPass = vgpu_get_default_render_pass();
        vgpu_render_pass_set_color_clear_value(defaultRenderPass, 0, &clear_color.r);
        vgpu_cmd_begin_render_pass(defaultRenderPass);
        vgpu_cmd_end_render_pass();
        */
        uint32_t width = window_width(main_window);
        uint32_t height = window_height(main_window);

        XMMATRIX proj = XMMatrixPerspectiveFovRH(XMConvertToRadians(60.0f), (float)width / (float)height, 0.01f, 10.0f);
        XMMATRIX view = XMMatrixLookAtRH(XMVectorSet(0.0f, 1.5f, 6.0f, 0.0f), XMVectorZero(), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
        XMMATRIX view_proj = XMMatrixMultiply(view, proj);

        static float rx = 0.0f;
        static float ry = 0.0f;

        /* rotated model matrix */
        rx += 1.0f; ry += 2.0f;
        XMMATRIX rxm = XMMatrixRotationAxis(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), XMConvertToRadians(rx));
        XMMATRIX rym = XMMatrixRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), XMConvertToRadians(ry));
        XMMATRIX model = XMMatrixMultiply(rxm, rym);

        /* model-view-projection matrix for vertex shader */
        vs_params_t vs_params;
        XMMATRIX worldViewProjection = XMMatrixMultiply(model, view_proj);
        XMStoreFloat4x4(&vs_params.mvp, worldViewProjection);
        //vgpu_buffer_sub_data(uboBuffer, 0, 0, &vs_params);

        VGPURenderPassDescriptor renderPass;
        memset(&renderPass, 0, sizeof(VGPURenderPassDescriptor));

        vgpuCmdBeginRenderPass(&renderPass);
        vgpuSetPipeline(render_pipeline);
        vgpuCmdSetVertexBuffers(0, vertexBuffer, 0);
        vgpuCmdSetIndexBuffer(indexBuffer, 0);
        vgpu_set_uniform_buffer_data(0, 0, &vs_params, sizeof(vs_params));
        vgpuCmdDrawIndexed(36, 1, 0);
        vgpuCmdEndRenderPass();
        vgpuFrameFinish();
        window_swap_buffers(main_window);
    }

    int Game::Run()
    {
        if (running) {
            ALIMER_LOGERROR("Application is already running");
            return EXIT_FAILURE;
        }


#if !defined(__GNUC__) && _HAS_EXCEPTIONS
        try
#endif
        {
            Setup();

            if (exitCode)
                return exitCode;

            running = true;

            InitBeforeRun();

            // Main message loop
            while (running)
            {
                os_event evt;
                while (event_poll(&evt))
                {
                    if (evt.type == OS_EVENT_QUIT)
                    {
                        running = false;
                        break;
                    }
                }

                Tick();
            }
        }
#if !defined(__GNUC__) && _HAS_EXCEPTIONS
        catch (std::bad_alloc&)
        {
            //ErrorDialog(GetTypeName(), "An out-of-memory error occurred. The application will now exit.");
            return EXIT_FAILURE;
        }
#endif

        return exitCode;
    }

    void Game::Tick()
    {
        time.Tick([&]()
            {
                Update(time);
            });

        Render();
    }

    void Game::Update(const GameTime& gameTime)
    {
        for (auto gameSystem : gameSystems)
        {
            gameSystem->Update(gameTime);
        }
    }

    void Game::Render()
    {
        // Don't try to render anything before the first Update.
        if (running
            && time.GetFrameCount() > 0
            && !window_is_minimized(main_window)
            && BeginDraw())
        {
            Draw(time);
            EndDraw();
        }
    }
}
